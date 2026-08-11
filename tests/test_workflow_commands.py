from __future__ import annotations

from collections.abc import Mapping, Sequence
from contextlib import contextmanager
from dataclasses import replace
import os
from pathlib import Path
import subprocess
from typing import Iterator
from unittest.mock import Mock, patch

import pytest

from tools.atcoder_workflow import WorkflowError
from tools.atcoder_workflow.cli import _default_dependencies, main
from tools.atcoder_workflow.commands import (
    WorkflowDependencies,
    run_build,
    run_program,
    run_submit,
    run_tests,
)
from tools.atcoder_workflow.compiler import (
    BuildMode,
    CompilerFamily,
    CompilerInfo,
)
from tools.atcoder_workflow.context import TaskContext


ROOT = Path(__file__).parents[1]

GCC15 = CompilerInfo(
    executable="/opt/g++-15",
    family=CompilerFamily.GCC,
    major=15,
    version_text="g++ (GCC) 15.2.0",
)
CONTEXT = TaskContext(
    repository_root=Path("/repo"),
    contest_id="abc999",
    task_id="abc999_a",
    task_label="A",
    contest_dir=Path("/repo/contests/abc999"),
    task_dir=Path("/repo/contests/abc999/a"),
    source_path=Path("/repo/contests/abc999/a/main.cpp"),
    submission_path=Path("/repo/contests/abc999/a/submission.cpp"),
    test_dir=Path("/repo/contests/abc999/a/test"),
    build_dir=Path("/repo/.atcoder-local/build/abc999/abc999_a"),
)


def submission_context(tmp_path: Path) -> TaskContext:
    repository_root = tmp_path / "repo"
    task_dir = repository_root / "contests/abc999/a"
    source_path = task_dir / "main.cpp"
    test_dir = task_dir / "test"
    library_header = repository_root / "library/atcoder_local/core.hpp"
    test_dir.mkdir(parents=True)
    library_header.parent.mkdir(parents=True)
    source_path.write_text("int main() {}\n", encoding="utf-8")
    library_header.write_text("#pragma once\n", encoding="utf-8")
    return TaskContext(
        repository_root=repository_root,
        contest_id="abc999",
        task_id="abc999_a",
        task_label="A",
        contest_dir=task_dir.parent,
        task_dir=task_dir,
        source_path=source_path,
        submission_path=task_dir / "submission.cpp",
        test_dir=test_dir,
        build_dir=repository_root / ".atcoder-local/build/abc999/abc999_a",
    )


def without_samples(context: TaskContext) -> TaskContext:
    return replace(context, test_dir=None)


def set_mtime(path: Path, value: int) -> None:
    path.touch(exist_ok=True)
    os.utime(path, ns=(value, value))


def test_submit_rejects_missing_artifact_before_prompt(tmp_path: Path) -> None:
    context = submission_context(tmp_path)
    events: list[str] = []

    with pytest.raises(WorkflowError, match=r"run acc test first"):
        run_submit(context, submit_dependencies(events))

    assert events == []


@pytest.mark.parametrize("newer_input", ["source", "library"])
def test_submit_rejects_artifact_older_than_submission_input(
    tmp_path: Path,
    newer_input: str,
) -> None:
    context = submission_context(tmp_path)
    events: list[str] = []
    submission = context.submission_path
    submission.parent.mkdir(parents=True, exist_ok=True)
    header = context.repository_root / "library/atcoder_local/core.hpp"
    set_mtime(context.source_path, 100)
    set_mtime(header, 100)
    set_mtime(submission, 200)
    set_mtime(context.source_path if newer_input == "source" else header, 300)

    with pytest.raises(WorkflowError, match=r"stale.*run acc test first"):
        run_submit(context, submit_dependencies(events))

    assert events == []


def test_submit_accepts_artifact_newer_than_source_and_library(
    tmp_path: Path,
) -> None:
    context = submission_context(tmp_path)
    events: list[str] = []
    submission = context.submission_path
    submission.parent.mkdir(parents=True, exist_ok=True)
    header = context.repository_root / "library/atcoder_local/core.hpp"
    set_mtime(context.source_path, 100)
    set_mtime(header, 100)
    set_mtime(submission, 200)

    assert run_submit(context, submit_dependencies(events)) == 0
    assert events == ["prompt", "submit"]


def test_submit_rejects_missing_library_before_prompt(tmp_path: Path) -> None:
    context = submission_context(tmp_path)
    events: list[str] = []
    submission = context.submission_path
    submission.parent.mkdir(parents=True, exist_ok=True)
    submission.write_text("bundled source\n", encoding="utf-8")
    library_dir = context.repository_root / "library"
    library_dir.rename(context.repository_root / "library-hidden")

    with pytest.raises(WorkflowError, match=r"library directory"):
        run_submit(context, submit_dependencies(events))

    assert events == []


