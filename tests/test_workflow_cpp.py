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


def test_bundle_rejects_unbundled_local_include_and_preserves_old_output(
    tmp_path: Path,
) -> None:
    source = tmp_path / "main.cpp"
    source.write_text("int main() {}\n", encoding="utf-8")
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
        return subprocess.CompletedProcess(
            argv,
            0,
            '#line 1 "main.cpp"\n#include <atcoder_local/core.hpp>\nint main() {}\n',
            "",
        )

    with pytest.raises(WorkflowError, match="unbundled local include.*use quotes"):
        bundle_cpp(
            source_path=source,
            output_path=output,
            working_dir=tmp_path,
            library_dir=tmp_path / "library",
            runner=runner,
        )

    assert output.read_text(encoding="utf-8") == "old bundle"
    assert not temporary.exists()


@pytest.mark.parametrize("operation", ["compile", "bundle"])
def test_spawn_oserror_is_normalized_and_cleans_temporary(
    tmp_path: Path, operation: str
) -> None:
    source = tmp_path / "main.cpp"
    source.write_text("int main() {}\n", encoding="utf-8")
    output_name = "main" if operation == "compile" else "bundled.cpp"
    output = tmp_path / f".atcoder-local/build/abc999/a/{output_name}"
    output.parent.mkdir(parents=True)
    output.write_text("old output", encoding="utf-8")
    temporary = output.with_name(f".{output.name}.tmp")
    spawn_error = OSError("cannot spawn")

    def runner(
        argv: Sequence[str],
        *,
        cwd: Path | str | None = None,
        env: Mapping[str, str] | None = None,
        capture_output: bool = False,
    ) -> subprocess.CompletedProcess[str]:
        del argv, cwd, env, capture_output
        if operation == "compile":
            temporary.write_text("partial output", encoding="utf-8")
        raise spawn_error

    with pytest.raises(WorkflowError) as raised:
        if operation == "compile":
            compile_cpp(
                source_path=source,
                output_path=output,
                working_dir=tmp_path,
                compiler=GCC15,
                mode=BuildMode.RELEASE,
                library_dir=tmp_path / "library",
                runner=runner,
            )
        else:
            temporary.write_text("stale temporary", encoding="utf-8")
            bundle_cpp(
                source_path=source,
                output_path=output,
                working_dir=tmp_path,
                library_dir=tmp_path / "library",
                runner=runner,
            )

    assert raised.value.__cause__ is spawn_error
    assert output.read_text(encoding="utf-8") == "old output"
    assert not temporary.exists()


