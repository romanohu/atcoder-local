from __future__ import annotations

import json
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Callable

if __package__:
    from . import push_guard
else:  # Direct execution from tools/acc-wrapper.zsh.
    import push_guard


COMMANDS_FOR_NEW = {"new", "n"}
INSTALL_COMMAND = "uv run python tools/push_guard.py install"
OPTIONS_WITH_VALUE = {
    "-c",
    "--choice",
    "-d",
    "--contest-dirname-format",
    "-t",
    "--task-dirname-format",
    "--template",
}


@dataclass(frozen=True)
class WrapperOperations:
    preflight_guard: Callable[[Path], Path]
    create_memo: Callable[[list[str], Path], None]
    register_contest: Callable[[str, Path], None]


def preflight_guard(cwd: Path) -> Path:
    root = push_guard.repository_root(cwd)
    if not push_guard.guard_is_installed(root):
        raise push_guard.PushGuardError("pre-push guard is not installed")
    return push_guard.state_path_for_repository(root)


def register_contest_lock(contest_id: str, state_path: Path) -> None:
    push_guard.register_contest(
        contest_id,
        state_path,
        fetch_schedule=push_guard.fetch_contest_schedule,
        input_value=input,
        is_interactive=sys.stdin.isatty(),
        now=push_guard.utc_now,
    )


def first_nonempty_line(text: str) -> str:
    for line in text.splitlines():
        stripped = line.strip()
        if stripped:
            return stripped
    return ""


def extract_contest_id(args: list[str]) -> str | None:
    if not args or args[0] not in COMMANDS_FOR_NEW:
        return None

    idx = 1
    while idx < len(args):
        arg = args[idx]
        if arg in OPTIONS_WITH_VALUE:
            idx += 2
            continue
        if arg.startswith("--contest-dirname-format="):
            idx += 1
            continue
        if arg.startswith("-"):
            idx += 1
            continue
        return arg
    return None


def extract_contest_dir_format(args: list[str]) -> str | None:
    for idx, arg in enumerate(args):
        if arg in {"-d", "--contest-dirname-format"}:
            if idx + 1 < len(args):
                return args[idx + 1]
            return None
        if arg.startswith("--contest-dirname-format="):
            return arg.split("=", 1)[1]
    return None


def extract_task_headings(config: dict[str, Any]) -> list[str]:
    headings: list[str] = []
    seen: set[str] = set()
    tasks = config.get("tasks")
    if not isinstance(tasks, list):
        return headings

    for task in tasks:
        if not isinstance(task, dict):
            continue
        heading: str | None = None
        directory = task.get("directory")
        if isinstance(directory, dict):
            path = directory.get("path")
            if isinstance(path, str) and path.strip():
                heading = path.strip().lower()

        if heading is None:
            label = task.get("label")
            if isinstance(label, str) and label.strip():
                heading = label.strip().lower()

        if heading is not None and heading not in seen:
            headings.append(heading)
            seen.add(heading)
    return headings


def build_memo_content(contest_id: str, task_headings: list[str]) -> str:
    lines = [f"# {contest_id}", ""]
    for heading in task_headings:
        lines.extend((f"## {heading}", ""))
    return "\n".join(lines)


def create_memo_if_missing(
    contest_dir: Path, contest_id: str, task_headings: list[str]
) -> bool:
    memo_path = contest_dir / "memo.md"
    if memo_path.exists():
        return False
    memo_path.write_text(
        build_memo_content(contest_id, task_headings),
        encoding="utf-8",
    )
    return True


def run_capture_stdout(command: list[str]) -> str:
    result = subprocess.run(command, check=True, capture_output=True, text=True)
    return result.stdout


def resolve_contest_dir(contest_id: str, args: list[str], cwd: Path) -> Path:
    contest_dir_format = extract_contest_dir_format(args)
    if contest_dir_format is None:
        config_stdout = run_capture_stdout(
            ["acc", "config", "default-contest-dirname-format"]
        )
        contest_dir_format = first_nonempty_line(config_stdout)
    if not contest_dir_format:
        raise RuntimeError("contest directory format is empty")

    format_stdout = run_capture_stdout(["acc", "format", contest_dir_format, contest_id])
    relative_path = first_nonempty_line(format_stdout)
    if not relative_path:
        raise RuntimeError("failed to resolve contest directory")
    return cwd / relative_path


def maybe_create_memo(args: list[str], cwd: Path) -> None:
    contest_id = extract_contest_id(args)
    if contest_id is None:
        return

    contest_dir = resolve_contest_dir(contest_id, args, cwd)
    contest_config_path = contest_dir / "contest.acc.json"
    if not contest_config_path.exists():
        print(
            f"[acc-wrapper] skip memo.md: {contest_config_path} does not exist.",
            file=sys.stderr,
        )
        return

    config = json.loads(contest_config_path.read_text(encoding="utf-8"))
    task_headings = extract_task_headings(config)
    created = create_memo_if_missing(contest_dir, contest_id, task_headings)
    if created:
        print(f"[acc-wrapper] created {contest_dir / 'memo.md'}")
        return
    print(f"[acc-wrapper] skip memo.md: {(contest_dir / 'memo.md')} already exists.")


def default_operations() -> WrapperOperations:
    return WrapperOperations(
        preflight_guard=preflight_guard,
        create_memo=maybe_create_memo,
        register_contest=register_contest_lock,
    )


def main(
    argv: list[str] | None = None,
    *,
    cwd: Path | None = None,
    acc_runner: Callable[[list[str]], int] | None = None,
    operations: WrapperOperations | None = None,
) -> int:
    args = list(sys.argv[1:] if argv is None else argv)
    working_directory = Path.cwd() if cwd is None else cwd
    contest_id = extract_contest_id(args)
    state_path: Path | None = None
    active_operations = operations
    if contest_id is not None:
        if active_operations is None:
            active_operations = default_operations()
        try:
            state_path = active_operations.preflight_guard(working_directory)
        except push_guard.PushGuardError as exc:
            print(f"[acc-wrapper] push guard unavailable: {exc}", file=sys.stderr)
            print(
                f"[acc-wrapper] install it with: {INSTALL_COMMAND}",
                file=sys.stderr,
            )
            return 1

    if acc_runner is None:
        result = subprocess.run(["acc", *args], check=False)
        returncode = result.returncode
    else:
        returncode = acc_runner(args)
    if returncode != 0:
        return returncode

    memo_interrupt: KeyboardInterrupt | None = None
    if args and args[0] in COMMANDS_FOR_NEW:
        try:
            if contest_id is not None:
                assert active_operations is not None
                active_operations.create_memo(args, working_directory)
        except KeyboardInterrupt as exc:
            memo_interrupt = exc
        except Exception as exc:
            print(f"[acc-wrapper] failed to create memo.md: {exc}", file=sys.stderr)
    if contest_id is not None:
        assert state_path is not None
        assert active_operations is not None
        try:
            active_operations.register_contest(contest_id, state_path)
        except push_guard.PushGuardError as exc:
            print(
                f"[acc-wrapper] failed to register contest {contest_id}: {exc}",
                file=sys.stderr,
            )
            return 1
    if memo_interrupt is not None:
        raise memo_interrupt
    return returncode


if __name__ == "__main__":
    raise SystemExit(main())