def test_submit_rejects_unreadable_library_input_before_prompt(
    tmp_path: Path,
) -> None:
    context = submission_context(tmp_path)
    events: list[str] = []
    submission = context.submission_path
    submission.parent.mkdir(parents=True, exist_ok=True)
    unreadable = context.repository_root / "library/atcoder_local/unreadable.hpp"
    set_mtime(context.source_path, 100)
    set_mtime(context.repository_root / "library/atcoder_local/core.hpp", 100)
    set_mtime(unreadable, 100)
    set_mtime(submission, 200)
    original_stat = Path.stat
    original_is_file = Path.is_file

    def fail_unreadable_stat(path: Path, *args: object, **kwargs: object):
        if path == unreadable:
            raise OSError("cannot stat library input")
        return original_stat(path, *args, **kwargs)

    def suppress_unreadable_is_file(path: Path) -> bool:
        if path != unreadable:
            return original_is_file(path)
        try:
            path.stat()
        except OSError:
            return False
        return True

    with (
        patch.object(Path, "stat", autospec=True, side_effect=fail_unreadable_stat),
        patch.object(
            Path,
            "is_file",
            autospec=True,
            side_effect=suppress_unreadable_is_file,
        ),
    ):
        with pytest.raises(
            WorkflowError, match=r"cannot validate submission artifact"
        ):
            run_submit(context, submit_dependencies(events))

    assert events == []


def test_submit_rejects_unreadable_library_directory_before_prompt(
    tmp_path: Path,
) -> None:
    context = submission_context(tmp_path)
    events: list[str] = []
    submission = context.submission_path
    submission.parent.mkdir(parents=True, exist_ok=True)
    restricted = context.repository_root / "library/atcoder_local/restricted"
    restricted_header = restricted / "hidden.hpp"
    restricted.mkdir()
    set_mtime(context.source_path, 100)
    set_mtime(context.repository_root / "library/atcoder_local/core.hpp", 100)
    set_mtime(restricted_header, 100)
    set_mtime(submission, 200)
    original_scandir = os.scandir

    def fail_restricted_scandir(path: str | bytes | os.PathLike[str]):
        if Path(path) == restricted:
            raise PermissionError("cannot scan library directory")
        return original_scandir(path)

    with patch("os.scandir", side_effect=fail_restricted_scandir):
        with pytest.raises(
            WorkflowError, match=r"cannot validate submission artifact"
        ):
            run_submit(context, submit_dependencies(events))

    assert events == []


def unused_runner(
    argv: Sequence[str],
    *,
    cwd: Path | str | None = None,
    env: Mapping[str, str] | None = None,
    capture_output: bool = False,
) -> subprocess.CompletedProcess[str]:
    del argv, cwd, env, capture_output
    raise AssertionError("runner should not be called directly")


DEPENDENCIES = WorkflowDependencies(
    runner=unused_runner,
    environ={"CXX": "/opt/g++-15"},
    which=lambda name: f"/usr/bin/{name}",
    input_fn=lambda prompt: prompt,
    stdin_isatty=lambda: True,
)


def test_readme_documents_complete_workflow() -> None:
    readme = (ROOT / "README.md").read_text(encoding="utf-8")
    required = [
        "uv sync --group dev",
        "uv run acc check-oj",
        "tools/acc-wrapper.zsh",
        "tools/acc-wrapper.bash",
        "acc doctor",
        "acc build",
        "acc run",
        "acc test",
        "acc test --debug",
        "acc submit",
        "acc test -c abc123 -t a",
        "CAPTCHA",
        "Rated",
    ]
    for fragment in required:
        assert fragment in readme


def test_readme_documents_test_owned_submission_artifact() -> None:
    readme = (ROOT / "README.md").read_text(encoding="utf-8")

    assert "`acc test` と `acc test --debug` は" in readme
    assert "`submission.cpp`" in readme
    assert "各タスクディレクトリの `submission.cpp`" in readme
    assert "サンプルテストがない場合" in readme
    assert "サンプルテストを省略" in readme
    assert "`acc submit` は bundle、コンパイル、サンプルテストを実行しません" in readme
    assert "先に `acc test` を実行" in readme
    assert "`main.cpp` または `library/` 配下の任意のファイル" in readme


