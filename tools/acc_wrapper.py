from __future__ import annotations

import os
import subprocess
import sys
from collections.abc import Callable
from pathlib import Path
from typing import NoReturn

if __package__:
    from .atcoder_workflow.acc_locator import find_upstream_acc
else:
    from atcoder_workflow.acc_locator import find_upstream_acc


CUSTOM_COMMANDS = {"build", "run", "test", "submit", "doctor"}


def main(
    argv: list[str] | None = None,
    *,
    cwd: Path | None = None,
    acc_runner: Callable[[list[str]], int] | None = None,
    workflow_runner: Callable[[list[str], Path], int] | None = None,
) -> int:
    args = list(sys.argv[1:] if argv is None else argv)
    working_directory = Path.cwd() if cwd is None else cwd
    if args and args[0] in CUSTOM_COMMANDS:
        if workflow_runner is not None:
            return workflow_runner(args, working_directory)
        if __package__:
            from .atcoder_workflow.cli import main as workflow_main
        else:
            from atcoder_workflow.cli import main as workflow_main
        return workflow_main(args, cwd=working_directory)

    if acc_runner is not None:
        return acc_runner(args)
    raw_acc = find_upstream_acc(os.environ)
    if raw_acc is None:
        print("[acc-wrapper] upstream acc executable was not found", file=sys.stderr)
        return 1
    return subprocess.run([raw_acc, *args], check=False).returncode


def console_main() -> NoReturn:
    os.environ["ATCODER_LOCAL_CONSOLE"] = "1"
    raise SystemExit(main())


if __name__ == "__main__":
    console_main()
