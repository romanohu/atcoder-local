from __future__ import annotations

from collections.abc import Mapping, Sequence
from pathlib import Path
import subprocess
from unittest.mock import patch

import pytest

from tools.atcoder_workflow import WorkflowError
from tools.atcoder_workflow.cli import main
from tools.atcoder_workflow.commands import (
    WorkflowDependencies,
    run_build,
    run_program,
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
