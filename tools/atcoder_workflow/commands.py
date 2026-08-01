from __future__ import annotations

from collections.abc import Callable, Mapping
from dataclasses import dataclass
from enum import Enum
from pathlib import Path

from . import WorkflowError
from .compiler import BuildMode, CompilerFamily, detect_compiler
from .context import TaskContext
from .cpp import bundle_cpp, compile_cpp, run_binary, run_samples
from .runner import ProcessRunner

try:
    from .. import push_guard
except ImportError:  # Direct execution through tools/acc_wrapper.py.
    import push_guard


ACL_VERSION = "v1.6"
ACL_COMMIT = "864245a00b00dd008d1abfdc239618fdb7d139da"


class CheckStatus(Enum):
    OK = "ok"
    WARNING = "warning"
    ERROR = "error"


@dataclass(frozen=True)
class DoctorCheck:
    name: str
    status: CheckStatus
    message: str


@dataclass(frozen=True)
class WorkflowDependencies:
    runner: ProcessRunner
    environ: Mapping[str, str]
    which: Callable[[str], str | None]
    input_fn: Callable[[str], str]
    stdin_isatty: Callable[[], bool]
    guard_is_installed: Callable[[Path], bool] | None = None


def collect_doctor_checks(
    root: Path | str, dependencies: WorkflowDependencies
) -> list[DoctorCheck]:
    repository_root = Path(root)
    checks = [
        _executable_check("uv", "uv", dependencies),
        _executable_check("atcoder-cli", "acc", dependencies),
        _executable_check("oj", "oj", dependencies),
        _executable_check("oj-bundle", "oj-bundle", dependencies),
        _compiler_check(dependencies),
        _acl_check(repository_root),
        _push_guard_check(repository_root, dependencies),
        _shell_wrapper_check(repository_root, dependencies),
    ]
    return checks


def run_doctor(root: Path | str, dependencies: WorkflowDependencies) -> int:
    checks = collect_doctor_checks(root, dependencies)
    for check in checks:
        print(f"[{check.status.value}] {check.name}: {check.message}")
    if any(check.status is CheckStatus.ERROR for check in checks):
        return 1
    return 0


def _executable_check(
    name: str, executable: str, dependencies: WorkflowDependencies
) -> DoctorCheck:
    path = dependencies.which(executable)
    if path is None:
        return DoctorCheck(name, CheckStatus.ERROR, f"{executable} was not found")
    return DoctorCheck(name, CheckStatus.OK, path)


def _compiler_check(dependencies: WorkflowDependencies) -> DoctorCheck:
    try:
        compiler = detect_compiler(
            dependencies.environ, dependencies.runner, dependencies.which
        )
    except WorkflowError as error:
        return DoctorCheck("compiler", CheckStatus.ERROR, str(error))

    description = f"{compiler.executable} ({compiler.family.value} {compiler.major})"
    if compiler.family is CompilerFamily.GCC and compiler.major == 15:
        return DoctorCheck("compiler", CheckStatus.OK, description)
    return DoctorCheck(
        "compiler",
        CheckStatus.WARNING,
        f"{description}; GCC 15 is recommended",
    )


def _acl_check(root: Path) -> DoctorCheck:
    acl_directory = root / "library" / "atcoder"
    metadata_path = acl_directory / "VERSION"
    try:
        metadata = metadata_path.read_text(encoding="utf-8").splitlines()
    except OSError as error:
        return DoctorCheck(
            "acl", CheckStatus.ERROR, f"cannot read {metadata_path}: {error}"
        )

    expected_metadata = [ACL_VERSION, ACL_COMMIT]
    if metadata != expected_metadata:
        return DoctorCheck(
            "acl", CheckStatus.ERROR, f"invalid metadata in {metadata_path}"
        )

    required_paths = [
        acl_directory / "LICENSE",
        acl_directory / "all",
        root / "library" / "atcoder_local" / "core.hpp",
        root / "library" / "atcoder_local" / "io.hpp",
        root / "library" / "atcoder_local" / "debug.hpp",
    ]
    missing = [str(path) for path in required_paths if not path.is_file()]
    if missing:
        return DoctorCheck(
            "acl", CheckStatus.ERROR, f"required file is missing: {missing[0]}"
        )
    return DoctorCheck("acl", CheckStatus.OK, f"ACL {ACL_VERSION} and local headers")


def _push_guard_check(
    root: Path, dependencies: WorkflowDependencies
) -> DoctorCheck:
    checker = dependencies.guard_is_installed or push_guard.guard_is_installed
    try:
        installed = checker(root)
    except Exception as error:
        return DoctorCheck("push-guard", CheckStatus.ERROR, str(error))
    if not installed:
        return DoctorCheck("push-guard", CheckStatus.ERROR, "not installed")
    return DoctorCheck("push-guard", CheckStatus.OK, "installed")


def _shell_wrapper_check(
    root: Path, dependencies: WorkflowDependencies
) -> DoctorCheck:
    wrappers = [
        root / "tools" / "acc-wrapper.bash",
        root / "tools" / "acc-wrapper.zsh",
    ]
    missing = [str(path) for path in wrappers if not path.is_file()]
    if missing:
        return DoctorCheck(
            "shell-wrapper", CheckStatus.ERROR, f"wrapper is missing: {missing[0]}"
        )
    if dependencies.environ.get("ATCODER_LOCAL_WRAPPER") != "1":
        return DoctorCheck(
            "shell-wrapper",
            CheckStatus.ERROR,
            "current shell is not using the repository wrapper",
        )
    return DoctorCheck("shell-wrapper", CheckStatus.OK, "bash/zsh wrapper active")


