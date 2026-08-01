from __future__ import annotations

from collections.abc import Callable, Mapping
from dataclasses import dataclass
from pathlib import Path

from . import WorkflowError
from .compiler import BuildMode, detect_compiler
from .context import TaskContext
from .cpp import bundle_cpp, compile_cpp, run_binary, run_samples
from .runner import ProcessRunner


@dataclass(frozen=True)
class WorkflowDependencies:
    runner: ProcessRunner
    environ: Mapping[str, str]
    which: Callable[[str], str | None]
    input_fn: Callable[[str], str]
    stdin_isatty: Callable[[], bool]


def run_build(
    context: TaskContext,
    dependencies: WorkflowDependencies,
    mode: BuildMode = BuildMode.RELEASE,
) -> Path:
    if context.source_path.suffix != ".cpp":
        raise WorkflowError(f"unsupported source file: {context.source_path}")

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
    binary_path = run_build(context, dependencies, mode=mode)
    return run_samples(
        binary_path, context.test_dir, context.task_dir, dependencies.runner
    )


def run_submit(
    context: TaskContext, dependencies: WorkflowDependencies
) -> int:
    compiler = detect_compiler(
        dependencies.environ, dependencies.runner, dependencies.which
    )
    submission_path = context.build_dir / "submission.cpp"
    binary_path = context.build_dir / "submission-main"
    library_dir = context.repository_root / "library"

    bundle_cpp(
        source_path=context.source_path,
        output_path=submission_path,
        working_dir=context.task_dir,
        library_dir=library_dir,
        runner=dependencies.runner,
    )
    compile_cpp(
        source_path=submission_path,
        output_path=binary_path,
        working_dir=context.task_dir,
        compiler=compiler,
        mode=BuildMode.RELEASE,
        library_dir=library_dir,
        runner=dependencies.runner,
    )
    sample_returncode = run_samples(
        binary_path, context.test_dir, context.task_dir, dependencies.runner
    )
    if sample_returncode != 0:
        return sample_returncode

    if not dependencies.stdin_isatty():
        raise WorkflowError("submission requires an interactive terminal")

    print(f"Contest: {context.contest_id}")
    print(f"Task: {context.task_id}")
    print(f"File: {submission_path}")
    answer = dependencies.input_fn(
        f"Submit {context.task_id} from {submission_path}? [y/N] "
    )
    if answer.strip().casefold() not in {"y", "yes"}:
        return 1

    raw_acc = dependencies.which("acc")
    if raw_acc is None:
        raise WorkflowError("acc executable was not found")

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
    return result.returncode
