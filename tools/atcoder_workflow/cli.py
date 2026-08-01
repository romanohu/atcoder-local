from __future__ import annotations

import argparse
from collections.abc import Sequence
import os
from pathlib import Path
import shutil
import sys

from . import WorkflowError
from .commands import (
    WorkflowDependencies,
    run_doctor,
    run_build,
    run_program,
    run_submit,
    run_tests,
)
from .compiler import BuildMode
from .context import resolve_task_context
from .runner import run_process


def _parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="acc-wrapper")
    subparsers = parser.add_subparsers(dest="command", required=True)

    for command in ("build", "run", "test", "submit"):
        command_parser = subparsers.add_parser(command)
        command_parser.add_argument("-c", "--contest", dest="contest_id")
        command_parser.add_argument("-t", "--task", dest="task_label")
        if command == "test":
            command_parser.add_argument("--debug", action="store_true")

    subparsers.add_parser("doctor")

    return parser


def main(
    argv: Sequence[str],
    cwd: Path | str,
    dependencies: WorkflowDependencies | None = None,
) -> int:
    arguments = _parser().parse_args(list(argv))
    active_dependencies = dependencies or _default_dependencies()

    try:
        if arguments.command == "doctor":
            return run_doctor(cwd, active_dependencies)
        context = resolve_task_context(
            cwd, arguments.contest_id, arguments.task_label
        )
        if arguments.command == "build":
            run_build(context, active_dependencies, mode=BuildMode.RELEASE)
            return 0
        if arguments.command == "run":
            return run_program(context, active_dependencies)
        if arguments.command == "test":
            return run_tests(context, active_dependencies, debug=arguments.debug)
        return run_submit(context, active_dependencies)
    except WorkflowError as error:
        print(f"[acc-wrapper] {error}", file=sys.stderr)
        return 1


def _default_dependencies() -> WorkflowDependencies:
    return WorkflowDependencies(
        runner=run_process,
        environ=os.environ,
        which=shutil.which,
        input_fn=input,
        stdin_isatty=sys.stdin.isatty,
    )