def run_build(
    context: TaskContext,
    dependencies: WorkflowDependencies,
    mode: BuildMode = BuildMode.RELEASE,
) -> Path:
    _require_cpp_source(context)

    if mode is BuildMode.RELEASE:
        output_path = context.build_dir / "main"
    elif mode is BuildMode.DEBUG:
        output_path = context.build_dir / "main-debug"
    else:
        raise WorkflowError(f"unsupported build mode: {mode}")

    compiler = detect_compiler(
        dependencies.environ, dependencies.runner, dependencies.which
    )
    return compile_cpp(
        source_path=context.source_path,
        output_path=output_path,
        working_dir=context.task_dir,
        compiler=compiler,
        mode=mode,
        library_dir=context.repository_root / "library",
        runner=dependencies.runner,
    )


def run_program(
    context: TaskContext, dependencies: WorkflowDependencies
) -> int:
    binary_path = run_build(context, dependencies, mode=BuildMode.RELEASE)
    return run_binary(binary_path, context.task_dir, dependencies.runner)


def run_tests(
    context: TaskContext,
    dependencies: WorkflowDependencies,
    debug: bool = False,
) -> int:
    mode = BuildMode.DEBUG if debug else BuildMode.RELEASE
    candidate_path = context.build_dir / ".submission.cpp.pending"
    submission_path = context.build_dir / "submission.cpp"
    try:
        prepared_candidate, binary_path = _prepare_submission_candidate(
            context, dependencies, mode
        )
        assert prepared_candidate == candidate_path
        sample_returncode = run_samples(
            binary_path, context.test_dir, context.task_dir, dependencies.runner
        )
        if sample_returncode != 0:
            return sample_returncode
        try:
            candidate_path.replace(submission_path)
        except OSError as error:
            raise WorkflowError(
                f"cannot publish submission artifact: {submission_path}"
            ) from error
        return 0
    finally:
        _discard_generated_file(candidate_path, "pending submission artifact")


def _prepare_submission_candidate(
    context: TaskContext,
    dependencies: WorkflowDependencies,
    mode: BuildMode,
) -> tuple[Path, Path]:
    _require_cpp_source(context)
    submission_path = context.build_dir / "submission.cpp"
    candidate_path = context.build_dir / ".submission.cpp.pending"
    binary_name = (
        "submission-main-debug" if mode is BuildMode.DEBUG else "submission-main"
    )
    binary_path = context.build_dir / binary_name
    library_dir = context.repository_root / "library"

    _discard_generated_file(submission_path, "verified submission artifact")
    _discard_generated_file(candidate_path, "pending submission artifact")
    compiler = detect_compiler(
        dependencies.environ, dependencies.runner, dependencies.which
    )
    bundle_environment = dict(dependencies.environ)
    bundle_environment["CXX"] = compiler.executable
    bundle_cpp(
        source_path=context.source_path,
        output_path=candidate_path,
        working_dir=context.task_dir,
        library_dir=library_dir,
        runner=dependencies.runner,
        environment=bundle_environment,
    )
    compile_cpp(
        source_path=candidate_path,
        output_path=binary_path,
        working_dir=context.task_dir,
        compiler=compiler,
        mode=mode,
        library_dir=library_dir,
        runner=dependencies.runner,
    )
    return candidate_path, binary_path


def _discard_generated_file(path: Path, description: str) -> None:
    try:
        path.unlink(missing_ok=True)
    except OSError as error:
        raise WorkflowError(f"cannot remove {description}: {path}") from error


def _require_fresh_submission(context: TaskContext) -> Path:
    submission_path = context.build_dir / "submission.cpp"
    library_dir = context.repository_root / "library"
    if not submission_path.is_file():
        raise WorkflowError(
            f"submission artifact is missing: {submission_path}; run acc test first"
        )
    if not library_dir.is_dir():
        raise WorkflowError(f"library directory does not exist: {library_dir}")

    try:
        submission_mtime = submission_path.stat().st_mtime_ns
        inputs = [
            context.source_path,
            *(path for path in library_dir.rglob("*") if path.is_file()),
        ]
        stale_input = next(
            (path for path in inputs if path.stat().st_mtime_ns > submission_mtime),
            None,
        )
    except OSError as error:
        raise WorkflowError(
            f"cannot validate submission artifact: {submission_path}"
        ) from error

    if stale_input is not None:
        raise WorkflowError(
            f"submission artifact is stale: {stale_input} is newer; "
            "run acc test first"
        )
    return submission_path


def run_submit(
    context: TaskContext, dependencies: WorkflowDependencies
) -> int:
    _require_cpp_source(context)
    if not dependencies.stdin_isatty():
        raise WorkflowError("submission requires an interactive terminal")
    submission_path = _require_fresh_submission(context)

    print(f"Contest: {context.contest_id}")
    print(f"Task: {context.task_id}")
    print(f"File: {submission_path}")
    try:
        answer = dependencies.input_fn(
            f"Submit {context.task_id} from {submission_path}? [y/N] "
        )
    except (EOFError, KeyboardInterrupt) as error:
        raise WorkflowError("submission confirmation was interrupted") from error
    if answer.strip().casefold() not in {"y", "yes"}:
        return 1

    raw_acc = dependencies.which("acc")
    if raw_acc is None:
        raise WorkflowError("acc executable was not found")

    try:
        result = dependencies.runner(
            [
                raw_acc,
                "submit",
                str(submission_path),
                "-c",
                context.contest_id,
                "-t",
                context.task_id,
            ],
            cwd=context.task_dir,
        )
    except OSError as error:
        raise WorkflowError(f"submission failed for {submission_path}") from error
    return result.returncode


def _require_cpp_source(context: TaskContext) -> None:
    if context.source_path.suffix != ".cpp":
        raise WorkflowError(f"unsupported source file: {context.source_path}")
