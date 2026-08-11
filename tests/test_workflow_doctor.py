from __future__ import annotations

from collections.abc import Mapping, Sequence
from pathlib import Path
import subprocess
from types import SimpleNamespace
from unittest.mock import patch

import pytest

from tools.atcoder_workflow.commands import (
    CheckStatus,
    collect_doctor_checks,
    run_doctor,
)
from tools.atcoder_workflow.cli import main as workflow_main


ACL_COMMIT = "864245a00b00dd008d1abfdc239618fdb7d139da"
APPLE_CLANG17 = "Apple clang version 17.0.0 (clang-1700.0.13.5)"
GCC15 = "g++ (GCC) 15.2.0"


def _prepare_root(root: Path) -> None:
    acl = root / "library" / "atcoder"
    acl.mkdir(parents=True)
    (acl / "VERSION").write_text(f"v1.6\n{ACL_COMMIT}\n", encoding="utf-8")
    (acl / "LICENSE").write_text("ACL license\n", encoding="utf-8")
    (acl / "all").write_text("#pragma once\n", encoding="utf-8")

    local = root / "library" / "atcoder_local"
    local.mkdir(parents=True)
    for header in ("core.hpp", "io.hpp", "debug.hpp"):
        (local / header).write_text("#pragma once\n", encoding="utf-8")

    tools = root / "tools"
    tools.mkdir()
    for wrapper in ("acc-wrapper.bash", "acc-wrapper.zsh"):
        (tools / wrapper).write_text("acc() { :; }\n", encoding="utf-8")


def _dependencies(
    *,
    missing: set[str] | None = None,
    compiler: str = GCC15,
    marker: str | None = "1",
    installed_marker: str | None = None,
) -> SimpleNamespace:
    unavailable = set() if missing is None else missing
    executables = {
        "uv": "/usr/local/bin/uv",
        "acc": "/usr/local/bin/acc",
        "oj": "/usr/local/bin/oj",
        "oj-bundle": "/usr/local/bin/oj-bundle",
        "g++-15": "/opt/g++-15",
    }

    def which(name: str) -> str | None:
        if name in unavailable:
            return None
        return executables.get(name)

    def runner(
        argv: Sequence[str],
        *,
        cwd: Path | str | None = None,
        env: Mapping[str, str] | None = None,
        capture_output: bool = False,
    ) -> subprocess.CompletedProcess[str]:
        del cwd, env, capture_output
        if list(argv)[-1:] == ["--version"]:
            return subprocess.CompletedProcess(argv, 0, stdout=compiler, stderr="")
        return subprocess.CompletedProcess(argv, 0, stdout="", stderr="")

    environ = {} if marker is None else {"ATCODER_LOCAL_WRAPPER": marker}
    if installed_marker is not None:
        environ["ATCODER_LOCAL_CONSOLE"] = installed_marker
    return SimpleNamespace(
        runner=runner,
        environ=environ,
        which=which,
        input_fn=lambda prompt: prompt,
        stdin_isatty=lambda: True,
    )


def _check(root: Path, dependencies: SimpleNamespace, name: str):
    return next(
        item
        for item in collect_doctor_checks(root, dependencies)
        if item.name == name
    )


@pytest.mark.parametrize(
    ("missing_executable", "check_name"),
    [
        ("uv", "uv"),
        ("acc", "atcoder-cli"),
        ("oj", "oj"),
        ("oj-bundle", "oj-bundle"),
    ],
)
def test_doctor_errors_when_required_executable_is_missing(
    tmp_path: Path, missing_executable: str, check_name: str
) -> None:
    _prepare_root(tmp_path)

    check = _check(tmp_path, _dependencies(missing={missing_executable}), check_name)

    assert check.status is CheckStatus.ERROR


def test_doctor_errors_when_no_cpp23_compiler_is_available(tmp_path: Path) -> None:
    _prepare_root(tmp_path)

    check = _check(tmp_path, _dependencies(missing={"g++-15"}), "compiler")

    assert check.status is CheckStatus.ERROR


def test_doctor_warns_for_non_gcc15_compiler(tmp_path: Path) -> None:
    _prepare_root(tmp_path)
    dependencies = _dependencies(compiler=APPLE_CLANG17)
    dependencies.environ["CXX"] = "/usr/bin/clang++"

    check = _check(tmp_path, dependencies, "compiler")

    assert check.status is CheckStatus.WARNING


