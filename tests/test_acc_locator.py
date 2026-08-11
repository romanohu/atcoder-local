from __future__ import annotations

import os
from pathlib import Path

from tools.atcoder_workflow.acc_locator import find_upstream_acc


def executable(path: Path) -> Path:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("#!/bin/sh\nexit 0\n", encoding="utf-8")
    path.chmod(0o755)
    return path


def installed_wrapper(path: Path) -> Path:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        "#!/usr/bin/env python3\n"
        "import sys\n"
        "from tools.acc_wrapper import console_main\n"
        "if __name__ == '__main__':\n"
        "    sys.exit(console_main())\n",
        encoding="utf-8",
    )
    path.chmod(0o755)
    return path


def test_skips_active_wrapper_and_returns_next_acc(tmp_path: Path) -> None:
    wrapper = executable(tmp_path / "wrapper/acc")
    upstream = executable(tmp_path / "upstream/acc")
    environ = {"PATH": os.pathsep.join([str(wrapper.parent), str(upstream.parent)])}

    assert find_upstream_acc(environ, wrapper) == str(upstream.resolve())


def test_skips_all_installed_wrappers_before_native_acc(tmp_path: Path) -> None:
    active = installed_wrapper(tmp_path / "venv/bin/acc")
    other_wrapper = installed_wrapper(tmp_path / "uv-tool/bin/acc")
    upstream = executable(tmp_path / "npm/bin/acc")
    environ = {
        "PATH": os.pathsep.join(
            [str(active.parent), str(other_wrapper.parent), str(upstream.parent)]
        )
    }

    assert find_upstream_acc(environ, active) == str(upstream.resolve())


def test_prefers_valid_shell_override(tmp_path: Path) -> None:
    upstream = executable(tmp_path / "upstream/acc")

    assert find_upstream_acc(
        {"PATH": "", "ATCODER_LOCAL_RAW_ACC": str(upstream)}
    ) == str(upstream.resolve())


def test_rejects_installed_wrapper_override_and_finds_native_acc(
    tmp_path: Path,
) -> None:
    active = installed_wrapper(tmp_path / "venv/bin/acc")
    other_wrapper = installed_wrapper(tmp_path / "uv-tool/bin/acc")
    upstream = executable(tmp_path / "npm/bin/acc")
    environ = {
        "PATH": os.pathsep.join(
            [str(active.parent), str(other_wrapper.parent), str(upstream.parent)]
        ),
        "ATCODER_LOCAL_RAW_ACC": str(other_wrapper),
    }

    assert find_upstream_acc(environ, active) == str(upstream.resolve())


def test_falls_back_when_override_is_missing(tmp_path: Path) -> None:
    upstream = executable(tmp_path / "upstream/acc")
    environ = {
        "PATH": str(upstream.parent),
        "ATCODER_LOCAL_RAW_ACC": str(tmp_path / "missing"),
    }

    assert find_upstream_acc(environ) == str(upstream.resolve())


def test_returns_none_without_distinct_acc(tmp_path: Path) -> None:
    wrapper = executable(tmp_path / "wrapper/acc")

    assert find_upstream_acc({"PATH": str(wrapper.parent)}, wrapper) is None
