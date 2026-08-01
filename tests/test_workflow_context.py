from __future__ import annotations

import json
from pathlib import Path

import pytest

from tools.atcoder_workflow import WorkflowError
from tools.atcoder_workflow.context import resolve_task_context


def write_contest(
    root: Path,
    relative_directory: str = "contests/abc999",
    *,
    submit: str = "main.cpp",
) -> tuple[Path, Path]:
    contest_dir = root / relative_directory
    task_dir = contest_dir / "a"
    test_dir = task_dir / "test"
    task_dir.mkdir(parents=True)
    test_dir.mkdir()
    (task_dir / submit).write_text("source\n", encoding="utf-8")
    config = {
        "contest": {
            "id": "abc999",
            "title": "Contest",
            "url": "https://atcoder.jp/contests/abc999",
        },
        "tasks": [
            {
                "id": "abc999_a",
                "label": "A",
                "title": "Task A",
                "url": "https://atcoder.jp/contests/abc999/tasks/abc999_a",
                "directory": {
                    "path": "a",
                    "submit": submit,
                    "testdir": "test",
                },
            }
        ],
    }
    (contest_dir / "contest.acc.json").write_text(
        json.dumps(config), encoding="utf-8"
    )
    return contest_dir, task_dir


def repository(tmp_path: Path) -> Path:
    (tmp_path / ".git").mkdir()
    return tmp_path


def test_discovers_task_context_from_configured_task_directory(tmp_path: Path) -> None:
    root = repository(tmp_path)
    contest_dir, task_dir = write_contest(root)

    context = resolve_task_context(task_dir)

    assert context.repository_root == root
    assert context.contest_id == "abc999"
    assert context.task_id == "abc999_a"
    assert context.task_label == "A"
    assert context.contest_dir == contest_dir
    assert context.task_dir == task_dir
    assert context.source_path == task_dir / "main.cpp"
    assert context.test_dir == task_dir / "test"
    assert context.build_dir == root / ".atcoder-local/build/abc999/abc999_a"


def test_resolves_root_flags_and_task_label_case_insensitively(tmp_path: Path) -> None:
    root = repository(tmp_path)
    contest_dir, task_dir = write_contest(root)

    context = resolve_task_context(root, contest_id="abc999", task_label="a")

    assert context.contest_dir == contest_dir
    assert context.task_dir == task_dir
    assert context.task_id == "abc999_a"


def test_resolves_non_cpp_submit_without_selecting_a_backend(tmp_path: Path) -> None:
    root = repository(tmp_path)
    _, task_dir = write_contest(root, submit="main.py")

    context = resolve_task_context(task_dir)

    assert context.source_path == task_dir / "main.py"


@pytest.mark.parametrize(
    ("contest_id", "task_label"), [("abc999", None), (None, "a")]
)
def test_rejects_only_one_context_flag(
    tmp_path: Path, contest_id: str | None, task_label: str | None
) -> None:
    root = repository(tmp_path)
    write_contest(root)

    with pytest.raises(WorkflowError):
        resolve_task_context(root, contest_id=contest_id, task_label=task_label)


def test_rejects_duplicate_contest_configs_for_explicit_flags(tmp_path: Path) -> None:
    root = repository(tmp_path)
    write_contest(root, "contests/abc999")
    write_contest(root, "archive/abc999")

    with pytest.raises(WorkflowError):
        resolve_task_context(root, contest_id="abc999", task_label="A")


def test_rejects_submit_path_escaping_task_directory(tmp_path: Path) -> None:
    root = repository(tmp_path)
    _, task_dir = write_contest(root, submit="../outside.cpp")

    with pytest.raises(WorkflowError):
        resolve_task_context(task_dir)


def test_rejects_current_directory_that_is_not_a_configured_task(tmp_path: Path) -> None:
    root = repository(tmp_path)
    contest_dir, _ = write_contest(root)

    with pytest.raises(WorkflowError):
        resolve_task_context(contest_dir)
