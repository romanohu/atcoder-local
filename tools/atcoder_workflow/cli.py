from __future__ import annotations

import argparse
from collections.abc import Sequence
from pathlib import Path
import sys

from . import WorkflowError
from .commands import (
    WorkflowDependencies,
    run_build,
    run_program,
    run_submit,
    run_tests,
)
from .compiler import BuildMode
from .context import resolve_task_context


def _parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="acc-wrapper")
    subparsers = parser.add_subparsers(dest="command", required=True)

    for command in ("build", "run", "test", "submit"):
        command_parser = subparsers.add_parser(command)
        command_parser.add_argument("-c", "--contest", dest="contest_id")
        command_parser.add_argument("-t", "--task", dest="task_label")
        if command == "test":
            command_parser.add_argument("--debug", action="store_true")

    return parser


def main(
    argv: Sequence[str],
    cwd: Path | str,
    dependencies: WorkflowDependencies,
) -> int:
    arguments = _parser().parse_args(list(argv))

    try:
        context = resolve_task_context(
            cwd, arguments.contest_id, arguments.task_label
        )
        if arguments.command == "build":
            run_build(context, dependencies, mode=BuildMode.RELEASE)
            return 0
        if arguments.command == "run":
            return run_program(context, dependencies)
        if arguments.command == "test":
            return run_tests(context, dependencies, debug=arguments.debug)
        return run_submit(context, dependencies)
    except WorkflowError as error:
        print(f"[acc-wrapper] {error}", file=sys.stderr)
        return 1
