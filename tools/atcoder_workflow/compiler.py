from __future__ import annotations

from collections.abc import Callable, Mapping
from dataclasses import dataclass
from enum import Enum
from pathlib import Path
import re
from tempfile import TemporaryDirectory

from . import WorkflowError
from .runner import ProcessRunner


class CompilerFamily(Enum):
    GCC = "gcc"
    CLANG = "clang"


class BuildMode(Enum):
    RELEASE = "release"
    DEBUG = "debug"


@dataclass(frozen=True)
class CompilerInfo:
    executable: str
    family: CompilerFamily
    major: int
    version_text: str


CompilerLocator = Callable[[str], str | None]


def detect_compiler(
    environ: Mapping[str, str], runner: ProcessRunner, which: CompilerLocator
) -> CompilerInfo:
    if "CXX" in environ:
        return _verified_compiler(environ["CXX"], runner)

    verified_paths: dict[str, CompilerInfo | None] = {}
    candidates = (
        ("g++-15", _is_gcc15),
        ("g++15", _is_gcc15),
        ("g++", _accept_any_compiler),
        ("clang++", _accept_any_compiler),
    )
    for name, acceptable in candidates:
        executable = which(name)
        if executable is None:
            continue
        if executable not in verified_paths:
            try:
                verified_paths[executable] = _verified_compiler(executable, runner)
            except WorkflowError:
                verified_paths[executable] = None
        info = verified_paths[executable]
        if info is None:
            continue
        if acceptable(info):
            return info

    raise WorkflowError("no C++23 compiler found")


def compiler_flags(mode: BuildMode, library_dir: Path | str) -> list[str]:
    if mode is BuildMode.RELEASE:
        return [
            "-std=gnu++23",
            "-O2",
            "-Wall",
            "-Wextra",
            "-DONLINE_JUDGE",
            "-DATCODER",
            "-I",
            str(library_dir),
        ]
    if mode is BuildMode.DEBUG:
        return [
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
            str(library_dir),
        ]
    raise WorkflowError(f"unsupported build mode: {mode}")


def _verified_compiler(executable: str, runner: ProcessRunner) -> CompilerInfo:
    if not executable:
        raise WorkflowError("CXX must name one compiler executable")

    try:
        version = runner([executable, "--version"], capture_output=True)
    except OSError as error:
        raise WorkflowError(f"failed to execute compiler: {executable}") from error
    version_text = version.stdout + version.stderr
    if version.returncode != 0:
        raise WorkflowError(f"compiler version check failed: {executable}")

    family, major = _parse_version(version_text, executable)
    _probe_cpp23(executable, runner)
    return CompilerInfo(executable, family, major, version_text)


def _parse_version(version_text: str, executable: str) -> tuple[CompilerFamily, int]:
    clang = re.search(r"(?:apple\s+)?clang version\s+(\d+)", version_text, re.IGNORECASE)
    if clang is not None:
        return CompilerFamily.CLANG, int(clang.group(1))

    gcc = re.search(r"(?:g\+\+|gcc).*?(\d+)", version_text, re.IGNORECASE)
    if gcc is not None:
        return CompilerFamily.GCC, int(gcc.group(1))

    raise WorkflowError(f"unrecognized compiler version: {executable}")


def _probe_cpp23(executable: str, runner: ProcessRunner) -> None:
    with TemporaryDirectory(prefix="atcoder-workflow-") as directory:
        source_path = Path(directory) / "probe.cpp"
        output_path = Path(directory) / "probe"
        source_path.write_text("int main() { return 0; }\n", encoding="utf-8")
        try:
            result = runner(
                [executable, "-std=gnu++23", str(source_path), "-o", str(output_path)],
                capture_output=True,
            )
        except OSError as error:
            raise WorkflowError(f"failed to execute compiler: {executable}") from error
    if result.returncode != 0:
        raise WorkflowError(f"compiler does not support C++23: {executable}")


def _is_gcc15(info: CompilerInfo) -> bool:
    return info.family is CompilerFamily.GCC and info.major == 15


def _accept_any_compiler(info: CompilerInfo) -> bool:
    return True
