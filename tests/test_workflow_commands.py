from __future__ import annotations

from collections.abc import Mapping, Sequence
from contextlib import contextmanager
from pathlib import Path
import subprocess
from typing import Iterator
from unittest.mock import Mock, patch

import pytest

from tools.atcoder_workflow import WorkflowError
from tools.atcoder_workflow.cli import main
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
    test_dir=Path("/repo/contests/abc999/a/test"),
    build_dir=Path("/repo/.atcoder-local/build/abc999/abc999_a"),
)


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
def patched_submit_stages(
    events: list[str],
    *,
    bundle_error: WorkflowError | None = None,
    compile_error: WorkflowError | None = None,
    sample_returncode: int = 0,
) -> Iterator[tuple[Mock, Mock, Mock]]:
    submission = CONTEXT.build_dir / "submission.cpp"
    binary = CONTEXT.build_dir / "submission-main"

    def bundle_stage(**kwargs: object) -> Path:
        del kwargs
        events.append("bundle")
        if bundle_error is not None:
            raise bundle_error
        return submission

    def compile_stage(**kwargs: object) -> Path:
        del kwargs
        events.append("compile")
        if compile_error is not None:
            raise compile_error
        return binary

    def sample_stage(*args: object, **kwargs: object) -> int:
        del args, kwargs
        events.append("samples")
        return sample_returncode

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
        yield bundle, compile_, samples


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


def test_run_tests_builds_before_samples_and_propagates_failure() -> None:
    events: list[str] = []
    binary = CONTEXT.build_dir / "main"
    with (
        patch(
            "tools.atcoder_workflow.commands.run_build",
            side_effect=lambda *args, **kwargs: events.append("compile") or binary,
        ) as build,
        patch(
            "tools.atcoder_workflow.commands.run_samples",
            side_effect=lambda *args, **kwargs: events.append("samples") or 19,
        ) as samples,
    ):
        result = run_tests(CONTEXT, DEPENDENCIES)

    assert result == 19
    assert events == ["compile", "samples"]
    build.assert_called_once_with(CONTEXT, DEPENDENCIES, mode=BuildMode.RELEASE)
    samples.assert_called_once_with(
        binary, CONTEXT.test_dir, CONTEXT.task_dir, DEPENDENCIES.runner
    )


def test_run_tests_debug_selects_debug_build_and_output() -> None:
    binary = CONTEXT.build_dir / "main-debug"
    with (
        patch(
            "tools.atcoder_workflow.commands.detect_compiler", return_value=GCC15
        ),
        patch(
            "tools.atcoder_workflow.commands.compile_cpp", return_value=binary
        ) as compile_,
        patch(
            "tools.atcoder_workflow.commands.run_samples", return_value=0
        ) as samples,
    ):
        result = run_tests(CONTEXT, DEPENDENCIES, debug=True)

    assert result == 0
    assert compile_.call_args.kwargs["mode"] is BuildMode.DEBUG
    assert compile_.call_args.kwargs["output_path"] == binary
    samples.assert_called_once_with(
        binary, CONTEXT.test_dir, CONTEXT.task_dir, DEPENDENCIES.runner
    )


@pytest.mark.parametrize("command", ["run", "test"])
def test_failed_build_stops_before_later_stage(command: str) -> None:
    build_error = WorkflowError("compile failed")
    with (
        patch(
            "tools.atcoder_workflow.commands.run_build", side_effect=build_error
        ),
        patch("tools.atcoder_workflow.commands.run_binary") as run,
        patch("tools.atcoder_workflow.commands.run_samples") as samples,
    ):
        with pytest.raises(WorkflowError) as raised:
            if command == "run":
                run_program(CONTEXT, DEPENDENCIES)
            else:
                run_tests(CONTEXT, DEPENDENCIES)

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


