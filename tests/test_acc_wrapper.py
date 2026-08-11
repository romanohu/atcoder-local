from __future__ import annotations

import inspect
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile
from unittest.mock import patch

import pytest

from tools.acc_wrapper import console_main, main


def test_wrapper_supports_injected_context_and_collaborators() -> None:
    parameters = inspect.signature(main).parameters

    assert "cwd" in parameters
    assert "acc_runner" in parameters
    assert "workflow_runner" in parameters


def test_wrapper_has_no_feature_specific_operations_dependency() -> None:
    assert "operations" not in inspect.signature(main).parameters


def test_only_the_five_local_commands_use_workflow_dispatch() -> None:
    for command in ("build", "run", "test", "submit", "doctor"):
        native_calls: list[list[str]] = []
        workflow_calls: list[tuple[list[str], Path]] = []

        result = main(
            [command],
            cwd=Path("/repo"),
            acc_runner=lambda args: native_calls.append(args) or 19,
            workflow_runner=lambda args, cwd: workflow_calls.append((args, cwd))
            or 7,
        )

        assert result == 7
        assert native_calls == []
        assert workflow_calls == [([command], Path("/repo"))]


def test_config_delegates_once_and_returns_exact_acc_code() -> None:
    native_calls: list[list[str]] = []
    workflow_calls: list[list[str]] = []

    result = main(
        ["config", "default-task-choice"],
        cwd=Path("/repo"),
        acc_runner=lambda args: native_calls.append(args) or 29,
        workflow_runner=lambda args, cwd: workflow_calls.append(args) or 0,
    )

    assert result == 29
    assert native_calls == [["config", "default-task-choice"]]
    assert workflow_calls == []


def test_new_delegates_once_and_returns_exact_acc_code() -> None:
    native_calls: list[list[str]] = []
    workflow_calls: list[list[str]] = []

    result = main(
        ["new", "--template", "cpp", "abc454"],
        cwd=Path("/repo"),
        acc_runner=lambda args: native_calls.append(args) or 23,
        workflow_runner=lambda args, cwd: workflow_calls.append(args) or 0,
    )

    assert result == 23
    assert native_calls == [["new", "--template", "cpp", "abc454"]]
    assert workflow_calls == []


def test_default_raw_acc_delegation_uses_exact_command() -> None:
    completed = subprocess.CompletedProcess(args=[], returncode=17)

    with (
        patch(
            "tools.acc_wrapper.find_upstream_acc",
            return_value="/opt/native/bin/acc",
        ),
        patch("tools.acc_wrapper.subprocess.run", return_value=completed) as run,
    ):
        result = main(["new", "abc454"])

    assert result == 17
    run.assert_called_once_with(
        ["/opt/native/bin/acc", "new", "abc454"], check=False
    )


def test_default_raw_acc_delegation_reports_missing_upstream(
    capsys: pytest.CaptureFixture[str],
) -> None:
    with patch("tools.acc_wrapper.find_upstream_acc", return_value=None):
        result = main(["new", "abc454"])

    assert result == 1
    assert (
        capsys.readouterr().err
        == "[acc-wrapper] upstream acc executable was not found\n"
    )


def test_console_main_marks_installed_entry_before_dispatching() -> None:
    observed_marker: str | None = None

    def wrapped_main() -> int:
        nonlocal observed_marker
        observed_marker = os.environ.get("ATCODER_LOCAL_CONSOLE")
        return 23

    with (
        patch("tools.acc_wrapper.main", side_effect=wrapped_main) as main_call,
        pytest.raises(SystemExit) as raised,
    ):
        console_main()

    assert raised.value.code == 23
    assert observed_marker == "1"
    main_call.assert_called_once_with()