def test_readme_documents_installed_acc_and_task_local_submission() -> None:
    readme = (ROOT / "README.md").read_text(encoding="utf-8")

    assert "uv tool install --editable ." in readme
    assert "uv tool install --force --editable ." in readme
    assert "インストール済みの `acc` で `acc doctor`" in readme
    assert "各タスクディレクトリの `submission.cpp`" in readme
    assert "サンプルテストがない場合" in readme
    assert "サンプルテストを省略" in readme


def test_gitignore_scopes_generated_submission_to_contests() -> None:
    ignore = (ROOT / ".gitignore").read_text(encoding="utf-8")
    lines = {line.strip() for line in ignore.splitlines()}

    assert "contests/**/submission.cpp" in lines
    assert "contests/**/.submission.cpp.pending" in lines
    assert "**/submission.cpp" not in lines


def test_readme_omits_retired_features() -> None:
    readme = (ROOT / "README.md").read_text(encoding="utf-8")

    retired_fragments = ["push" + "-guard", "memo" + ".md", "--no-" + "verify"]
    for fragment in retired_fragments:
        assert fragment not in readme


def test_readme_documents_shell_startup_file_edits_are_manual() -> None:
    readme = (ROOT / "README.md").read_text(encoding="utf-8")

    assert "`.zshrc` や `.bashrc` を自動では変更しません" in readme


def test_readme_pins_supported_atcoder_cli_adapter_version() -> None:
    readme_lines = (ROOT / "README.md").read_text(encoding="utf-8").splitlines()

    assert "npm install -g atcoder-cli@2.2.0" in readme_lines
    assert "npm install -g atcoder-cli" not in readme_lines


def submit_dependencies(
    events: list[str],
    *,
    answer: str = "yes",
    tty: bool = True,
    submit_returncode: int = 0,
    raw_acc: str | None = "/usr/local/bin/acc",
) -> WorkflowDependencies:
    def runner(
        argv: Sequence[str],
        *,
        cwd: Path | str | None = None,
        env: Mapping[str, str] | None = None,
        capture_output: bool = False,
    ) -> subprocess.CompletedProcess[str]:
        del cwd, env, capture_output
        events.append("submit")
        return subprocess.CompletedProcess(argv, submit_returncode)

    def input_fn(prompt: str) -> str:
        del prompt
        events.append("prompt")
        return answer

    return WorkflowDependencies(
        runner=runner,
        environ={"CXX": "/opt/g++-15"},
        which=lambda name: raw_acc if name == "acc" else None,
        input_fn=input_fn,
        stdin_isatty=lambda: tty,
    )


@contextmanager
def patched_fresh_submission(
    context: TaskContext = CONTEXT,
) -> Iterator[Path]:
    submission = context.submission_path
    with patch(
        "tools.atcoder_workflow.commands._require_fresh_submission",
        return_value=submission,
    ) as validate:
        yield submission
    validate.assert_called_once()
    assert validate.call_args.args[0].submission_path == submission


@contextmanager
def patched_test_stages(
    context: TaskContext,
    *,
    failed_stage: str | None = None,
) -> Iterator[tuple[list[str], Mock, Mock, Mock]]:
    events: list[str] = []

    def bundle_stage(**kwargs: object) -> Path:
        events.append("bundle")
        if failed_stage == "bundle":
            raise WorkflowError("bundle failed")
        output = kwargs["output_path"]
        assert isinstance(output, Path)
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text("bundled source\n", encoding="utf-8")
        return output

    def compile_stage(**kwargs: object) -> Path:
        events.append("compile")
        if failed_stage == "compile":
            raise WorkflowError("compile failed")
        output = kwargs["output_path"]
        assert isinstance(output, Path)
        return output

    def sample_stage(*args: object, **kwargs: object) -> int:
        del args, kwargs
        events.append("samples")
        return 17 if failed_stage == "samples" else 0

    with (
        patch(
            "tools.atcoder_workflow.commands.detect_compiler", return_value=GCC15
        ),
        patch(
            "tools.atcoder_workflow.commands.bundle_cpp", side_effect=bundle_stage
        ) as bundle,
        patch(
            "tools.atcoder_workflow.commands.compile_cpp", side_effect=compile_stage
        ) as compile_,
        patch(
            "tools.atcoder_workflow.commands.run_samples", side_effect=sample_stage
        ) as samples,
    ):
        yield events, bundle, compile_, samples


