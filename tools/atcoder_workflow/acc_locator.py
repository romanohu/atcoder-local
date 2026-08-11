from __future__ import annotations

from collections.abc import Mapping
import os
from pathlib import Path
import sys


RAW_ACC_ENVIRONMENT_VARIABLE = "ATCODER_LOCAL_RAW_ACC"


def find_upstream_acc(
    environ: Mapping[str, str],
    wrapper_path: Path | str | None = None,
) -> str | None:
    active = Path(sys.argv[0] if wrapper_path is None else wrapper_path).resolve()
    override = environ.get(RAW_ACC_ENVIRONMENT_VARIABLE)
    if override:
        candidate = Path(override).expanduser().resolve()
        if _is_executable(candidate) and candidate != active:
            return str(candidate)

    for directory in environ.get("PATH", "").split(os.pathsep):
        if not directory:
            continue
        candidate = (Path(directory).expanduser() / "acc").resolve()
        if candidate != active and _is_executable(candidate):
            return str(candidate)
    return None


def _is_executable(path: Path) -> bool:
    return path.is_file() and os.access(path, os.X_OK)