def test_console_main_prepends_runtime_bin_to_path(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    runtime_python = Path("/isolated/tool/bin/python")
    observed_path: str | None = None

    def wrapped_main() -> int:
        nonlocal observed_path
        observed_path = os.environ.get("PATH")
        return 0

    monkeypatch.setenv("PATH", os.pathsep.join(["/usr/bin", "/bin"]))
    monkeypatch.setattr(sys, "executable", str(runtime_python))
    with (
        patch("tools.acc_wrapper.main", side_effect=wrapped_main),
        pytest.raises(SystemExit) as raised,
    ):
        console_main()

    assert raised.value.code == 0
    assert observed_path == os.pathsep.join(
        [str(runtime_python.parent), "/usr/bin", "/bin"]
    )


def test_console_main_moves_existing_runtime_bin_to_front_once(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    runtime_python = Path("/isolated/tool/bin/python")
    observed_path: str | None = None

    def wrapped_main() -> int:
        nonlocal observed_path
        observed_path = os.environ.get("PATH")
        return 0

    monkeypatch.setenv(
        "PATH",
        os.pathsep.join(["/usr/bin", str(runtime_python.parent), "/bin"]),
    )
    monkeypatch.setattr(sys, "executable", str(runtime_python))
    with (
        patch("tools.acc_wrapper.main", side_effect=wrapped_main),
        pytest.raises(SystemExit),
    ):
        console_main()

    assert observed_path == os.pathsep.join(
        [str(runtime_python.parent), "/usr/bin", "/bin"]
    )


def test_absolute_script_execution_delegates_raw_acc() -> None:
    script_path = Path(__file__).parents[1] / "tools" / "acc_wrapper.py"
    with tempfile.TemporaryDirectory() as tmpdir:
        fake_acc = Path(tmpdir) / "acc"
        fake_acc.write_text(
            "#!/bin/sh\nprintf 'native-out\\n'\n"
            "printf 'native-err\\n' >&2\nexit 17\n",
            encoding="utf-8",
        )
        fake_acc.chmod(0o755)
        environment = dict(os.environ)
        environment["PATH"] = f"{tmpdir}{os.pathsep}{environment['PATH']}"

        completed = subprocess.run(
            [sys.executable, str(script_path), "config", "default-task-choice"],
            cwd=tmpdir,
            env=environment,
            check=False,
            capture_output=True,
            text=True,
        )

    assert completed.returncode == 17
    assert completed.stdout == "native-out\n"
    assert completed.stderr == "native-err\n"


def test_direct_script_doctor_rejects_unmarked_invocation() -> None:
    root = Path(__file__).parents[1]
    script_path = root / "tools" / "acc_wrapper.py"
    environment = dict(os.environ)
    environment.pop("ATCODER_LOCAL_CONSOLE", None)
    environment.pop("ATCODER_LOCAL_WRAPPER", None)

    completed = subprocess.run(
        [sys.executable, str(script_path), "doctor"],
        cwd=root,
        env=environment,
        check=False,
        capture_output=True,
        text=True,
    )

    assert completed.returncode == 1
    assert (
        "[error] shell-wrapper: current shell is not using the repository wrapper"
        in completed.stdout
    )


def test_bash_and_zsh_wrappers_dispatch_with_current_marker() -> None:
    root = Path(__file__).parents[1]
    cases = [
        ("bash", ["bash", "--noprofile", "--norc", "-c"]),
        ("zsh", ["zsh", "-f", "-c"]),
    ]

    for name, shell in cases:
        with tempfile.TemporaryDirectory() as tmpdir:
            temp = Path(tmpdir)
            fake_uv = temp / "uv"
            fake_uv.write_text(
                "#!/bin/sh\n"
                "printf 'marker=%s\\n' \"$ATCODER_LOCAL_WRAPPER\"\n"
                "printf 'raw=%s\\n' \"$ATCODER_LOCAL_RAW_ACC\"\n"
                "for arg in \"$@\"; do printf 'arg=%s\\n' \"$arg\"; done\n"
                "exit 37\n",
                encoding="utf-8",
            )
            fake_uv.chmod(0o755)
            fake_acc = temp / "acc"
            fake_acc.write_text("#!/bin/sh\nexit 0\n", encoding="utf-8")
            fake_acc.chmod(0o755)
            wrapper = root / "tools" / f"acc-wrapper.{name}"
            command = f"source {shlex.quote(str(wrapper))}; acc test --debug"
            environment = dict(os.environ)
            environment["PATH"] = f"{temp}{os.pathsep}{environment['PATH']}"

            completed = subprocess.run(
                [*shell, command],
                cwd=root,
                env=environment,
                check=False,
                capture_output=True,
                text=True,
            )

        assert completed.returncode == 37
        assert completed.stdout.splitlines() == [
            "marker=1",
            f"raw={fake_acc}",
            "arg=run",
            "arg=python",
            f"arg={root / 'tools' / 'acc_wrapper.py'}",
            "arg=test",
            "arg=--debug",
        ]
