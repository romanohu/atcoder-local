from __future__ import annotations

from collections.abc import Callable, Mapping
from pathlib import Path
import subprocess
import sys

import pytest

from tools.atcoder_workflow import WorkflowError
from tools.atcoder_workflow.compiler import (
    BuildMode,
    CompilerFamily,
    compiler_flags,
    detect_compiler,
)
from tools.atcoder_workflow.runner import run_process


def fake_compiler_runner(
    versions: Mapping[str, str], *, failed_probes: set[str] | None = None
) -> Callable[..., subprocess.CompletedProcess[str]]:
    failed_probes = failed_probes or set()

    def run(
        argv: list[str],
        *,
        cwd: Path | str | None = None,
        env: Mapping[str, str] | None = None,
        capture_output: bool = False,
    ) -> subprocess.CompletedProcess[str]:
        del cwd, env, capture_output
        executable = argv[0]
        if argv[1:] == ["--version"]:
            if executable in versions:
                return subprocess.CompletedProcess(argv, 0, versions[executable], "")
            return subprocess.CompletedProcess(argv, 127, "", "not found")
        return subprocess.CompletedProcess(
            argv, 1 if executable in failed_probes else 0, "", "unsupported"
        )

    return run


def fake_which(paths: Mapping[str, str]) -> Callable[[str], str | None]:
    return lambda name: paths.get(name)


def test_run_process_uses_argv_and_returns_text_output() -> None:
    result = run_process(
        [sys.executable, "-c", "import sys; print(sys.argv[1])", "literal value"],
        capture_output=True,
    )

    assert result.returncode == 0
    assert result.stdout == "literal value\n"


def test_explicit_cxx_is_one_executable_token_and_wins_over_gcc15() -> None:
    explicit_cxx = "/custom/clang++ --not-a-separate-argument"
    info = detect_compiler(
        environ={"CXX": explicit_cxx},
        runner=fake_compiler_runner({explicit_cxx: "Apple clang version 17.0.0"}),
        which=fake_which({"g++-15": "/opt/g++-15"}),
    )

    assert info.executable == explicit_cxx
    assert info.family is CompilerFamily.CLANG


def test_verified_gcc15_wins_over_apple_gpp() -> None:
    versions = {
        "/opt/g++-15": "g++ (GCC) 15.2.0",
        "/usr/bin/g++": "Apple clang version 17.0.0",
    }
    info = detect_compiler(
        environ={},
        runner=fake_compiler_runner(versions),
        which=fake_which({"g++-15": "/opt/g++-15", "g++": "/usr/bin/g++"}),
    )

    assert (info.executable, info.major) == ("/opt/g++-15", 15)


def test_broken_explicit_cxx_raises_workflow_error() -> None:
    with pytest.raises(WorkflowError):
        detect_compiler(
            environ={"CXX": "missing-cxx"},
            runner=fake_compiler_runner({}),
            which=fake_which({"g++-15": "/opt/g++-15"}),
        )


def test_explicit_compiler_that_cannot_compile_cpp23_raises_workflow_error() -> None:
    with pytest.raises(WorkflowError):
        detect_compiler(
            environ={"CXX": "/custom/g++"},
            runner=fake_compiler_runner(
                {"/custom/g++": "g++ (GCC) 15.2.0"},
                failed_probes={"/custom/g++"},
            ),
            which=fake_which({}),
        )


def test_auto_candidate_that_cannot_compile_cpp23_is_skipped() -> None:
    info = detect_compiler(
        environ={},
        runner=fake_compiler_runner(
            {
                "/opt/g++-15": "g++ (GCC) 15.2.0",
                "/opt/clang++": "clang version 18.1.0",
            },
            failed_probes={"/opt/g++-15"},
        ),
        which=fake_which({"g++-15": "/opt/g++-15", "clang++": "/opt/clang++"}),
    )

    assert info.executable == "/opt/clang++"


def test_missing_compiler_raises_workflow_error() -> None:
    with pytest.raises(WorkflowError):
        detect_compiler(
            environ={}, runner=fake_compiler_runner({}), which=fake_which({})
        )


def test_release_flags_are_exact() -> None:
    assert compiler_flags(BuildMode.RELEASE, Path("/repo/library")) == [
        "-std=gnu++23",
        "-O2",
        "-Wall",
        "-Wextra",
        "-DONLINE_JUDGE",
        "-DATCODER",
        "-I",
        "/repo/library",
    ]


def test_debug_flags_are_exact_and_exclude_online_judge() -> None:
    flags = compiler_flags(BuildMode.DEBUG, Path("/repo/library"))

    assert flags == [
        "-std=gnu++23",
        "-O0",
        "-g",
        "-Wall",
        "-Wextra",
        "-DATCODER",
        "-DLOCAL",
        "-fsanitize=address,undefined",
        "-fno-omit-frame-pointer",
        "-I",
        "/repo/library",
    ]
    assert "-DONLINE_JUDGE" not in flags
