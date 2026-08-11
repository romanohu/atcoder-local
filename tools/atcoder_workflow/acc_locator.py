from __future__ import annotations

import ast
from collections.abc import Mapping
import os
from pathlib import Path
import sys
import tokenize


RAW_ACC_ENVIRONMENT_VARIABLE = "ATCODER_LOCAL_RAW_ACC"


def find_upstream_acc(
    environ: Mapping[str, str],
    wrapper_path: Path | str | None = None,
) -> str | None:
    active = Path(sys.argv[0] if wrapper_path is None else wrapper_path).resolve()
    override = environ.get(RAW_ACC_ENVIRONMENT_VARIABLE)
    if override:
        candidate = Path(override).expanduser().resolve()
        if _is_upstream_acc(candidate, active):
            return str(candidate)

    for directory in environ.get("PATH", "").split(os.pathsep):
        if not directory:
            continue
        candidate = (Path(directory).expanduser() / "acc").resolve()
        if _is_upstream_acc(candidate, active):
            return str(candidate)
    return None


def _is_upstream_acc(path: Path, active: Path) -> bool:
    return (
        path != active
        and _is_executable(path)
        and not _is_atcoder_local_wrapper(path)
    )


def _is_executable(path: Path) -> bool:
    return path.is_file() and os.access(path, os.X_OK)


def _is_atcoder_local_wrapper(path: Path) -> bool:
    try:
        if path.stat().st_size > 64 * 1024:
            return False
        with tokenize.open(path) as script:
            module = ast.parse(script.read())
    except (LookupError, OSError, SyntaxError, UnicodeError):
        return False

    return any(
        isinstance(node, ast.ImportFrom)
        and node.module == "tools.acc_wrapper"
        and any(name.name == "console_main" for name in node.names)
        for node in ast.walk(module)
    )