def test_submit_runs_all_gates_in_order_with_verified_artifacts(
    capsys: pytest.CaptureFixture[str],
) -> None:
    events: list[str] = []
    dependencies = submit_dependencies(events)
    submission = CONTEXT.build_dir / "submission.cpp"
    binary = CONTEXT.build_dir / "submission-main"

    with patched_submit_stages(events) as (bundle, compile_, samples):
        result = run_submit(CONTEXT, dependencies)

    assert result == 0
    assert events == ["bundle", "compile", "samples", "prompt", "submit"]
    bundle.assert_called_once_with(
        source_path=CONTEXT.source_path,
        output_path=submission,
        working_dir=CONTEXT.task_dir,
        library_dir=CONTEXT.repository_root / "library",
        runner=dependencies.runner,
    )
    compile_.assert_called_once_with(
        source_path=submission,
        output_path=binary,
        working_dir=CONTEXT.task_dir,
        compiler=GCC15,
        mode=BuildMode.RELEASE,
        library_dir=CONTEXT.repository_root / "library",
        runner=dependencies.runner,
    )
    samples.assert_called_once_with(
        binary, CONTEXT.test_dir, CONTEXT.task_dir, dependencies.runner
    )
    assert capsys.readouterr().out == (
        "Contest: abc999\n"
        "Task: abc999_a\n"
        f"File: {submission}\n"
    )


@pytest.mark.parametrize("failed_stage", ["bundle", "compile", "samples"])
def test_submit_stops_after_each_failed_verification_gate(
    failed_stage: str,
) -> None:
    events: list[str] = []
    dependencies = submit_dependencies(events)
    bundle_error = WorkflowError("bundle failed") if failed_stage == "bundle" else None
    compile_error = (
        WorkflowError("compile failed") if failed_stage == "compile" else None
    )
    sample_returncode = 17 if failed_stage == "samples" else 0

    with patched_submit_stages(
        events,
        bundle_error=bundle_error,
        compile_error=compile_error,
        sample_returncode=sample_returncode,
    ):
        if failed_stage in {"bundle", "compile"}:
            with pytest.raises(WorkflowError, match=f"{failed_stage} failed"):
                run_submit(CONTEXT, dependencies)
        else:
            assert run_submit(CONTEXT, dependencies) == sample_returncode

    expected = {
        "bundle": ["bundle"],
        "compile": ["bundle", "compile"],
        "samples": ["bundle", "compile", "samples"],
    }
    assert events == expected[failed_stage]


def test_submit_rejects_non_tty_before_prompt_or_submission() -> None:
    events: list[str] = []
    dependencies = submit_dependencies(events, tty=False)

    with patched_submit_stages(events):
        with pytest.raises(WorkflowError, match="interactive terminal"):
            run_submit(CONTEXT, dependencies)

    assert events == ["bundle", "compile", "samples"]


@pytest.mark.parametrize("answer", ["", "n", "no", "anything else"])
def test_submit_defaults_to_no_and_rejects_other_answers(answer: str) -> None:
    events: list[str] = []
    dependencies = submit_dependencies(events, answer=answer)

    with patched_submit_stages(events):
        assert run_submit(CONTEXT, dependencies) == 1

    assert events == ["bundle", "compile", "samples", "prompt"]


@pytest.mark.parametrize("answer", ["y", "Y", "yes", "YES", "  YeS  "])
def test_submit_accepts_only_y_or_yes_case_insensitively(answer: str) -> None:
    events: list[str] = []
    dependencies = submit_dependencies(events, answer=answer)

    with patched_submit_stages(events):
        assert run_submit(CONTEXT, dependencies) == 0

    assert events == ["bundle", "compile", "samples", "prompt", "submit"]


def test_submit_uses_raw_acc_with_full_task_id_and_propagates_failure() -> None:
    events: list[str] = []
    dependencies = submit_dependencies(events, submit_returncode=42)

    with patched_submit_stages(events):
        result = run_submit(CONTEXT, dependencies)

    assert result == 42
    assert events == ["bundle", "compile", "samples", "prompt", "submit"]


def test_submit_has_no_fallback_when_raw_acc_is_unavailable() -> None:
    events: list[str] = []
    dependencies = submit_dependencies(events, raw_acc=None)

    with patched_submit_stages(events):
        with pytest.raises(WorkflowError, match="acc executable"):
            run_submit(CONTEXT, dependencies)

    assert events == ["bundle", "compile", "samples", "prompt"]


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

    with patched_submit_stages(events):
        result = run_submit(CONTEXT, dependencies)

    assert result == 31
    assert calls == [
        (
            [
                "/opt/homebrew/bin/acc",
                "submit",
                str(CONTEXT.build_dir / "submission.cpp"),
                "-c",
                "abc999",
                "-t",
                "abc999_a",
            ],
            CONTEXT.task_dir,
            False,
        )
    ]


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

    with patched_submit_stages(events):
        assert run_submit(CONTEXT, dependencies) == 1

    assert prompts == [
        f"Submit abc999_a from {CONTEXT.build_dir / 'submission.cpp'}? [y/N] "
    ]


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
