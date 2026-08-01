from __future__ import annotations

from collections.abc import Mapping
from pathlib import Path
import re
import shlex

from . import WorkflowError
from .compiler import BuildMode, CompilerInfo, compiler_flags
from .runner import ProcessRunner


UNBUNDLED_LOCAL_INCLUDE = re.compile(
    r"^\s*#\s*include\s*<atcoder_local/[^>]+>", re.MULTILINE
)


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
    temporary = output_path.with_name(f".{output_path.name}.tmp")
    try:
        output_path.parent.mkdir(parents=True, exist_ok=True)
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
            raise WorkflowError(f"compile failed for {source_path}")
        temporary.replace(output_path)
    except OSError as error:
        raise WorkflowError(f"compile failed for {source_path}") from error
    finally:
        _discard_temporary(temporary)
    return output_path


def bundle_cpp(
    *,
    source_path: Path,
    output_path: Path,
    working_dir: Path,
    library_dir: Path,
    runner: ProcessRunner,
    environment: Mapping[str, str] | None = None,
) -> Path:
    temporary = output_path.with_name(f".{output_path.name}.tmp")
    try:
        output_path.parent.mkdir(parents=True, exist_ok=True)
        temporary.unlink(missing_ok=True)
        result = runner(
            ["oj-bundle", "-I", str(library_dir), str(source_path)],
            cwd=working_dir,
            env=environment,
            capture_output=True,
        )
        if result.returncode != 0:
            raise WorkflowError(f"bundle failed for {source_path}")
        _reject_unbundled_local_includes(result.stdout, source_path)
        temporary.write_text(result.stdout, encoding="utf-8")
        temporary.replace(output_path)
    except OSError as error:
        raise WorkflowError(f"bundle failed for {source_path}") from error
    finally:
        _discard_temporary(temporary)
    return output_path


def _reject_unbundled_local_includes(source: str, source_path: Path) -> None:
    if UNBUNDLED_LOCAL_INCLUDE.search(source):
        raise WorkflowError(
            f"bundle left an unbundled local include in {source_path}; "
            "use quotes for atcoder_local headers, for example "
            '#include "atcoder_local/core.hpp"'
        )


def _discard_temporary(temporary: Path) -> None:
    try:
        temporary.unlink(missing_ok=True)
    except OSError:
        pass


def run_binary(binary_path: Path, working_dir: Path, runner: ProcessRunner) -> int:
    try:
        result = runner([str(binary_path)], cwd=working_dir)
    except OSError as error:
        raise WorkflowError(f"run failed for {binary_path}") from error
    return result.returncode


def run_samples(
    binary_path: Path,
    test_dir: Path,
    working_dir: Path,
    runner: ProcessRunner,
) -> int:
    try:
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
    except OSError as error:
        raise WorkflowError(f"sample test failed for {binary_path}") from error
    return result.returncode
