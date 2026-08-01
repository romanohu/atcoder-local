from __future__ import annotations

from collections.abc import Callable, Mapping, Sequence
from pathlib import Path
import subprocess

import pytest

from tools.atcoder_workflow import WorkflowError
from tools.atcoder_workflow.compiler import BuildMode, CompilerFamily, CompilerInfo
from tools.atcoder_workflow.cpp import (
    bundle_cpp,
    compile_cpp,
    run_binary,
    run_samples,
)


GCC15 = CompilerInfo(
    executable="/opt/g++-15",
    family=CompilerFamily.GCC,
    major=15,
    version_text="g++ (GCC) 15.2.0",
)


def compile_runner(
    *, returncode: int, output_text: str = "new binary"
) -> Callable[..., subprocess.CompletedProcess[str]]:
    def run(
        argv: Sequence[str],
        *,
        cwd: Path | str | None = None,
        env: Mapping[str, str] | None = None,
        capture_output: bool = False,
    ) -> subprocess.CompletedProcess[str]:
        del cwd, env, capture_output
        Path(argv[-1]).write_text(output_text, encoding="utf-8")
        return subprocess.CompletedProcess(argv, returncode, "", "compile error")

    return run


def test_failed_compile_preserves_old_output_and_removes_temporary(
    tmp_path: Path,
) -> None:
    source = tmp_path / "main.cpp"
    source.write_text("int main() {}\n", encoding="utf-8")
    output = tmp_path / ".atcoder-local/build/abc999/a/main"
    output.parent.mkdir(parents=True)
    output.write_text("old binary", encoding="utf-8")

    with pytest.raises(WorkflowError, match="compile failed"):
        compile_cpp(
            source_path=source,
            output_path=output,
            working_dir=tmp_path,
            compiler=GCC15,
            mode=BuildMode.RELEASE,
            library_dir=tmp_path / "library",
            runner=compile_runner(returncode=1, output_text="partial binary"),
        )

    assert output.read_text(encoding="utf-8") == "old binary"
    assert not output.with_name(".main.tmp").exists()


def test_successful_compile_atomically_replaces_output(tmp_path: Path) -> None:
    source = tmp_path / "main.cpp"
    source.write_text("int main() {}\n", encoding="utf-8")
    output = tmp_path / ".atcoder-local/build/abc999/a/main"
    output.parent.mkdir(parents=True)
    output.write_text("old binary", encoding="utf-8")
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
        Path(argv[-1]).write_text("new binary", encoding="utf-8")
        return subprocess.CompletedProcess(argv, 0, "", "")

    result = compile_cpp(
        source_path=source,
        output_path=output,
        working_dir=tmp_path,
        compiler=GCC15,
        mode=BuildMode.RELEASE,
        library_dir=tmp_path / "library",
        runner=runner,
    )

    temporary = output.with_name(".main.tmp")
    assert result == output
    assert output.read_text(encoding="utf-8") == "new binary"
    assert not temporary.exists()
    assert calls == [
        (
            [
                "/opt/g++-15",
                "-std=gnu++23",
                "-O2",
                "-Wall",
                "-Wextra",
                "-DONLINE_JUDGE",
                "-DATCODER",
                "-I",
                str(tmp_path / "library"),
                str(source),
                "-o",
                str(temporary),
            ],
            tmp_path,
            False,
        )
    ]


def test_failed_bundle_preserves_old_output_and_removes_temporary(
    tmp_path: Path,
) -> None:
    source = tmp_path / "main.cpp"
    source.write_text('#include "library.hpp"\n', encoding="utf-8")
    output = tmp_path / ".atcoder-local/build/abc999/a/bundled.cpp"
    output.parent.mkdir(parents=True)
    output.write_text("old bundle", encoding="utf-8")
    temporary = output.with_name(".bundled.cpp.tmp")
    temporary.write_text("stale temporary", encoding="utf-8")

    def runner(
        argv: Sequence[str],
        *,
        cwd: Path | str | None = None,
        env: Mapping[str, str] | None = None,
        capture_output: bool = False,
    ) -> subprocess.CompletedProcess[str]:
        del cwd, env, capture_output
        return subprocess.CompletedProcess(argv, 1, "partial bundle", "bundle error")

    with pytest.raises(WorkflowError, match="bundle failed"):
        bundle_cpp(
            source_path=source,
            output_path=output,
            working_dir=tmp_path,
            library_dir=tmp_path / "library",
            runner=runner,
        )

    assert output.read_text(encoding="utf-8") == "old bundle"
    assert not temporary.exists()


def test_successful_bundle_atomically_replaces_output(tmp_path: Path) -> None:
    source = tmp_path / "main.cpp"
    source.write_text('#include "library.hpp"\n', encoding="utf-8")
    output = tmp_path / ".atcoder-local/build/abc999/a/bundled.cpp"
    output.parent.mkdir(parents=True)
    output.write_text("old bundle", encoding="utf-8")
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
        return subprocess.CompletedProcess(argv, 0, "expanded source\n", "")

    result = bundle_cpp(
        source_path=source,
        output_path=output,
        working_dir=tmp_path,
        library_dir=tmp_path / "library",
        runner=runner,
    )

    assert result == output
    assert output.read_text(encoding="utf-8") == "expanded source\n"
    assert not output.with_name(".bundled.cpp.tmp").exists()
    assert calls == [
        (
            ["oj-bundle", "-I", str(tmp_path / "library"), str(source)],
            tmp_path,
            True,
        )
    ]


def test_run_binary_uses_one_argv_token_and_inherits_stdio() -> None:
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
        return subprocess.CompletedProcess(argv, 7, "", "")

    returncode = run_binary(
        Path("/tmp/AtCoder build/main"), Path("/repo/contest/a"), runner
    )

    assert returncode == 7
    assert calls == [
        (["/tmp/AtCoder build/main"], Path("/repo/contest/a"), False)
    ]


def test_run_samples_uses_exact_oj_test_arguments() -> None:
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
        return subprocess.CompletedProcess(argv, 3, "", "")

    returncode = run_samples(
        Path("/tmp/AtCoder build/main"),
        Path("/repo/contest/a/test"),
        Path("/repo/contest/a"),
        runner,
    )

    assert returncode == 3
    assert calls == [
        (
            [
                "oj",
                "test",
                "-c",
                "'/tmp/AtCoder build/main'",
                "-d",
                "/repo/contest/a/test",
            ],
            Path("/repo/contest/a"),
            False,
        )
    ]
