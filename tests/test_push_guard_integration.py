from __future__ import annotations

from datetime import timedelta
from pathlib import Path
import os
import shutil
import subprocess
from tempfile import TemporaryDirectory
import unittest
from unittest.mock import patch

from tools import push_guard
from tools.push_guard import ContestLock


PROJECT_ROOT = Path(__file__).parents[1]
GIT_REPOSITORY_ENVIRONMENT_VARIABLES = {
    "GIT_ALTERNATE_OBJECT_DIRECTORIES",
    "GIT_CEILING_DIRECTORIES",
    "GIT_COMMON_DIR",
    "GIT_CONFIG",
    "GIT_DIR",
    "GIT_DISCOVERY_ACROSS_FILESYSTEM",
    "GIT_GRAFT_FILE",
    "GIT_IMPLICIT_WORK_TREE",
    "GIT_INDEX_FILE",
    "GIT_INTERNAL_SUPER_PREFIX",
    "GIT_NAMESPACE",
    "GIT_NO_REPLACE_OBJECTS",
    "GIT_OBJECT_DIRECTORY",
    "GIT_PREFIX",
    "GIT_QUARANTINE_PATH",
    "GIT_REPLACE_REF_BASE",
    "GIT_SHALLOW_FILE",
    "GIT_TEMPLATE_DIR",
    "GIT_WORK_TREE",
}


def sanitized_git_environment() -> dict[str, str]:
    environment = dict(os.environ)
    for key in list(environment):
        if key in GIT_REPOSITORY_ENVIRONMENT_VARIABLES or key.startswith(
            "GIT_CONFIG_"
        ):
            environment.pop(key)
    environment["GIT_CONFIG_NOSYSTEM"] = "1"
    environment["GIT_CONFIG_GLOBAL"] = os.devnull
    return environment