def test_bundle_write_oserror_is_normalized_and_removes_partial_temporary(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    source = tmp_path / "main.cpp"
    source.write_text("int main() {}\n", encoding="utf-8")
    output = tmp_path / ".atcoder-local/build/abc999/a/bundled.cpp"
    output.parent.mkdir(parents=True)
    output.write_text("old bundle", encoding="utf-8")
    temporary = output.with_name(".bundled.cpp.tmp")
    write_error = OSError("disk full")
    original_write_text = Path.write_text

    def failing_write_text(
        path: Path, data: str, encoding: str | None = None
    ) -> int:
        written = original_write_text(path, data, encoding=encoding)
        if path == temporary:
            raise write_error
        return written

    monkeypatch.setattr(Path, "write_text", failing_write_text)

    def runner(
        argv: Sequence[str],
        *,
        cwd: Path | str | None = None,
        env: Mapping[str, str] | None = None,
        capture_output: bool = False,
    ) -> subprocess.CompletedProcess[str]:
        del cwd, env, capture_output
        return subprocess.CompletedProcess(argv, 0, "partial bundle", "")

    with pytest.raises(WorkflowError) as raised:
        bundle_cpp(
            source_path=source,
            output_path=output,
            working_dir=tmp_path,
            library_dir=tmp_path / "library",
            runner=runner,
        )

    assert raised.value.__cause__ is write_error
    assert output.read_text(encoding="utf-8") == "old bundle"
    assert not temporary.exists()


@pytest.mark.parametrize("operation", ["compile", "bundle"])
def test_replace_oserror_is_normalized_and_cleans_temporary(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, operation: str
) -> None:
    source = tmp_path / "main.cpp"
    source.write_text("int main() {}\n", encoding="utf-8")
    output_name = "main" if operation == "compile" else "bundled.cpp"
    output = tmp_path / f".atcoder-local/build/abc999/a/{output_name}"
    output.parent.mkdir(parents=True)
    output.write_text("old output", encoding="utf-8")
    temporary = output.with_name(f".{output.name}.tmp")
    replace_error = OSError("replace blocked")

    def failing_replace(path: Path, target: Path) -> Path:
        assert path == temporary
        assert target == output
        raise replace_error

    monkeypatch.setattr(Path, "replace", failing_replace)

    def runner(
        argv: Sequence[str],
        *,
        cwd: Path | str | None = None,
        env: Mapping[str, str] | None = None,
        capture_output: bool = False,
    ) -> subprocess.CompletedProcess[str]:
        del cwd, env, capture_output
        if operation == "compile":
            temporary.write_text("new binary", encoding="utf-8")
            return subprocess.CompletedProcess(argv, 0, "", "")
        return subprocess.CompletedProcess(argv, 0, "new bundle", "")

    with pytest.raises(WorkflowError) as raised:
        if operation == "compile":
            compile_cpp(
                source_path=source,
                output_path=output,
                working_dir=tmp_path,
                compiler=GCC15,
                mode=BuildMode.RELEASE,
                library_dir=tmp_path / "library",
                runner=runner,
            )
        else:
            bundle_cpp(
                source_path=source,
                output_path=output,
                working_dir=tmp_path,
                library_dir=tmp_path / "library",
                runner=runner,
            )

    assert raised.value.__cause__ is replace_error
    assert output.read_text(encoding="utf-8") == "old output"
    assert not temporary.exists()


@pytest.mark.parametrize(
    ("operation", "control_flow"),
    [("compile", KeyboardInterrupt()), ("bundle", SystemExit(9))],
)
def test_control_flow_exceptions_propagate_after_temporary_cleanup(
    tmp_path: Path, operation: str, control_flow: BaseException
) -> None:
    source = tmp_path / "main.cpp"
    source.write_text("int main() {}\n", encoding="utf-8")
    output_name = "main" if operation == "compile" else "bundled.cpp"
    output = tmp_path / f".atcoder-local/build/abc999/a/{output_name}"
    output.parent.mkdir(parents=True)
    output.write_text("old output", encoding="utf-8")
    temporary = output.with_name(f".{output.name}.tmp")

    def runner(
        argv: Sequence[str],
        *,
        cwd: Path | str | None = None,
        env: Mapping[str, str] | None = None,
        capture_output: bool = False,
    ) -> subprocess.CompletedProcess[str]:
        del argv, cwd, env, capture_output
        temporary.write_text("partial output", encoding="utf-8")
        raise control_flow

    with pytest.raises(type(control_flow)) as raised:
        if operation == "compile":
            compile_cpp(
                source_path=source,
                output_path=output,
                working_dir=tmp_path,
                compiler=GCC15,
                mode=BuildMode.RELEASE,
                library_dir=tmp_path / "library",
                runner=runner,
            )
        else:
            bundle_cpp(
                source_path=source,
                output_path=output,
                working_dir=tmp_path,
                library_dir=tmp_path / "library",
                runner=runner,
            )

    assert raised.value is control_flow
    assert output.read_text(encoding="utf-8") == "old output"
    assert not temporary.exists()


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


@pytest.mark.parametrize(
    ("operation", "expected_message"),
    [
        ("run", "run failed for /repo/.atcoder-local/build/abc999/a/main"),
        (
            "samples",
            "sample test failed for /repo/.atcoder-local/build/abc999/a/main",
        ),
    ],
)
def test_execution_spawn_oserror_is_normalized_with_binary_path(
    operation: str, expected_message: str
) -> None:
    binary_path = Path("/repo/.atcoder-local/build/abc999/a/main")
    spawn_error = OSError("cannot spawn")

    def runner(
        argv: Sequence[str],
        *,
        cwd: Path | str | None = None,
        env: Mapping[str, str] | None = None,
        capture_output: bool = False,
    ) -> subprocess.CompletedProcess[str]:
        del argv, cwd, env, capture_output
        raise spawn_error

    with pytest.raises(WorkflowError) as raised:
        if operation == "run":
            run_binary(binary_path, Path("/repo/contests/abc999/a"), runner)
        else:
            run_samples(
                binary_path,
                Path("/repo/contests/abc999/a/test"),
                Path("/repo/contests/abc999/a"),
                runner,
            )

    assert str(raised.value) == expected_message
    assert raised.value.__cause__ is spawn_error


@pytest.mark.parametrize(
    ("operation", "control_flow"),
    [("run", KeyboardInterrupt()), ("samples", SystemExit(9))],
)
def test_execution_control_flow_exceptions_propagate(
    operation: str, control_flow: BaseException
) -> None:
    binary_path = Path("/repo/.atcoder-local/build/abc999/a/main")

    def runner(
        argv: Sequence[str],
        *,
        cwd: Path | str | None = None,
        env: Mapping[str, str] | None = None,
        capture_output: bool = False,
    ) -> subprocess.CompletedProcess[str]:
        del argv, cwd, env, capture_output
        raise control_flow

    with pytest.raises(type(control_flow)) as raised:
        if operation == "run":
            run_binary(binary_path, Path("/repo/contests/abc999/a"), runner)
        else:
            run_samples(
                binary_path,
                Path("/repo/contests/abc999/a/test"),
                Path("/repo/contests/abc999/a"),
                runner,
            )

    assert raised.value is control_flow
