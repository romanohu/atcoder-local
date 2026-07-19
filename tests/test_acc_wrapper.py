import builtins
import json
import os
from contextlib import redirect_stderr
from io import StringIO
import inspect
import subprocess
import sys
import tempfile
from types import SimpleNamespace
import unittest
from pathlib import Path
from unittest.mock import Mock, patch

from tools import acc_wrapper
from tools.acc_wrapper import (
    build_memo_content,
    create_memo_if_missing,
    extract_contest_dir_format,
    extract_contest_id,
    extract_task_headings,
    main,
)
from tools.push_guard import PushGuardError, StateError


class TestAccWrapper(unittest.TestCase):
    def test_extract_contest_id_simple(self) -> None:
        self.assertEqual(extract_contest_id(["new", "abc454"]), "abc454")

    def test_extract_contest_id_with_option_values(self) -> None:
        args = ["new", "-c", "all", "--template", "cpp", "abc454"]
        self.assertEqual(extract_contest_id(args), "abc454")

    def test_extract_contest_dir_format_short_option(self) -> None:
        args = ["new", "-d", "archive/{ContestID}", "abc454"]
        self.assertEqual(extract_contest_dir_format(args), "archive/{ContestID}")

    def test_extract_contest_dir_format_long_option_with_equal(self) -> None:
        args = ["new", "--contest-dirname-format=archive/{ContestID}", "abc454"]
        self.assertEqual(extract_contest_dir_format(args), "archive/{ContestID}")

    def test_extract_task_headings_prefers_directory_path(self) -> None:
        config = {
            "tasks": [
                {"directory": {"path": "a"}, "label": "A"},
                {"directory": {"path": "b"}, "label": "B"},
            ]
        }
        self.assertEqual(extract_task_headings(config), ["a", "b"])

    def test_extract_task_headings_falls_back_to_label(self) -> None:
        config = {"tasks": [{"label": "PracticeA"}, {"label": "ABC086A"}]}
        self.assertEqual(extract_task_headings(config), ["practicea", "abc086a"])

    def test_build_memo_content(self) -> None:
        expected = "# abc454\n\n## a\n\n## b\n"
        self.assertEqual(build_memo_content("abc454", ["a", "b"]), expected)

    def test_create_memo_if_missing_writes_file(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            contest_dir = Path(tmpdir)
            created = create_memo_if_missing(contest_dir, "abc454", ["a", "b"])
            self.assertTrue(created)
            self.assertEqual(
                (contest_dir / "memo.md").read_text(encoding="utf-8"),
                "# abc454\n\n## a\n\n## b\n",
            )

    def test_create_memo_if_missing_does_not_overwrite(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            contest_dir = Path(tmpdir)
            memo_path = contest_dir / "memo.md"
            memo_path.write_text("# keep\n", encoding="utf-8")

            created = create_memo_if_missing(contest_dir, "abc454", ["a", "b"])
            self.assertFalse(created)
            self.assertEqual(memo_path.read_text(encoding="utf-8"), "# keep\n")

    def test_extract_task_headings_from_realistic_json(self) -> None:
        config_text = """
        {
          "contest": { "id": "abc454" },
          "tasks": [
            { "label": "A", "directory": { "path": "a" } },
            { "label": "B", "directory": { "path": "b" } }
          ]
        }
        """
        config = json.loads(config_text)
        self.assertEqual(extract_task_headings(config), ["a", "b"])


class TestAccWrapperMainCharacterization(unittest.TestCase):
    def test_non_new_delegates_once_and_returns_exact_acc_code(self) -> None:
        completed = subprocess.CompletedProcess(args=[], returncode=23)
        preflight = Mock()
        create_memo = Mock()
        register_contest = Mock()
        operations = SimpleNamespace(
            preflight_guard=preflight,
            create_memo=create_memo,
            register_contest=register_contest,
        )

        with patch("tools.acc_wrapper.subprocess.run", return_value=completed) as run:
            result = main(["submit", "a"], operations=operations)

        self.assertEqual(result, 23)
        run.assert_called_once_with(["acc", "submit", "a"], check=False)
        preflight.assert_not_called()
        create_memo.assert_not_called()
        register_contest.assert_not_called()

    def test_failed_acc_new_returns_exact_code_and_skips_memo(self) -> None:
        completed = subprocess.CompletedProcess(args=[], returncode=7)
        create_memo = Mock()
        register_contest = Mock()
        operations = SimpleNamespace(
            preflight_guard=lambda cwd: Path("/repo/state.json"),
            create_memo=create_memo,
            register_contest=register_contest,
        )

        with patch("tools.acc_wrapper.subprocess.run", return_value=completed) as run:
            result = main(["new", "abc454"], operations=operations)

        self.assertEqual(result, 7)
        run.assert_called_once_with(["acc", "new", "abc454"], check=False)
        create_memo.assert_not_called()
        register_contest.assert_not_called()

    def test_successful_acc_new_keeps_memo_failure_best_effort(self) -> None:
        completed = subprocess.CompletedProcess(args=[], returncode=0)
        stderr = StringIO()
        create_memo = Mock(side_effect=OSError("memo unavailable"))
        register_contest = Mock()
        operations = SimpleNamespace(
            preflight_guard=lambda cwd: Path("/repo/state.json"),
            create_memo=create_memo,
            register_contest=register_contest,
        )

        with patch("tools.acc_wrapper.subprocess.run", return_value=completed) as run:
            with redirect_stderr(stderr):
                result = main(["new", "abc454"], operations=operations)

        self.assertEqual(result, 0)
        run.assert_called_once_with(["acc", "new", "abc454"], check=False)
        create_memo.assert_called_once_with(["new", "abc454"], Path.cwd())
        register_contest.assert_called_once_with(
            "abc454", Path("/repo/state.json")
        )
        self.assertEqual(
            stderr.getvalue(),
            "[acc-wrapper] failed to create memo.md: memo unavailable\n",
        )


class TestAccWrapperMain(unittest.TestCase):
    def test_main_supports_injected_context_and_collaborators(self) -> None:
        parameters = inspect.signature(main).parameters

        self.assertIn("cwd", parameters)
        self.assertIn("acc_runner", parameters)
        self.assertIn("operations", parameters)

    def test_new_calls_preflight_acc_memo_and_registration_in_order(self) -> None:
        events: list[object] = []
        cwd = Path("/repo/contest")
        state_path = Path("/repo/.git/atcoder-push-lock.json")
        operations = SimpleNamespace(
            preflight_guard=lambda actual_cwd: events.append(
                ("preflight", actual_cwd)
            )
            or state_path,
            create_memo=lambda args, actual_cwd: events.append(
                ("memo", args, actual_cwd)
            ),
            register_contest=lambda contest_id, actual_state_path: events.append(
                ("register", contest_id, actual_state_path)
            ),
        )

        result = main(
            ["n", "--template", "cpp", "abc454"],
            cwd=cwd,
            acc_runner=lambda args: events.append(("acc", args)) or 0,
            operations=operations,
        )

        self.assertEqual(result, 0)
        self.assertEqual(
            events,
            [
                ("preflight", cwd),
                ("acc", ["n", "--template", "cpp", "abc454"]),
                ("memo", ["n", "--template", "cpp", "abc454"], cwd),
                ("register", "abc454", state_path),
            ],
        )

    def test_preflight_error_reports_install_command_without_calling_acc(self) -> None:
        acc_calls: list[list[str]] = []
        stderr = StringIO()

        def fail_preflight(cwd: Path) -> Path:
            raise PushGuardError("not a repository")

        operations = SimpleNamespace(
            preflight_guard=fail_preflight,
            create_memo=lambda args, cwd: self.fail("memo must not run"),
            register_contest=lambda contest_id, state_path: self.fail(
                "registration must not run"
            ),
        )

        with redirect_stderr(stderr):
            try:
                result: int | str = main(
                    ["new", "abc454"],
                    cwd=Path("/repo"),
                    acc_runner=lambda args: acc_calls.append(args) or 0,
                    operations=operations,
                )
            except PushGuardError:
                result = "raised"

        self.assertEqual(result, 1)
        self.assertEqual(acc_calls, [])
        self.assertEqual(
            stderr.getvalue(),
            "[acc-wrapper] push guard unavailable: not a repository\n"
            "[acc-wrapper] install it with: "
            "uv run python tools/push_guard.py install\n",
        )

    def test_inactive_default_guard_blocks_before_acc(self) -> None:
        root = Path("/repo")
        acc_calls: list[list[str]] = []
        stderr = StringIO()

        with patch.object(acc_wrapper.push_guard, "repository_root", return_value=root):
            with patch.object(
                acc_wrapper.push_guard,
                "guard_is_installed",
                return_value=False,
            ):
                with patch.object(
                    acc_wrapper.push_guard,
                    "state_path_for_repository",
                ) as state_path:
                    with redirect_stderr(stderr):
                        result = main(
                            ["new", "abc454"],
                            cwd=root,
                            acc_runner=lambda args: acc_calls.append(args) or 0,
                        )

        self.assertEqual(result, 1)
        self.assertEqual(acc_calls, [])
        state_path.assert_not_called()
        self.assertEqual(
            stderr.getvalue(),
            "[acc-wrapper] push guard unavailable: "
            "pre-push guard is not installed\n"
            "[acc-wrapper] install it with: "
            "uv run python tools/push_guard.py install\n",
        )

    def test_default_operations_register_with_official_dependencies(self) -> None:
        root = Path("/repo")
        state_path = Path("/repo/.git/atcoder-push-lock.json")
        memo = Mock()
        stdin = SimpleNamespace(isatty=Mock(return_value=True))

        with (
            patch.object(
                acc_wrapper.push_guard, "repository_root", return_value=root
            ),
            patch.object(
                acc_wrapper.push_guard, "guard_is_installed", return_value=True
            ),
            patch.object(
                acc_wrapper.push_guard,
                "state_path_for_repository",
                return_value=state_path,
            ),
            patch.object(acc_wrapper, "maybe_create_memo", memo),
            patch.object(
                acc_wrapper.push_guard, "fetch_contest_schedule"
            ) as fetch_schedule,
            patch.object(acc_wrapper.push_guard, "utc_now") as utc_now,
            patch.object(
                acc_wrapper.push_guard, "register_contest"
            ) as register_contest,
            patch.object(acc_wrapper.sys, "stdin", stdin),
        ):
            result = main(
                ["new", "abc454"],
                cwd=Path("/repo/tasks"),
                acc_runner=lambda args: 0,
            )

        self.assertEqual(result, 0)
        memo.assert_called_once_with(["new", "abc454"], Path("/repo/tasks"))
        stdin.isatty.assert_called_once_with()
        register_contest.assert_called_once_with(
            "abc454",
            state_path,
            fetch_schedule=fetch_schedule,
            input_value=builtins.input,
            is_interactive=True,
            now=utc_now,
        )

    def test_registration_error_after_successful_acc_returns_one(self) -> None:
        state_path = Path("/repo/state.json")
        stderr = StringIO()
        memo = Mock()

        def fail_registration(contest_id: str, actual_state_path: Path) -> None:
            raise PushGuardError("end time is unresolved")

        operations = SimpleNamespace(
            preflight_guard=lambda cwd: state_path,
            create_memo=memo,
            register_contest=fail_registration,
        )

        with redirect_stderr(stderr):
            try:
                result: int | str = main(
                    ["new", "abc454"],
                    cwd=Path("/repo"),
                    acc_runner=lambda args: 0,
                    operations=operations,
                )
            except PushGuardError:
                result = "raised"

        self.assertEqual(result, 1)
        memo.assert_called_once_with(["new", "abc454"], Path("/repo"))
        self.assertEqual(
            stderr.getvalue(),
            "[acc-wrapper] failed to register contest abc454: "
            "end time is unresolved\n",
        )

    def test_persistence_error_after_successful_acc_returns_one(self) -> None:
        operations = SimpleNamespace(
            preflight_guard=lambda cwd: Path("/repo/state.json"),
            create_memo=lambda args, cwd: None,
            register_contest=lambda contest_id, state_path: (_ for _ in ()).throw(
                StateError("failed to replace push-guard state")
            ),
        )
        stderr = StringIO()

        with redirect_stderr(stderr):
            result = main(
                ["n", "abc454"],
                cwd=Path("/repo"),
                acc_runner=lambda args: 0,
                operations=operations,
            )

        self.assertEqual(result, 1)
        self.assertEqual(
            stderr.getvalue(),
            "[acc-wrapper] failed to register contest abc454: "
            "failed to replace push-guard state\n",
        )

    def test_new_without_contest_id_only_delegates_to_acc(self) -> None:
        acc_calls: list[list[str]] = []
        operations = SimpleNamespace(
            preflight_guard=lambda cwd: self.fail("preflight must not run"),
            create_memo=lambda args, cwd: self.fail("memo must not run"),
            register_contest=lambda contest_id, state_path: self.fail(
                "registration must not run"
            ),
        )

        result = main(
            ["new", "--template"],
            cwd=Path("/repo"),
            acc_runner=lambda args: acc_calls.append(args) or 31,
            operations=operations,
        )

        self.assertEqual(result, 31)
        self.assertEqual(acc_calls, [["new", "--template"]])

    def test_absolute_script_execution_can_import_push_guard(self) -> None:
        script_path = Path(__file__).parents[1] / "tools" / "acc_wrapper.py"
        with tempfile.TemporaryDirectory() as tmpdir:
            fake_acc = Path(tmpdir) / "acc"
            fake_acc.write_text("#!/bin/sh\nexit 17\n", encoding="utf-8")
            fake_acc.chmod(0o755)
            environment = dict(os.environ)
            environment["PATH"] = f"{tmpdir}{os.pathsep}{environment['PATH']}"

            completed = subprocess.run(
                [sys.executable, str(script_path), "--version"],
                cwd=tmpdir,
                env=environment,
                check=False,
                capture_output=True,
                text=True,
            )

        self.assertEqual(completed.returncode, 17)
        self.assertEqual(completed.stderr, "")


if __name__ == "__main__":
    unittest.main()