class TestGitEnvironmentIsolation(unittest.TestCase):
    def test_sanitized_environment_preserves_ordinary_values_and_removes_git_injection(
        self,
    ) -> None:
        contaminated_environment = {
            "PATH": os.environ["PATH"],
            "HOME": os.environ.get("HOME", ""),
            "LANG": "C",
            "UNRELATED_TEST_VALUE": "keep-me",
            "GIT_PAGER": "cat",
            "GIT_DIR": "/hostile/repository.git",
            "GIT_WORK_TREE": "/hostile/work-tree",
            "GIT_COMMON_DIR": "/hostile/common.git",
            "GIT_INDEX_FILE": "/hostile/index",
            "GIT_OBJECT_DIRECTORY": "/hostile/objects",
            "GIT_ALTERNATE_OBJECT_DIRECTORIES": "/hostile/alternates",
            "GIT_CONFIG_GLOBAL": "/hostile/global-config",
            "GIT_CONFIG_NOSYSTEM": "0",
            "GIT_CONFIG_COUNT": "1",
            "GIT_CONFIG_KEY_0": "core.hooksPath",
            "GIT_CONFIG_VALUE_0": "/hostile/hooks",
            "GIT_CONFIG_PARAMETERS": "'core.hooksPath'='/hostile/hooks'",
        }

        with patch.dict(os.environ, contaminated_environment, clear=True):
            sanitized = sanitized_git_environment()

        for key in (
            "PATH",
            "HOME",
            "LANG",
            "UNRELATED_TEST_VALUE",
            "GIT_PAGER",
        ):
            self.assertEqual(sanitized[key], contaminated_environment[key])
        for key in (
            "GIT_DIR",
            "GIT_WORK_TREE",
            "GIT_COMMON_DIR",
            "GIT_INDEX_FILE",
            "GIT_OBJECT_DIRECTORY",
            "GIT_ALTERNATE_OBJECT_DIRECTORIES",
            "GIT_CONFIG_COUNT",
            "GIT_CONFIG_KEY_0",
            "GIT_CONFIG_VALUE_0",
            "GIT_CONFIG_PARAMETERS",
        ):
            self.assertNotIn(key, sanitized)
        self.assertEqual(sanitized["GIT_CONFIG_NOSYSTEM"], "1")
        self.assertEqual(sanitized["GIT_CONFIG_GLOBAL"], os.devnull)

    def test_hostile_global_receive_hook_affects_only_unsanitized_probe(self) -> None:
        with TemporaryDirectory() as tmpdir:
            root = Path(tmpdir).resolve()
            repository = root / "work"
            remote = root / "remote.git"
            hostile_hooks = root / "hostile-hooks"
            hostile_hooks.mkdir()
            rejecting_hook = hostile_hooks / "pre-receive"
            rejecting_hook.write_text(
                "#!/bin/sh\n"
                "echo 'hostile global receive hook' >&2\n"
                "exit 1\n",
                encoding="utf-8",
            )
            rejecting_hook.chmod(0o755)
            hostile_config = root / "hostile-global.config"
            hostile_config.write_text(
                f"[core]\n\thooksPath = {hostile_hooks}\n",
                encoding="utf-8",
            )
            clean_environment = sanitized_git_environment()

            def run_git(
                arguments: list[str],
                *,
                cwd: Path = root,
                environment: dict[str, str] = clean_environment,
            ) -> subprocess.CompletedProcess[str]:
                return subprocess.run(
                    ["git", *arguments],
                    cwd=cwd,
                    env=environment,
                    check=False,
                    capture_output=True,
                    text=True,
                )

            for arguments in (
                [
                    "-c",
                    "init.defaultBranch=main",
                    "-c",
                    "init.templateDir=",
                    "init",
                    str(repository),
                ],
                [
                    "-c",
                    "init.defaultBranch=main",
                    "-c",
                    "init.templateDir=",
                    "init",
                    "--bare",
                    str(remote),
                ],
            ):
                initialized = run_git(arguments)
                self.assertEqual(initialized.returncode, 0, initialized.stderr)
            for key, value in (
                ("user.name", "Push Guard Test"),
                ("user.email", "push-guard@example.invalid"),
                ("commit.gpgSign", "false"),
            ):
                configured = run_git(
                    ["config", "--local", key, value],
                    cwd=repository,
                )
                self.assertEqual(configured.returncode, 0, configured.stderr)
            (repository / "tracked.txt").write_text("initial\n", encoding="utf-8")
            for arguments in (
                ["add", "tracked.txt"],
                ["commit", "-m", "initial"],
            ):
                completed = run_git(arguments, cwd=repository)
                self.assertEqual(completed.returncode, 0, completed.stderr)

            hostile_environment = dict(clean_environment)
            hostile_environment["GIT_CONFIG_GLOBAL"] = str(hostile_config)
            # This one call is deliberately unsanitized to prove the hostile
            # global hook changes real Git receive behavior.
            rejected = run_git(
                ["push", str(remote), "main"],
                cwd=repository,
                environment=hostile_environment,
            )
            self.assertNotEqual(rejected.returncode, 0)
            self.assertIn("hostile global receive hook", rejected.stderr)

            with patch.dict(os.environ, hostile_environment, clear=True):
                isolated_environment = sanitized_git_environment()
            accepted = run_git(
                ["push", str(remote), "main"],
                cwd=repository,
                environment=isolated_environment,
            )
            self.assertEqual(accepted.returncode, 0, accepted.stderr)
            local_head = run_git(["rev-parse", "HEAD"], cwd=repository)
            remote_head = run_git(
                [
                    f"--git-dir={remote}",
                    "rev-parse",
                    "--verify",
                    "refs/heads/main",
                ]
            )
            self.assertEqual(remote_head.returncode, 0, remote_head.stderr)
            self.assertEqual(remote_head.stdout, local_head.stdout)


class TestPushGuardGitIntegration(unittest.TestCase):
    def setUp(self) -> None:
        temporary_directory = TemporaryDirectory()
        self.addCleanup(temporary_directory.cleanup)
        self.temporary_root = Path(temporary_directory.name).resolve()
        self.git_environment = sanitized_git_environment()
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

        with patch.dict(os.environ, self.git_environment, clear=True):
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
            env=self.git_environment,
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
            env=self.git_environment,
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
            env=self.git_environment,
            check=False,
            capture_output=True,
            text=True,
        )
        if check:
            self.assertEqual(result.returncode, 0, result.stderr)
        return result


if __name__ == "__main__":
    unittest.main()
