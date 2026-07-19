from __future__ import annotations

from datetime import timedelta
from pathlib import Path
import os
import shutil
import subprocess
from tempfile import TemporaryDirectory
import unittest

from tools import push_guard
from tools.push_guard import ContestLock


PROJECT_ROOT = Path(__file__).parents[1]


class TestPushGuardGitIntegration(unittest.TestCase):
    def setUp(self) -> None:
        temporary_directory = TemporaryDirectory()
        self.addCleanup(temporary_directory.cleanup)
        self.temporary_root = Path(temporary_directory.name).resolve()
        self.repository = self.temporary_root / "work"
        self.origin = self.temporary_root / "origin.git"
        self.secondary = self.temporary_root / "secondary.git"

        self._init_repository(self.repository)
        self._init_repository(self.origin, bare=True)
        self._init_repository(self.secondary, bare=True)
        self._configure_repository()
        self._copy_push_guard()
        self._git(["remote", "add", "origin", str(self.origin)])
        self._git(["remote", "add", "secondary", str(self.secondary)])
        self._commit("initial")
        self.assertEqual(self._git(["branch", "--show-current"]).stdout.strip(), "main")

        self.state_path = push_guard.state_path_for_repository(self.repository)

    def test_no_state_allows_first_push(self) -> None:
        self.assertFalse(self.state_path.exists())

        result = self._push("origin", "main")

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(
            self._remote_ref(self.origin, "refs/heads/main"),
            self._local_head(),
        )
        self.assertNotIn("push blocked:", result.stderr)
        self.assertFalse(self.state_path.exists())

    def test_active_lock_rejects_push_and_preserves_origin_ref(self) -> None:
        first_push = self._push("origin", "main")
        self.assertEqual(first_push.returncode, 0, first_push.stderr)
        remote_head = self._remote_ref(self.origin, "refs/heads/main")
        self.assertIsNotNone(remote_head)
        self._commit("after-active-registration")
        now = push_guard.utc_now()
        self._write_locks(
            [ContestLock("abc900", now - timedelta(hours=1), now + timedelta(hours=1))]
        )

        result = self._push("origin", "main")

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("push blocked: abc900 (active until ", result.stderr)
        self.assertEqual(
            self._remote_ref(self.origin, "refs/heads/main"),
            remote_head,
        )
        self.assertNotEqual(self._local_head(), remote_head)

    def test_unresolved_lock_rejects_push_and_preserves_origin_ref(self) -> None:
        first_push = self._push("origin", "main")
        self.assertEqual(first_push.returncode, 0, first_push.stderr)
        remote_head = self._remote_ref(self.origin, "refs/heads/main")
        self.assertIsNotNone(remote_head)
        self._commit("after-unresolved-registration")
        self._write_locks([ContestLock("abc901", push_guard.utc_now(), None)])

        result = self._push("origin", "main")

        self.assertNotEqual(result.returncode, 0)
        self.assertIn(
            "push blocked: abc901 (unresolved end time)",
            result.stderr,
        )
        self.assertEqual(
            self._remote_ref(self.origin, "refs/heads/main"),
            remote_head,
        )

    def test_corrupt_state_rejects_push_and_preserves_origin_ref(self) -> None:
        first_push = self._push("origin", "main")
        self.assertEqual(first_push.returncode, 0, first_push.stderr)
        remote_head = self._remote_ref(self.origin, "refs/heads/main")
        self.assertIsNotNone(remote_head)
        self._commit("after-state-corruption")
        self.state_path.write_text("{not valid json\n", encoding="utf-8")

        result = self._push("origin", "main")

        self.assertNotEqual(result.returncode, 0)
        self.assertIn(
            "push blocked: invalid state: malformed push-guard state JSON:",
            result.stderr,
        )
        self.assertEqual(
            self._remote_ref(self.origin, "refs/heads/main"),
            remote_head,
        )

    def test_expired_lock_allows_push(self) -> None:
        now = push_guard.utc_now()
        self._write_locks(
            [ContestLock("abc902", now - timedelta(hours=2), now - timedelta(hours=1))]
        )

        result = self._push("origin", "main")

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(
            self._remote_ref(self.origin, "refs/heads/main"),
            self._local_head(),
        )
        self.assertNotIn("push blocked:", result.stderr)

    def test_active_lock_rejects_nondefault_branch_to_second_remote(self) -> None:
        first_push = self._push("origin", "main")
        self.assertEqual(first_push.returncode, 0, first_push.stderr)
        self._git(["switch", "-c", "guarded-topic"])
        self._commit("guarded-topic-change")
        now = push_guard.utc_now()
        self._write_locks(
            [ContestLock("abc903", now - timedelta(hours=1), now + timedelta(hours=1))]
        )
        self.assertIsNone(
            self._remote_ref(self.secondary, "refs/heads/guarded-topic")
        )

        result = self._push("secondary", "guarded-topic")

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("push blocked: abc903 (active until ", result.stderr)
        self.assertIsNone(
            self._remote_ref(self.secondary, "refs/heads/guarded-topic")
        )
        self.assertIsNone(self._remote_ref(self.secondary, "refs/heads/main"))

    def _init_repository(self, path: Path, *, bare: bool = False) -> None:
        arguments = [
            "git",
            "-c",
            "init.defaultBranch=main",
            "-c",
            "init.templateDir=",
            "init",
        ]
        if bare:
            arguments.append("--bare")
        arguments.append(str(path))
        result = subprocess.run(
            arguments,
            cwd=self.temporary_root,
            check=False,
            capture_output=True,
            text=True,
        )
        self.assertEqual(result.returncode, 0, result.stderr)

    def _configure_repository(self) -> None:
        self._git(["config", "--local", "user.name", "Push Guard Test"])
        self._git(
            [
                "config",
                "--local",
                "user.email",
                "push-guard@example.invalid",
            ]
        )
        self._git(["config", "--local", "commit.gpgSign", "false"])
        self._git(["config", "--local", "core.hooksPath", ".githooks"])

    def _copy_push_guard(self) -> None:
        tools_directory = self.repository / "tools"
        hooks_directory = self.repository / ".githooks"
        tools_directory.mkdir()
        hooks_directory.mkdir()
        shutil.copy2(
            PROJECT_ROOT / "tools" / "push_guard.py",
            tools_directory / "push_guard.py",
        )
        copied_hook = shutil.copy2(
            PROJECT_ROOT / ".githooks" / "pre-push",
            hooks_directory / "pre-push",
        )
        self.assertTrue(os.access(copied_hook, os.X_OK))

    def _commit(self, label: str) -> str:
        tracked_file = self.repository / "tracked.txt"
        with tracked_file.open("a", encoding="utf-8") as stream:
            stream.write(f"{label}\n")
        self._git(["add", "tracked.txt"])
        self._git(["commit", "-m", label])
        return self._local_head()

    def _write_locks(self, locks: list[ContestLock]) -> None:
        push_guard.write_state(self.state_path, locks)

    def _push(
        self,
        remote: str,
        branch: str,
    ) -> subprocess.CompletedProcess[str]:
        return self._git(["push", remote, branch], check=False)

    def _local_head(self) -> str:
        return self._git(["rev-parse", "HEAD"]).stdout.strip()

    def _remote_ref(self, remote: Path, ref: str) -> str | None:
        result = subprocess.run(
            [
                "git",
                f"--git-dir={remote}",
                "rev-parse",
                "--verify",
                "--quiet",
                ref,
            ],
            cwd=self.temporary_root,
            check=False,
            capture_output=True,
            text=True,
        )
        if result.returncode == 1:
            return None
        self.assertEqual(result.returncode, 0, result.stderr)
        return result.stdout.strip()

    def _git(
        self,
        arguments: list[str],
        *,
        check: bool = True,
    ) -> subprocess.CompletedProcess[str]:
        result = subprocess.run(
            ["git", *arguments],
            cwd=self.repository,
            check=False,
            capture_output=True,
            text=True,
        )
        if check:
            self.assertEqual(result.returncode, 0, result.stderr)
        return result


if __name__ == "__main__":
    unittest.main()