def test_run_build_uses_release_output_and_repository_library() -> None:
    with (
        patch(
            "tools.atcoder_workflow.commands.detect_compiler", return_value=GCC15
        ) as detect,
        patch(
            "tools.atcoder_workflow.commands.compile_cpp",
            return_value=CONTEXT.build_dir / "main",
        ) as compile_,
    ):
        result = run_build(CONTEXT, DEPENDENCIES)

    assert result == CONTEXT.build_dir / "main"
    detect.assert_called_once_with(
        DEPENDENCIES.environ, DEPENDENCIES.runner, DEPENDENCIES.which
    )
    compile_.assert_called_once_with(
        source_path=CONTEXT.source_path,
        output_path=CONTEXT.build_dir / "main",
        working_dir=CONTEXT.task_dir,
        compiler=GCC15,
        mode=BuildMode.RELEASE,
        library_dir=CONTEXT.repository_root / "library",
        runner=DEPENDENCIES.runner,
    )


def test_build_does_not_require_sample_directory() -> None:
    context = without_samples(CONTEXT)
    with patch(
        "tools.atcoder_workflow.commands.detect_compiler", return_value=GCC15
    ), patch(
        "tools.atcoder_workflow.commands.compile_cpp",
        return_value=context.build_dir / "main",
    ) as compile_:
        assert run_build(context, DEPENDENCIES) == context.build_dir / "main"

    compile_.assert_called_once()


def test_run_does_not_require_sample_directory() -> None:
    context = without_samples(CONTEXT)
    binary = context.build_dir / "main"
    with patch(
        "tools.atcoder_workflow.commands.run_build", return_value=binary
    ) as build, patch(
        "tools.atcoder_workflow.commands.run_binary", return_value=19
    ) as run:
        assert run_program(context, DEPENDENCIES) == 19

    build.assert_called_once_with(context, DEPENDENCIES, mode=BuildMode.RELEASE)
    run.assert_called_once_with(binary, context.task_dir, DEPENDENCIES.runner)


def test_run_program_builds_before_running_and_propagates_exit_code() -> None:
    events: list[str] = []
    binary = CONTEXT.build_dir / "main"
    with (
        patch(
            "tools.atcoder_workflow.commands.run_build",
            side_effect=lambda *args, **kwargs: events.append("compile") or binary,
        ) as build,
        patch(
            "tools.atcoder_workflow.commands.run_binary",
            side_effect=lambda *args, **kwargs: events.append("run") or 37,
        ) as run,
    ):
        result = run_program(CONTEXT, DEPENDENCIES)

    assert result == 37
    assert events == ["compile", "run"]
    build.assert_called_once_with(CONTEXT, DEPENDENCIES, mode=BuildMode.RELEASE)
    run.assert_called_once_with(binary, CONTEXT.task_dir, DEPENDENCIES.runner)


def test_run_tests_bundles_compiles_samples_then_publishes_release_artifact(
    tmp_path: Path,
) -> None:
    context = submission_context(tmp_path)
    events: list[str] = []
    published = context.submission_path
    candidate = context.task_dir / ".submission.cpp.pending"
    binary = context.build_dir / "submission-main"

    def bundle_stage(**kwargs: object) -> Path:
        events.append("bundle")
        output = kwargs["output_path"]
        assert isinstance(output, Path)
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text("bundled source\n", encoding="utf-8")
        return output

    with (
        patch(
            "tools.atcoder_workflow.commands.detect_compiler", return_value=GCC15
        ),
        patch(
            "tools.atcoder_workflow.commands.bundle_cpp", side_effect=bundle_stage
        ) as bundle,
        patch(
            "tools.atcoder_workflow.commands.compile_cpp",
            side_effect=lambda **kwargs: events.append("compile")
            or kwargs["output_path"],
        ) as compile_,
        patch(
            "tools.atcoder_workflow.commands.run_samples",
            side_effect=lambda *args: events.append("samples") or 0,
        ) as samples,
    ):
        result = run_tests(context, DEPENDENCIES)

    assert result == 0
    assert events == ["bundle", "compile", "samples"]
    assert published.read_text(encoding="utf-8") == "bundled source\n"
    assert not candidate.exists()
    bundle.assert_called_once_with(
        source_path=context.source_path,
        output_path=candidate,
        working_dir=context.task_dir,
        library_dir=context.repository_root / "library",
        runner=DEPENDENCIES.runner,
        environment={**DEPENDENCIES.environ, "CXX": GCC15.executable},
    )
    assert compile_.call_args.kwargs["source_path"] == candidate
    assert compile_.call_args.kwargs["output_path"] == binary
    assert compile_.call_args.kwargs["mode"] is BuildMode.RELEASE
    samples.assert_called_once_with(
        binary, context.test_dir, context.task_dir, DEPENDENCIES.runner
    )


