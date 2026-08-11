from __future__ import annotations

import os
from pathlib import Path
import subprocess

import pytest

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


def npm_search_bin(tmp_path: Path) -> Path:
    search_bin = tmp_path / "search-bin"
    search_bin.mkdir()
    executable(search_bin / "npm")
    return search_bin


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


def test_finds_npm_global_acc_when_global_bin_is_not_on_path(
    tmp_path: Path,
) -> None:
    active = installed_wrapper(tmp_path / "uv-tool/bin/acc")
    prefix = tmp_path / "npm-global"
    upstream = executable(prefix / "bin/acc")
    search_bin = tmp_path / "search-bin"
    search_bin.mkdir()
    npm = search_bin / "npm"
    npm.write_text(
        "#!/bin/sh\n"
        "test \"$1 $2\" = \"prefix --global\" || exit 9\n"
        f"printf '%s\\n' '{prefix}'\n",
        encoding="utf-8",
    )
    npm.chmod(0o755)

    assert find_upstream_acc(
        {"PATH": str(search_bin)}, active
    ) == str(upstream.resolve())


@pytest.mark.parametrize(
    ("output", "exit_code"),
    [("", 0), ("relative-prefix", 0), ("/unused", 9)],
)
def test_ignores_invalid_npm_global_prefix(
    tmp_path: Path, output: str, exit_code: int
) -> None:
    active = installed_wrapper(tmp_path / "uv-tool/bin/acc")
    search_bin = tmp_path / "search-bin"
    search_bin.mkdir()
    npm = search_bin / "npm"
    npm.write_text(
        "#!/bin/sh\n"
        f"printf '%s\\n' '{output}'\n"
        f"exit {exit_code}\n",
        encoding="utf-8",
    )
    npm.chmod(0o755)

    assert find_upstream_acc({"PATH": str(search_bin)}, active) is None


@pytest.mark.parametrize(
    "output",
    ["/tmp/npm\0prefix", "~atcoder-local-no-such-user"],
    ids=["embedded-nul", "unknown-user"],
)
def test_ignores_malformed_npm_global_prefix(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
    output: str,
) -> None:
    active = installed_wrapper(tmp_path / "uv-tool/bin/acc")
    search_bin = npm_search_bin(tmp_path)
    completed = subprocess.CompletedProcess(
        args=[], returncode=0, stdout=output, stderr=""
    )
    monkeypatch.setattr(subprocess, "run", lambda *args, **kwargs: completed)

    assert find_upstream_acc({"PATH": str(search_bin)}, active) is None


def test_ignores_undecodable_npm_global_prefix(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    active = installed_wrapper(tmp_path / "uv-tool/bin/acc")
    search_bin = npm_search_bin(tmp_path)
    decoding_error = UnicodeDecodeError(
        "utf-8", b"\xff", 0, 1, "invalid start byte"
    )

    def fail_to_decode(*args: object, **kwargs: object) -> None:
        del args, kwargs
        raise decoding_error

    monkeypatch.setattr(subprocess, "run", fail_to_decode)

    assert find_upstream_acc({"PATH": str(search_bin)}, active) is None


@pytest.mark.parametrize(
    "error",
    [subprocess.TimeoutExpired(cmd="npm", timeout=5), OSError("cannot execute")],
    ids=["timeout", "os-error"],
)
def test_ignores_npm_execution_failure(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
    error: BaseException,
) -> None:
    active = installed_wrapper(tmp_path / "uv-tool/bin/acc")
    search_bin = npm_search_bin(tmp_path)

    def fail_to_run(*args: object, **kwargs: object) -> None:
        del args, kwargs
        raise error

    monkeypatch.setattr(subprocess, "run", fail_to_run)

    assert find_upstream_acc({"PATH": str(search_bin)}, active) is None


def test_ignores_npm_candidate_resolution_oserror(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    active = installed_wrapper(tmp_path / "uv-tool/bin/acc")
    search_bin = npm_search_bin(tmp_path)
    prefix = tmp_path / "npm-global"
    completed = subprocess.CompletedProcess(
        args=[], returncode=0, stdout=str(prefix), stderr=""
    )
    original_resolve = Path.resolve

    def resolve(path: Path, *args: object, **kwargs: object) -> Path:
        if path == prefix / "bin" / "acc":
            raise OSError("cannot resolve candidate")
        return original_resolve(path, *args, **kwargs)

    monkeypatch.setattr(subprocess, "run", lambda *args, **kwargs: completed)
    monkeypatch.setattr(Path, "resolve", resolve)

    assert find_upstream_acc({"PATH": str(search_bin)}, active) is None
