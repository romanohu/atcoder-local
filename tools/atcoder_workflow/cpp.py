from __future__ import annotations

from pathlib import Path
import shlex

from . import WorkflowError
from .compiler import BuildMode, CompilerInfo, compiler_flags
from .runner import ProcessRunner


def compile_cpp(
    *,
    source_path: Path,
    output_path: Path,
    working_dir: Path,
    compiler: CompilerInfo,
    mode: BuildMode,
    library_dir: Path,
    runner: ProcessRunner,
) -> Path:
    output_path.parent.mkdir(parents=True, exist_ok=True)
    temporary = output_path.with_name(f".{output_path.name}.tmp")
    temporary.unlink(missing_ok=True)
    result = runner(
        [
            compiler.executable,
            *compiler_flags(mode, library_dir),
            str(source_path),
            "-o",
            str(temporary),
        ],
        cwd=working_dir,
    )
    if result.returncode != 0:
        temporary.unlink(missing_ok=True)
        raise WorkflowError(f"compile failed for {source_path}")
    temporary.replace(output_path)
    return output_path


def bundle_cpp(
    *,
    source_path: Path,
    output_path: Path,
    working_dir: Path,
    library_dir: Path,
    runner: ProcessRunner,
) -> Path:
    output_path.parent.mkdir(parents=True, exist_ok=True)
    temporary = output_path.with_name(f".{output_path.name}.tmp")
    temporary.unlink(missing_ok=True)
    result = runner(
        ["oj-bundle", "-I", str(library_dir), str(source_path)],
        cwd=working_dir,
        capture_output=True,
    )
    if result.returncode != 0:
        temporary.unlink(missing_ok=True)
        raise WorkflowError(f"bundle failed for {source_path}")
    temporary.write_text(result.stdout, encoding="utf-8")
    temporary.replace(output_path)
    return output_path


def run_binary(binary_path: Path, working_dir: Path, runner: ProcessRunner) -> int:
    result = runner([str(binary_path)], cwd=working_dir)
    return result.returncode


def run_samples(
    binary_path: Path,
    test_dir: Path,
    working_dir: Path,
    runner: ProcessRunner,
) -> int:
    result = runner(
        [
            "oj",
            "test",
            "-c",
            shlex.quote(str(binary_path)),
            "-d",
            str(test_dir),
        ],
        cwd=working_dir,
    )
    return result.returncode