def test_run_tests_compiler_failure_invalidates_previous_submission(
    tmp_path: Path,
) -> None:
    context = submission_context(tmp_path)
    published = context.submission_path
    published.parent.mkdir(parents=True, exist_ok=True)
    published.write_text("old verified source\n", encoding="utf-8")

    with patch(
        "tools.atcoder_workflow.commands.detect_compiler",
        side_effect=WorkflowError("compiler detection failed"),
    ):
        with pytest.raises(WorkflowError, match="compiler detection failed"):
            run_tests(context, DEPENDENCIES)

    assert not published.exists()


@pytest.mark.parametrize("debug", [False, True])
def test_run_tests_without_samples_warns_and_publishes(
    tmp_path: Path,
    capsys: pytest.CaptureFixture[str],
    debug: bool,
) -> None:
    context = without_samples(submission_context(tmp_path))
    published = context.submission_path

    with patched_test_stages(context) as (events, _, _, samples):
        assert run_tests(context, DEPENDENCIES, debug=debug) == 0

    assert events == ["bundle", "compile"]
    samples.assert_not_called()
    assert published.read_text(encoding="utf-8") == "bundled source\n"
    assert capsys.readouterr().err == (
        "[warning] sample tests are unavailable; "
        "submission.cpp was not sample-tested\n"
    )


def test_run_tests_debug_compiles_pending_bundle_to_debug_output(
    tmp_path: Path,
) -> None:
    context = submission_context(tmp_path)
    with patched_test_stages(context) as (events, _, compile_, samples):
        assert run_tests(context, DEPENDENCIES, debug=True) == 0

    assert events == ["bundle", "compile", "samples"]
    assert compile_.call_args.kwargs["source_path"] == (
        context.task_dir / ".submission.cpp.pending"
    )
    assert compile_.call_args.kwargs["output_path"] == (
        context.build_dir / "submission-main-debug"
    )
    assert compile_.call_args.kwargs["mode"] is BuildMode.DEBUG
    samples.assert_called_once()


@pytest.mark.parametrize("failed_stage", ["bundle", "compile", "samples"])
def test_run_tests_failure_invalidates_submission_and_cleans_candidate(
    tmp_path: Path,
    failed_stage: str,
) -> None:
    context = submission_context(tmp_path)
    published = context.submission_path
    candidate = context.task_dir / ".submission.cpp.pending"
    published.parent.mkdir(parents=True, exist_ok=True)
    published.write_text("old verified source\n", encoding="utf-8")

    with patched_test_stages(context, failed_stage=failed_stage) as (
        events,
        _,
        _,
        _,
    ):
        if failed_stage == "samples":
            assert run_tests(context, DEPENDENCIES) == 17
        else:
            with pytest.raises(WorkflowError, match=f"{failed_stage} failed"):
                run_tests(context, DEPENDENCIES)

    assert not published.exists()
    assert not candidate.exists()
    assert events == {
        "bundle": ["bundle"],
        "compile": ["bundle", "compile"],
        "samples": ["bundle", "compile", "samples"],
    }[failed_stage]


def test_failed_build_stops_before_run() -> None:
    build_error = WorkflowError("compile failed")
    with (
        patch(
            "tools.atcoder_workflow.commands.run_build", side_effect=build_error
        ),
        patch("tools.atcoder_workflow.commands.run_binary") as run,
        patch("tools.atcoder_workflow.commands.run_samples") as samples,
    ):
        with pytest.raises(WorkflowError) as raised:
            run_program(CONTEXT, DEPENDENCIES)

    assert raised.value is build_error
    run.assert_not_called()
    samples.assert_not_called()


def test_build_rejects_unsupported_source_before_compiler_detection() -> None:
    python_context = TaskContext(
        **{**CONTEXT.__dict__, "source_path": CONTEXT.task_dir / "main.py"}
    )
    with patch("tools.atcoder_workflow.commands.detect_compiler") as detect:
        with pytest.raises(WorkflowError, match="unsupported source file"):
            run_build(python_context, DEPENDENCIES)

    detect.assert_not_called()


def test_submit_rejects_unsupported_source_before_any_stage() -> None:
    events: list[str] = []
    python_context = TaskContext(
        **{**CONTEXT.__dict__, "source_path": CONTEXT.task_dir / "main.py"}
    )
    dependencies = submit_dependencies(events)

    with (
        patch("tools.atcoder_workflow.commands.detect_compiler") as detect,
        patch("tools.atcoder_workflow.commands.bundle_cpp") as bundle,
        patch("tools.atcoder_workflow.commands.compile_cpp") as compile_,
        patch("tools.atcoder_workflow.commands.run_samples") as samples,
    ):
        with pytest.raises(WorkflowError, match="unsupported source file"):
            run_submit(python_context, dependencies)

    detect.assert_not_called()
    bundle.assert_not_called()
    compile_.assert_not_called()
    samples.assert_not_called()
    assert events == []