def test_doctor_errors_for_bad_acl_metadata(tmp_path: Path) -> None:
    _prepare_root(tmp_path)
    (tmp_path / "library" / "atcoder" / "VERSION").write_text(
        "v1.5\nunknown\n", encoding="utf-8"
    )

    check = _check(tmp_path, _dependencies(), "acl")

    assert check.status is CheckStatus.ERROR


def test_doctor_errors_when_a_local_header_is_missing(tmp_path: Path) -> None:
    _prepare_root(tmp_path)
    (tmp_path / "library" / "atcoder_local" / "debug.hpp").unlink()

    check = _check(tmp_path, _dependencies(), "acl")

    assert check.status is CheckStatus.ERROR


@pytest.mark.parametrize("wrapper", ["acc-wrapper.bash", "acc-wrapper.zsh"])
def test_doctor_errors_when_a_shell_wrapper_is_missing(
    tmp_path: Path, wrapper: str
) -> None:
    _prepare_root(tmp_path)
    (tmp_path / "tools" / wrapper).unlink()

    check = _check(tmp_path, _dependencies(), "shell-wrapper")

    assert check.status is CheckStatus.ERROR


def test_doctor_errors_when_current_shell_did_not_set_marker(tmp_path: Path) -> None:
    _prepare_root(tmp_path)

    check = _check(tmp_path, _dependencies(marker=None), "shell-wrapper")

    assert check.status is CheckStatus.ERROR


def test_doctor_accepts_installed_console_entry_marker(tmp_path: Path) -> None:
    _prepare_root(tmp_path)

    checks = collect_doctor_checks(
        tmp_path, _dependencies(marker=None, installed_marker="1")
    )

    assert all(check.status is CheckStatus.OK for check in checks)


def test_doctor_reports_all_healthy_checks_in_stable_order(tmp_path: Path) -> None:
    _prepare_root(tmp_path)

    checks = collect_doctor_checks(tmp_path, _dependencies())

    assert [check.name for check in checks] == [
        "uv",
        "atcoder-cli",
        "oj",
        "oj-bundle",
        "compiler",
        "acl",
        "shell-wrapper",
    ]
    assert all(check.status is CheckStatus.OK for check in checks)


def test_run_doctor_prints_every_check_and_fails_on_error(
    tmp_path: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    _prepare_root(tmp_path)

    result = run_doctor(tmp_path, _dependencies(missing={"oj"}))

    lines = capsys.readouterr().out.splitlines()
    assert result == 1
    assert len(lines) == 7
    assert any(line.startswith("[error] oj:") for line in lines)


def test_run_doctor_succeeds_with_warning_only(
    tmp_path: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    _prepare_root(tmp_path)
    dependencies = _dependencies(compiler=APPLE_CLANG17)
    dependencies.environ["CXX"] = "/usr/bin/clang++"

    result = run_doctor(tmp_path, dependencies)

    assert result == 0
    assert "[warning] compiler:" in capsys.readouterr().out


def test_cli_doctor_uses_nearest_repository_root_from_nested_directory(
    tmp_path: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    (tmp_path / ".git").mkdir()
    root = tmp_path / "nearest-repository"
    root.mkdir()
    (root / ".git").mkdir()
    _prepare_root(root)
    nested = root / "contests" / "abc999" / "a"
    nested.mkdir(parents=True)
    dependencies = _dependencies()
    with patch("tools.atcoder_workflow.cli.resolve_task_context") as resolve:
        result = workflow_main(["doctor"], cwd=nested, dependencies=dependencies)

    output = capsys.readouterr()
    assert result == 0
    assert len(output.out.splitlines()) == 7
    assert all(line.startswith("[ok]") for line in output.out.splitlines())
    assert output.err == ""
    resolve.assert_not_called()


def test_cli_doctor_reports_no_repository_as_workflow_error(
    tmp_path: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    with patch("tools.atcoder_workflow.cli.resolve_task_context") as resolve:
        result = workflow_main(
            ["doctor"], cwd=tmp_path, dependencies=_dependencies()
        )

    output = capsys.readouterr()
    assert result == 1
    assert output.out == ""
    assert output.err == "[acc-wrapper] not inside a Git repository\n"
    resolve.assert_not_called()
