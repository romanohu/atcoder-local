from __future__ import annotations

from dataclasses import dataclass
import json
import os
from pathlib import Path
from typing import Any

from . import WorkflowError


CONFIG_FILENAME = "contest.acc.json"
SKIPPED_DIRECTORIES = {".git", ".venv", ".worktrees", ".atcoder-local"}


@dataclass(frozen=True)
class TaskContext:
    repository_root: Path
    contest_id: str
    task_id: str
    task_label: str
    contest_dir: Path
    task_dir: Path
    source_path: Path
    test_dir: Path
    build_dir: Path


def resolve_task_context(
    cwd: Path | str,
    contest_id: str | None = None,
    task_label: str | None = None,
) -> TaskContext:
    current_directory = Path(cwd).resolve()
    if not current_directory.is_dir():
        raise WorkflowError(f"current directory does not exist: {current_directory}")

    repository_root = _find_repository_root(current_directory)
    has_contest = contest_id is not None
    has_task = task_label is not None
    if has_contest != has_task:
        raise WorkflowError("contest and task flags must be provided together")

    if has_contest:
        assert contest_id is not None
        assert task_label is not None
        config_path, config = _find_explicit_config(repository_root, contest_id)
        task = _find_task_by_label(config, task_label, config_path)
    else:
        config_path, config = _find_nearest_config(current_directory, repository_root)
        task = _find_task_for_directory(config, current_directory, config_path)

    return _build_context(repository_root, config_path, config, task)


def _find_repository_root(cwd: Path) -> Path:
    for directory in (cwd, *cwd.parents):
        if (directory / ".git").exists():
            return directory
    raise WorkflowError("not inside a Git repository")


def _find_explicit_config(repository_root: Path, contest_id: str) -> tuple[Path, dict[str, Any]]:
    matches: list[tuple[Path, dict[str, Any]]] = []
    for config_path in _iter_config_paths(repository_root):
        config = _load_config(config_path)
        if _config_contest_id(config, config_path) == contest_id:
            matches.append((config_path, config))

    if len(matches) != 1:
        raise WorkflowError(f"expected one contest config for {contest_id}, found {len(matches)}")
    return matches[0]


def _find_nearest_config(cwd: Path, repository_root: Path) -> tuple[Path, dict[str, Any]]:
    for directory in (cwd, *cwd.parents):
        if not directory.is_relative_to(repository_root):
            break
        config_path = directory / CONFIG_FILENAME
        if config_path.is_file():
            return config_path, _load_config(config_path)
    raise WorkflowError("no contest config found for the current directory")


def _iter_config_paths(repository_root: Path) -> list[Path]:
    paths: list[Path] = []
    for directory, subdirectories, filenames in os.walk(repository_root):
        subdirectories[:] = [
            name for name in subdirectories if name not in SKIPPED_DIRECTORIES
        ]
        if CONFIG_FILENAME in filenames:
            paths.append(Path(directory) / CONFIG_FILENAME)
    return sorted(paths)


def _load_config(path: Path) -> dict[str, Any]:
    try:
        config = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise WorkflowError(f"invalid contest config: {path}") from error
    if not isinstance(config, dict):
        raise WorkflowError(f"invalid contest config: {path}")
    return config


def _config_contest_id(config: dict[str, Any], config_path: Path) -> str:
    contest = config.get("contest")
    if not isinstance(contest, dict) or not isinstance(contest.get("id"), str):
        raise WorkflowError(f"contest id is missing from {config_path}")
    return contest["id"]


def _tasks(config: dict[str, Any], config_path: Path) -> list[dict[str, Any]]:
    tasks = config.get("tasks")
    if not isinstance(tasks, list) or not all(isinstance(task, dict) for task in tasks):
        raise WorkflowError(f"tasks are invalid in {config_path}")
    return tasks


def _find_task_by_label(
    config: dict[str, Any], task_label: str, config_path: Path
) -> dict[str, Any]:
    if not isinstance(task_label, str):
        raise WorkflowError("task label must be a string")
    matches = [
        task
        for task in _tasks(config, config_path)
        if isinstance(task.get("label"), str)
        and task["label"].casefold() == task_label.casefold()
    ]
    if len(matches) != 1:
        raise WorkflowError(
            f"expected one task with label {task_label}, found {len(matches)}"
        )
    return matches[0]


def _find_task_for_directory(
    config: dict[str, Any], cwd: Path, config_path: Path
) -> dict[str, Any]:
    contest_dir = config_path.parent.resolve()
    matches = [
        task
        for task in _tasks(config, config_path)
        if _resolve_within_owner(
            contest_dir, _directory_value(task, "path", config_path), "task directory"
        )
        == cwd
    ]
    if len(matches) != 1:
        raise WorkflowError(
            f"current directory must match exactly one configured task, found {len(matches)}"
        )
    return matches[0]


def _build_context(
    repository_root: Path,
    config_path: Path,
    config: dict[str, Any],
    task: dict[str, Any],
) -> TaskContext:
    contest_dir = config_path.parent.resolve()
    contest_id = _config_contest_id(config, config_path)
    task_id = _task_value(task, "id", config_path)
    task_label = _task_value(task, "label", config_path)
    task_dir = _resolve_within_owner(
        contest_dir, _directory_value(task, "path", config_path), "task directory"
    )
    source_path = _resolve_within_owner(
        task_dir, _directory_value(task, "submit", config_path), "submit path"
    )
    test_dir = _resolve_within_owner(
        task_dir, _directory_value(task, "testdir", config_path), "test directory"
    )

    if source_path.suffix != ".cpp":
        raise WorkflowError(f"submit path must be a .cpp file: {source_path}")
    if not source_path.is_file():
        raise WorkflowError(f"submit source does not exist: {source_path}")
    if not test_dir.is_dir():
        raise WorkflowError(f"test directory does not exist: {test_dir}")

    return TaskContext(
        repository_root=repository_root,
        contest_id=contest_id,
        task_id=task_id,
        task_label=task_label,
        contest_dir=contest_dir,
        task_dir=task_dir,
        source_path=source_path,
        test_dir=test_dir,
        build_dir=repository_root
        / ".atcoder-local"
        / "build"
        / contest_id
        / task_id,
    )


def _task_value(task: dict[str, Any], key: str, config_path: Path) -> str:
    value = task.get(key)
    if not isinstance(value, str):
        raise WorkflowError(f"task {key} is missing from {config_path}")
    return value


def _directory_value(task: dict[str, Any], key: str, config_path: Path) -> str:
    directory = task.get("directory")
    if not isinstance(directory, dict):
        raise WorkflowError(f"task directory is missing from {config_path}")
    value = directory.get(key)
    if not isinstance(value, str):
        raise WorkflowError(f"task directory {key} is missing from {config_path}")
    return value


def _resolve_within_owner(owner: Path, relative_path: str, description: str) -> Path:
    resolved_owner = owner.resolve()
    resolved_path = (resolved_owner / relative_path).resolve()
    if not resolved_path.is_relative_to(resolved_owner):
        raise WorkflowError(f"{description} escapes its owning directory")
    return resolved_path