def test_submit_displays_verified_artifact_before_prompt(
    capsys: pytest.CaptureFixture[str],
) -> None:
    events: list[str] = []
    dependencies = submit_dependencies(events)
    submission = CONTEXT.submission_path

    with patched_fresh_submission():
        result = run_submit(CONTEXT, dependencies)

    assert result == 0
    assert events == ["prompt", "submit"]
    assert capsys.readouterr().out == (
        "Contest: abc999\n"
        "Task: abc999_a\n"
        f"File: {submission}\n"
    )


def test_submit_never_builds_or_runs_samples() -> None:
    events: list[str] = []
    with (
        patched_fresh_submission(),
        patch("tools.atcoder_workflow.commands.detect_compiler") as detect,
        patch("tools.atcoder_workflow.commands.bundle_cpp") as bundle,
        patch("tools.atcoder_workflow.commands.compile_cpp") as compile_,
        patch("tools.atcoder_workflow.commands.run_samples") as samples,
    ):
        assert run_submit(CONTEXT, submit_dependencies(events)) == 0

    detect.assert_not_called()
    bundle.assert_not_called()
    compile_.assert_not_called()
    samples.assert_not_called()
    assert events == ["prompt", "submit"]


def test_submit_accepts_fresh_artifact_without_samples() -> None:
    context = without_samples(CONTEXT)
    events: list[str] = []

    with patched_fresh_submission():
        assert run_submit(context, submit_dependencies(events)) == 0

    assert events == ["prompt", "submit"]


def test_submit_rejects_non_tty_before_prompt_or_submission() -> None:
    events: list[str] = []
    dependencies = submit_dependencies(events, tty=False)

    with pytest.raises(WorkflowError, match="interactive terminal"):
        run_submit(CONTEXT, dependencies)

    assert events == []


@pytest.mark.parametrize("answer", ["", "n", "no", "anything else"])
def test_submit_defaults_to_no_and_rejects_other_answers(answer: str) -> None:
    events: list[str] = []
    dependencies = submit_dependencies(events, answer=answer)

    with patched_fresh_submission():
        assert run_submit(CONTEXT, dependencies) == 1

    assert events == ["prompt"]


@pytest.mark.parametrize("answer", ["y", "Y", "yes", "YES", "  YeS  "])
def test_submit_accepts_only_y_or_yes_case_insensitively(answer: str) -> None:
    events: list[str] = []
    dependencies = submit_dependencies(events, answer=answer)

    with patched_fresh_submission():
        assert run_submit(CONTEXT, dependencies) == 0

    assert events == ["prompt", "submit"]


def test_submit_uses_raw_acc_with_full_task_id_and_propagates_failure() -> None:
    events: list[str] = []
    dependencies = submit_dependencies(events, submit_returncode=42)

    with patched_fresh_submission():
        result = run_submit(CONTEXT, dependencies)

    assert result == 42
    assert events == ["prompt", "submit"]


def test_submit_has_no_fallback_when_raw_acc_is_unavailable() -> None:
    events: list[str] = []
    dependencies = submit_dependencies(events, raw_acc=None)

    with patched_fresh_submission():
        with pytest.raises(WorkflowError, match="acc executable"):
            run_submit(CONTEXT, dependencies)

    assert events == ["prompt"]


def test_submit_process_receives_exact_argv_and_working_directory() -> None:
    events: list[str] = []
    calls: list[tuple[list[str], Path | str | None, bool]] = []

    def runner(
        argv: Sequence[str],
        *,
        cwd: Path | str | None = None,
        env: Mapping[str, str] | None = None,
        capture_output: bool = False,
    ) -> subprocess.CompletedProcess[str]:
        del env
        calls.append((list(argv), cwd, capture_output))
        events.append("submit")
        return subprocess.CompletedProcess(argv, 31)

    dependencies = WorkflowDependencies(
        runner=runner,
        environ={"CXX": "/opt/g++-15"},
        which=lambda name: "/opt/homebrew/bin/acc" if name == "acc" else None,
        input_fn=lambda prompt: events.append("prompt") or "yes",
        stdin_isatty=lambda: True,
    )

    with patched_fresh_submission():
        result = run_submit(CONTEXT, dependencies)

    assert result == 31
    assert calls == [
        (
            [
                "/opt/homebrew/bin/acc",
                "submit",
                str(CONTEXT.submission_path),
                "-c",
                "abc999",
                "-t",
                "abc999_a",
            ],
            CONTEXT.task_dir,
            False,
        )
    ]


def test_submit_normalizes_raw_acc_spawn_oserror_without_retry_or_fallback() -> None:
    events: list[str] = []
    calls: list[tuple[list[str], Path | str | None]] = []
    spawn_error = OSError("cannot spawn acc")
    locate = Mock(return_value="/usr/local/bin/acc")

    def runner(
        argv: Sequence[str],
        *,
        cwd: Path | str | None = None,
        env: Mapping[str, str] | None = None,
        capture_output: bool = False,
    ) -> subprocess.CompletedProcess[str]:
        del env, capture_output
        calls.append((list(argv), cwd))
        events.append("submit")
        raise spawn_error

    dependencies = WorkflowDependencies(
        runner=runner,
        environ={"CXX": GCC15.executable},
        which=locate,
        input_fn=lambda prompt: events.append("prompt") or "yes",
        stdin_isatty=lambda: True,
    )
    submission = CONTEXT.submission_path

    with patched_fresh_submission():
        with pytest.raises(WorkflowError) as raised:
            run_submit(CONTEXT, dependencies)

    assert str(raised.value) == f"submission failed for {submission}"
    assert raised.value.__cause__ is spawn_error
    assert calls == [
        (
            [
                "/usr/local/bin/acc",
                "submit",
                str(submission),
                "-c",
                "abc999",
                "-t",
                "abc999_a",
            ],
            CONTEXT.task_dir,
        )
    ]
    locate.assert_called_once_with("acc")
    assert events == ["prompt", "submit"]


@pytest.mark.parametrize("control_flow", [KeyboardInterrupt(), SystemExit(9)])
def test_submit_process_control_flow_exceptions_propagate(
    control_flow: BaseException,
) -> None:
    events: list[str] = []

    def runner(
        argv: Sequence[str],
        *,
        cwd: Path | str | None = None,
        env: Mapping[str, str] | None = None,
        capture_output: bool = False,
    ) -> subprocess.CompletedProcess[str]:
        del argv, cwd, env, capture_output
        events.append("submit")
        raise control_flow

    dependencies = WorkflowDependencies(
        runner=runner,
        environ={"CXX": GCC15.executable},
        which=lambda name: "/usr/local/bin/acc" if name == "acc" else None,
        input_fn=lambda prompt: events.append("prompt") or "yes",
        stdin_isatty=lambda: True,
    )

    with patched_fresh_submission():
        with pytest.raises(type(control_flow)) as raised:
            run_submit(CONTEXT, dependencies)

    assert raised.value is control_flow
    assert events == ["prompt", "submit"]


def test_submit_prompt_names_full_task_and_bundled_file() -> None:
    events: list[str] = []
    prompts: list[str] = []

    def input_fn(prompt: str) -> str:
        prompts.append(prompt)
        events.append("prompt")
        return ""

    base = submit_dependencies(events)
    dependencies = WorkflowDependencies(
        runner=base.runner,
        environ=base.environ,
        which=base.which,
        input_fn=input_fn,
        stdin_isatty=base.stdin_isatty,
    )

    with patched_fresh_submission():
        assert run_submit(CONTEXT, dependencies) == 1

    assert prompts == [
        f"Submit abc999_a from {CONTEXT.submission_path}? [y/N] "
    ]


@pytest.mark.parametrize("interruption_type", [EOFError, KeyboardInterrupt])
def test_submit_normalizes_interrupted_confirmation_without_running_acc(
    interruption_type: type[BaseException],
) -> None:
    events: list[str] = []
    base = submit_dependencies(events)
    locate = Mock(return_value="/usr/local/bin/acc")
    dependencies = WorkflowDependencies(
        runner=base.runner,
        environ=base.environ,
        which=locate,
        input_fn=Mock(side_effect=interruption_type),
        stdin_isatty=base.stdin_isatty,
    )

    with patched_fresh_submission():
        with pytest.raises(WorkflowError, match="confirmation.*interrupted"):
            run_submit(CONTEXT, dependencies)

    locate.assert_not_called()
    assert events == []


def test_cli_dispatches_submit_and_propagates_raw_exit_code() -> None:
    with (
        patch(
            "tools.atcoder_workflow.cli.resolve_task_context", return_value=CONTEXT
        ) as resolve,
        patch("tools.atcoder_workflow.cli.run_submit", return_value=29) as submit,
    ):
        result = main(
            ["submit", "-c", "abc999", "-t", "a"],
            cwd=CONTEXT.repository_root,
            dependencies=DEPENDENCIES,
        )

    assert result == 29
    resolve.assert_called_once_with(CONTEXT.repository_root, "abc999", "a")
    submit.assert_called_once_with(CONTEXT, DEPENDENCIES)


def test_default_dependencies_locate_acc_without_recursing_into_wrapper() -> None:
    with (
        patch(
            "tools.atcoder_workflow.cli.find_upstream_acc",
            return_value="/opt/native/bin/acc",
        ) as find_acc,
        patch(
            "tools.atcoder_workflow.cli.shutil.which",
            return_value="/opt/toolchain/bin/g++",
        ) as which,
    ):
        dependencies = _default_dependencies()
        assert dependencies.which("acc") == "/opt/native/bin/acc"
        assert dependencies.which("g++") == "/opt/toolchain/bin/g++"

    find_acc.assert_called_once_with(os.environ)
    which.assert_called_once_with("g++")


def test_cli_resolves_root_flags_once_and_dispatches_release_build() -> None:
    with (
        patch(
            "tools.atcoder_workflow.cli.resolve_task_context", return_value=CONTEXT
        ) as resolve,
        patch("tools.atcoder_workflow.cli.run_build") as build,
    ):
        result = main(
            ["build", "-c", "abc999", "-t", "a"],
            cwd=CONTEXT.repository_root,
            dependencies=DEPENDENCIES,
        )

    assert result == 0
    resolve.assert_called_once_with(CONTEXT.repository_root, "abc999", "a")
    build.assert_called_once_with(CONTEXT, DEPENDENCIES, mode=BuildMode.RELEASE)


def test_cli_resolves_task_directory_without_flags_and_propagates_run_exit() -> None:
    with (
        patch(
            "tools.atcoder_workflow.cli.resolve_task_context", return_value=CONTEXT
        ) as resolve,
        patch("tools.atcoder_workflow.cli.run_program", return_value=23) as run,
    ):
        result = main(
            ["run"], cwd=CONTEXT.task_dir, dependencies=DEPENDENCIES
        )

    assert result == 23
    resolve.assert_called_once_with(CONTEXT.task_dir, None, None)
    run.assert_called_once_with(CONTEXT, DEPENDENCIES)


@pytest.mark.parametrize(
    ("argv", "expected_debug"), [(["test"], False), (["test", "--debug"], True)]
)
def test_cli_test_debug_flag_selects_mode(
    argv: list[str], expected_debug: bool
) -> None:
    with (
        patch(
            "tools.atcoder_workflow.cli.resolve_task_context", return_value=CONTEXT
        ),
        patch("tools.atcoder_workflow.cli.run_tests", return_value=0) as tests,
    ):
        result = main(argv, cwd=CONTEXT.task_dir, dependencies=DEPENDENCIES)

    assert result == 0
    tests.assert_called_once_with(CONTEXT, DEPENDENCIES, debug=expected_debug)


@pytest.mark.parametrize("command", ["build", "run"])
def test_cli_rejects_debug_outside_test(command: str) -> None:
    with pytest.raises(SystemExit) as raised:
        main(
            [command, "--debug"],
            cwd=CONTEXT.task_dir,
            dependencies=DEPENDENCIES,
        )

    assert raised.value.code == 2


def test_cli_reports_workflow_error_to_stderr(capsys: pytest.CaptureFixture[str]) -> None:
    with patch(
        "tools.atcoder_workflow.cli.resolve_task_context",
        side_effect=WorkflowError("task is ambiguous"),
    ):
        result = main(
            ["build"], cwd=CONTEXT.repository_root, dependencies=DEPENDENCIES
        )

    assert result == 1
    assert capsys.readouterr().err == "[acc-wrapper] task is ambiguous\n"


def test_cli_reports_run_spawn_error_without_traceback(
    capsys: pytest.CaptureFixture[str],
) -> None:
    binary = CONTEXT.build_dir / "main"

    def runner(
        argv: Sequence[str],
        *,
        cwd: Path | str | None = None,
        env: Mapping[str, str] | None = None,
        capture_output: bool = False,
    ) -> subprocess.CompletedProcess[str]:
        del argv, cwd, env, capture_output
        raise OSError("cannot execute binary")

    dependencies = WorkflowDependencies(
        runner=runner,
        environ={},
        which=lambda name: None,
        input_fn=lambda prompt: "",
        stdin_isatty=lambda: False,
    )
    with (
        patch(
            "tools.atcoder_workflow.cli.resolve_task_context",
            return_value=CONTEXT,
        ),
        patch("tools.atcoder_workflow.commands.run_build", return_value=binary),
    ):
        result = main(["run"], cwd=CONTEXT.task_dir, dependencies=dependencies)

    captured = capsys.readouterr()
    assert result == 1
    assert captured.out == ""
    assert captured.err == f"[acc-wrapper] run failed for {binary}\n"
    assert "Traceback" not in captured.err
