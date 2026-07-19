from contextlib import redirect_stderr, redirect_stdout
from datetime import datetime, timedelta, timezone, tzinfo
from http.client import BadStatusLine, IncompleteRead
from io import StringIO
import json
import math
import os
from pathlib import Path
import shutil
import stat
import subprocess
import sys
from tempfile import TemporaryDirectory
import unittest
from unittest.mock import patch
from urllib.error import HTTPError
from zoneinfo import ZoneInfo

from tools import push_guard
from tools.push_guard import (
    ContestLock,
    PushGuardError,
    STATE_VERSION,
    StateError,
    blocking_contests,
    contest_url,
    fetch_contest_schedule,
    load_state,
    parse_contest_schedule,
    status_for_lock,
    upsert_lock,
    write_state,
)


FIXTURES = Path(__file__).parent / "fixtures"


class _FailingTimezone(tzinfo):
    def __init__(self, error: ValueError | OverflowError) -> None:
        self.error = error

    def utcoffset(self, value: datetime | None) -> timedelta | None:
        raise self.error


class _RawCopyFailure(BaseException):
    pass


class TestGuardDecision(unittest.TestCase):
    def setUp(self) -> None:
        self.start = datetime(2026, 7, 18, 12, 0, tzinfo=timezone.utc)
        self.end = datetime(2026, 7, 18, 13, 40, tzinfo=timezone.utc)
        self.lock = ContestLock("abc467", self.start, self.end)

    def test_contest_lock_is_frozen(self) -> None:
        with self.assertRaises(AttributeError):
            self.lock.contest_id = "abc468"

    def test_blocks_closed_open_interval(self) -> None:
        self.assertEqual(blocking_contests([self.lock], self.start), [self.lock])
        almost_end = self.end - timedelta(microseconds=1)
        self.assertEqual(blocking_contests([self.lock], almost_end), [self.lock])

    def test_allows_before_start_and_at_end(self) -> None:
        before_start = self.start - timedelta(microseconds=1)
        self.assertEqual(blocking_contests([self.lock], before_start), [])
        self.assertEqual(blocking_contests([self.lock], self.end), [])

    def test_unresolved_always_blocks(self) -> None:
        unresolved = ContestLock("abc467", self.start, None)

        long_before_start = self.start - timedelta(days=365)
        long_after_start = self.start + timedelta(days=365)
        self.assertEqual(
            blocking_contests([unresolved], long_before_start),
            [unresolved],
        )
        self.assertEqual(
            blocking_contests([unresolved], long_after_start),
            [unresolved],
        )

    def test_returns_every_blocking_contest_sorted_by_id(self) -> None:
        locks = [
            ContestLock("zzz", self.start, None),
            ContestLock("past", self.start - timedelta(days=2), self.start),
            ContestLock("aaa", self.start, self.end),
            ContestLock("future", self.end, self.end + timedelta(hours=1)),
        ]

        result = blocking_contests(locks, self.start)

        self.assertEqual([lock.contest_id for lock in result], ["aaa", "zzz"])

    def test_reports_all_status_values(self) -> None:
        now = self.start
        cases = {
            "upcoming": ContestLock("upcoming", now + timedelta(seconds=1), self.end),
            "active": ContestLock("active", now, self.end),
            "expired": ContestLock("expired", now - timedelta(hours=1), now),
            "unresolved": ContestLock("unresolved", now + timedelta(days=1), None),
        }

        for expected, lock in cases.items():
            with self.subTest(expected=expected):
                self.assertEqual(status_for_lock(lock, now), expected)

    def test_status_uses_utc_timeline_across_fallback_fold(self) -> None:
        zone = ZoneInfo("America/New_York")
        lock = ContestLock(
            "fallback",
            datetime(2025, 11, 2, 1, 30, tzinfo=zone, fold=0),
            datetime(2025, 11, 2, 1, 30, tzinfo=zone, fold=1),
        )
        now = datetime(2025, 11, 2, 1, 15, tzinfo=zone, fold=1)

        self.assertEqual(status_for_lock(lock, now), "active")

    def test_blocking_uses_utc_timeline_across_fallback_fold(self) -> None:
        zone = ZoneInfo("America/New_York")
        lock = ContestLock(
            "fallback",
            datetime(2025, 11, 2, 1, 30, tzinfo=zone, fold=0),
            datetime(2025, 11, 2, 1, 30, tzinfo=zone, fold=1),
        )
        now = datetime(2025, 11, 2, 1, 15, tzinfo=zone, fold=1)

        self.assertEqual(blocking_contests([lock], now), [lock])


class TestStatePersistence(unittest.TestCase):
    def setUp(self) -> None:
        temporary_directory = TemporaryDirectory()
        self.addCleanup(temporary_directory.cleanup)
        self.directory = Path(temporary_directory.name)
        self.path = self.directory / "atcoder-push-lock.json"
        self.start = datetime(2026, 7, 18, 12, 0, tzinfo=timezone.utc)
        self.end = datetime(2026, 7, 18, 13, 40, tzinfo=timezone.utc)

    def test_missing_state_file_returns_empty_list(self) -> None:
        self.assertEqual(load_state(self.path), [])

    def test_round_trip_is_sorted_canonical_readable_and_unresolved(self) -> None:
        jst = timezone(timedelta(hours=9))
        locks = [
            ContestLock(
                "zzz",
                datetime(2026, 7, 18, 21, 0, tzinfo=jst),
                None,
            ),
            ContestLock("abc467", self.start, self.end),
        ]

        write_state(self.path, locks)

        raw = self.path.read_text(encoding="utf-8")
        self.assertTrue(raw.endswith("\n"))
        self.assertIn('\n  "version": 1,\n', raw)
        self.assertIn('\n  "contests": [\n', raw)
        self.assertEqual(
            json.loads(raw),
            {
                "version": STATE_VERSION,
                "contests": [
                    {
                        "contest_id": "abc467",
                        "start_at": "2026-07-18T12:00:00Z",
                        "end_at": "2026-07-18T13:40:00Z",
                    },
                    {
                        "contest_id": "zzz",
                        "start_at": "2026-07-18T12:00:00Z",
                        "end_at": None,
                    },
                ],
            },
        )
        self.assertEqual(
            load_state(self.path),
            [
                ContestLock("abc467", self.start, self.end),
                ContestLock("zzz", self.start, None),
            ],
        )

    def test_rejects_malformed_json(self) -> None:
        self.path.write_text("{not-json", encoding="utf-8")

        with self.assertRaises(StateError):
            load_state(self.path)

    def test_wraps_excessive_json_nesting(self) -> None:
        self.path.write_text("[" * 1100 + "]" * 1100, encoding="utf-8")

        with self.assertRaises(StateError) as raised:
            load_state(self.path)

        self.assertIsInstance(raised.exception.__cause__, RecursionError)

    def test_wraps_oversized_json_integer(self) -> None:
        raw = '{"version":' + "9" * 5000 + ',"contests":[]}'
        self.path.write_text(raw, encoding="utf-8")

        with self.assertRaises(StateError) as raised:
            load_state(self.path)

        self.assertIsInstance(raised.exception.__cause__, ValueError)

    def test_rejects_unknown_version_and_wrong_top_level_types(self) -> None:
        cases = {
            "top-level list": [],
            "string version": {"version": "1", "contests": []},
            "boolean version": {"version": True, "contests": []},
            "unknown version": {"version": 2, "contests": []},
            "object contests": {"version": 1, "contests": {}},
        }
        for name, value in cases.items():
            with self.subTest(name=name):
                self._write_json(value)
                with self.assertRaises(StateError):
                    load_state(self.path)

    def test_requires_exact_top_level_and_record_keys(self) -> None:
        valid_record = self._record()
        cases = {
            "missing top-level key": {"version": 1},
            "extra top-level key": {
                "version": 1,
                "contests": [],
                "extra": None,
            },
            "record is not an object": {"version": 1, "contests": [None]},
            "missing record key": {
                "version": 1,
                "contests": [
                    {"contest_id": "abc467", "start_at": valid_record["start_at"]}
                ],
            },
            "extra record key": {
                "version": 1,
                "contests": [{**valid_record, "extra": None}],
            },
        }
        for name, value in cases.items():
            with self.subTest(name=name):
                self._write_json(value)
                with self.assertRaises(StateError):
                    load_state(self.path)

    def test_rejects_invalid_record_field_types_and_contest_ids(self) -> None:
        cases = {
            "integer contest ID": self._record(contest_id=123),
            "invalid contest ID": self._record(contest_id="../abc467"),
            "integer start": self._record(start_at=123),
            "integer end": self._record(end_at=123),
            "boolean end": self._record(end_at=False),
        }
        for name, record in cases.items():
            with self.subTest(name=name):
                self._write_json({"version": 1, "contests": [record]})
                with self.assertRaises(StateError):
                    load_state(self.path)

    def test_rejects_noncanonical_naive_and_offset_timestamps(self) -> None:
        cases = {
            "space separator": "2026-07-18 12:00:00Z",
            "naive": "2026-07-18T12:00:00",
            "offset": "2026-07-18T12:00:00+00:00",
            "fractional": "2026-07-18T12:00:00.000000Z",
            "not zero padded": "2026-7-18T12:00:00Z",
            "invalid calendar date": "2026-02-30T12:00:00Z",
        }
        for name, start_at in cases.items():
            with self.subTest(name=name):
                record = self._record(start_at=start_at)
                self._write_json({"version": 1, "contests": [record]})
                with self.assertRaises(StateError):
                    load_state(self.path)

    def test_rejects_duplicate_contest_ids(self) -> None:
        record = self._record()
        self._write_json({"version": 1, "contests": [record, record]})

        with self.assertRaises(StateError):
            load_state(self.path)

    def test_rejects_nonincreasing_intervals(self) -> None:
        cases = {
            "equal": "2026-07-18T12:00:00Z",
            "reversed": "2026-07-18T11:59:59Z",
        }
        for name, end_at in cases.items():
            with self.subTest(name=name):
                record = self._record(end_at=end_at)
                self._write_json({"version": 1, "contests": [record]})
                with self.assertRaises(StateError):
                    load_state(self.path)

    def test_rejects_outgoing_interval_reversed_in_utc_across_fallback_fold(
        self,
    ) -> None:
        zone = ZoneInfo("America/New_York")
        lock = ContestLock(
            "fallback",
            datetime(2025, 11, 2, 1, 15, tzinfo=zone, fold=1),
            datetime(2025, 11, 2, 1, 45, tzinfo=zone, fold=0),
        )

        with self.assertRaises(StateError):
            write_state(self.path, [lock])

        self.assertFalse(self.path.exists())

    def test_wraps_utc_conversion_errors_for_outgoing_timestamps(self) -> None:
        errors = {
            "value": ValueError("invalid offset"),
            "overflow": OverflowError("offset overflow"),
        }
        for name, error in errors.items():
            with self.subTest(name=name):
                invalid_start = datetime(
                    2026,
                    7,
                    18,
                    12,
                    0,
                    tzinfo=_FailingTimezone(error),
                )
                lock = ContestLock("abc467", invalid_start, self.end)

                with self.assertRaises(StateError) as raised:
                    write_state(self.path, [lock])

                self.assertIs(raised.exception.__cause__, error)

    def test_wraps_nonabsence_read_and_unicode_decode_errors(self) -> None:
        errors = {
            "read": PermissionError("permission denied"),
            "decode": UnicodeDecodeError(
                "utf-8", b"\xff", 0, 1, "invalid start byte"
            ),
        }
        for name, error in errors.items():
            with self.subTest(name=name):
                with patch.object(Path, "read_text", side_effect=error):
                    with self.assertRaises(StateError) as raised:
                        load_state(self.path)
                self.assertIs(raised.exception.__cause__, error)

    def test_validates_every_outgoing_lock_before_replacing_state(self) -> None:
        original = ContestLock("abc467", self.start, self.end)
        write_state(self.path, [original])
        original_contents = self.path.read_bytes()
        cases = {
            "wrong lock type": object(),
            "invalid ID": ContestLock("../abc467", self.start, self.end),
            "naive start": ContestLock(
                "abc467", self.start.replace(tzinfo=None), self.end
            ),
            "fractional start": ContestLock(
                "abc467", self.start + timedelta(microseconds=1), self.end
            ),
            "equal interval": ContestLock("abc467", self.start, self.start),
        }
        for name, lock in cases.items():
            with self.subTest(name=name):
                with self.assertRaises(StateError):
                    write_state(self.path, [lock])
                self.assertEqual(self.path.read_bytes(), original_contents)

    def test_rejects_duplicate_outgoing_ids_before_replacing_state(self) -> None:
        original = ContestLock("abc467", self.start, self.end)
        write_state(self.path, [original])
        original_contents = self.path.read_bytes()

        with self.assertRaises(StateError):
            write_state(self.path, [original, original])

        self.assertEqual(self.path.read_bytes(), original_contents)

    def test_upsert_replaces_same_id_preserves_others_and_sorts(self) -> None:
        write_state(
            self.path,
            [
                ContestLock("zzz", self.start, None),
                ContestLock("abc467", self.start, self.end),
            ],
        )
        replacement_end = self.end + timedelta(hours=1)

        upsert_lock(
            self.path,
            ContestLock("abc467", self.start, replacement_end),
        )

        self.assertEqual(
            load_state(self.path),
            [
                ContestLock("abc467", self.start, replacement_end),
                ContestLock("zzz", self.start, None),
            ],
        )

    def test_upsert_does_not_overwrite_invalid_existing_state(self) -> None:
        original_contents = b"{not-json\n"
        self.path.write_bytes(original_contents)

        with self.assertRaises(StateError):
            upsert_lock(self.path, ContestLock("abc467", self.start, self.end))

        self.assertEqual(self.path.read_bytes(), original_contents)

    def test_upsert_rejects_invalid_new_lock_without_replacing_state(self) -> None:
        original = ContestLock("abc467", self.start, self.end)
        write_state(self.path, [original])
        original_contents = self.path.read_bytes()

        with self.assertRaises(StateError):
            upsert_lock(self.path, object())

        self.assertEqual(self.path.read_bytes(), original_contents)

    def test_write_uses_fsync_and_same_directory_atomic_replace(self) -> None:
        real_replace = os.replace
        replace_calls: list[tuple[Path, Path]] = []

        def observing_replace(source: str, destination: Path) -> None:
            source_path = Path(source)
            destination_path = Path(destination)
            self.assertEqual(source_path.parent, self.path.parent)
            self.assertEqual(destination_path, self.path)
            replace_calls.append((source_path, destination_path))
            real_replace(source, destination)

        with patch("tools.push_guard.os.fsync", wraps=os.fsync) as fsync:
            with patch(
                "tools.push_guard.os.replace", side_effect=observing_replace
            ):
                write_state(
                    self.path,
                    [ContestLock("abc467", self.start, self.end)],
                )

        fsync.assert_called_once()
        self.assertEqual(len(replace_calls), 1)
        self.assertEqual(load_state(self.path)[0].contest_id, "abc467")

    def test_write_cleans_unconsumed_temporary_file(self) -> None:
        original_contents = b"existing\n"
        self.path.write_bytes(original_contents)

        with patch("tools.push_guard.os.replace", side_effect=OSError("busy")):
            with self.assertRaises(StateError):
                write_state(
                    self.path,
                    [ContestLock("abc467", self.start, self.end)],
                )

        self.assertEqual(self.path.read_bytes(), original_contents)
        self.assertEqual(list(self.directory.iterdir()), [self.path])

    def test_write_preserves_primary_failure_when_temp_cleanup_fails(self) -> None:
        replace_error = OSError("primary replace failure")
        cleanup_error = OSError("secondary cleanup failure")

        with patch("tools.push_guard.os.replace", side_effect=replace_error):
            with patch.object(
                Path, "unlink", side_effect=cleanup_error
            ) as unlink:
                with self.assertRaises(StateError) as raised:
                    write_state(
                        self.path,
                        [ContestLock("abc467", self.start, self.end)],
                    )

        unlink.assert_called_once()
        self.assertIs(raised.exception.__cause__, replace_error)
        self.assertIn("primary replace failure", str(raised.exception))
        self.assertNotIn("secondary cleanup failure", str(raised.exception))

    def _write_json(self, value: object) -> None:
        self.path.write_text(json.dumps(value), encoding="utf-8")

    @staticmethod
    def _record(**overrides: object) -> dict[str, object]:
        record: dict[str, object] = {
            "contest_id": "abc467",
            "start_at": "2026-07-18T12:00:00Z",
            "end_at": "2026-07-18T13:40:00Z",
        }
        record.update(overrides)
        return record


class TestContestRegistration(unittest.TestCase):
    def setUp(self) -> None:
        temporary_directory = TemporaryDirectory()
        self.addCleanup(temporary_directory.cleanup)
        self.path = Path(temporary_directory.name) / "atcoder-push-lock.json"
        self.now = datetime(2026, 7, 18, 12, 0, tzinfo=timezone.utc)
        self.start = self.now - timedelta(minutes=10)
        self.end = self.now + timedelta(hours=1, minutes=40)

    def test_parses_exact_jst_manual_end_and_normalizes_to_utc(self) -> None:
        self.assertEqual(
            push_guard.parse_manual_end("2026-07-18 22:40", self.now),
            datetime(2026, 7, 18, 13, 40, tzinfo=timezone.utc),
        )

    def test_manual_end_rejects_invalid_nonfuture_and_naive_now(self) -> None:
        cases = {
            "non-string": (None, self.now),
            "seconds": ("2026-07-18 22:40:00", self.now),
            "wrong separator": ("2026-07-18T22:40", self.now),
            "not zero padded": ("2026-7-18 22:40", self.now),
            "invalid date": ("2026-02-30 22:40", self.now),
            "equal to now": ("2026-07-18 21:00", self.now),
            "before now": ("2026-07-18 20:59", self.now),
            "naive now": (
                "2026-07-18 22:40",
                self.now.replace(tzinfo=None),
            ),
            "non-datetime now": ("2026-07-18 22:40", None),
        }
        for name, (value, now) in cases.items():
            with self.subTest(name=name), self.assertRaises(PushGuardError):
                push_guard.parse_manual_end(value, now)

    def test_set_manual_end_preserves_start_and_other_locks(self) -> None:
        other = ContestLock(
            "other",
            self.start - timedelta(days=1),
            self.start,
        )
        unresolved = ContestLock("abc467", self.start, None)
        write_state(self.path, [other, unresolved])

        push_guard.set_manual_end(
            self.path,
            "abc467",
            "2026-07-18 22:40",
            self.now,
        )

        self.assertEqual(
            load_state(self.path),
            [
                ContestLock(
                    "abc467",
                    self.start,
                    datetime(2026, 7, 18, 13, 40, tzinfo=timezone.utc),
                ),
                other,
            ],
        )

    def test_set_manual_end_refuses_missing_finite_and_invalid_interval(self) -> None:
        cases = {
            "missing": [ContestLock("other", self.start, None)],
            "finite": [ContestLock("abc467", self.start, self.end)],
            "start after end": [
                ContestLock("abc467", self.now + timedelta(hours=2), None)
            ],
        }
        for name, locks in cases.items():
            with self.subTest(name=name):
                write_state(self.path, locks)
                original = self.path.read_bytes()
                with self.assertRaises(PushGuardError):
                    push_guard.set_manual_end(
                        self.path,
                        "abc467",
                        "2026-07-18 22:40",
                        self.now,
                    )
                self.assertEqual(self.path.read_bytes(), original)

    def test_official_schedule_upserts_finite_lock_without_prompting(self) -> None:
        prompt_calls = 0

        def input_value(prompt: str) -> str:
            nonlocal prompt_calls
            prompt_calls += 1
            return "2026-07-18 22:40"

        push_guard.register_contest(
            "abc467",
            self.path,
            fetch_schedule=lambda contest_id: (self.start, self.end),
            input_value=input_value,
            is_interactive=True,
            now=lambda: self.now,
        )

        self.assertEqual(
            load_state(self.path),
            [ContestLock("abc467", self.start, self.end)],
        )
        self.assertEqual(prompt_calls, 0)

    def test_fetch_failure_persists_unresolved_before_prompt_then_resolves(self) -> None:
        prompt_calls = 0

        def fail_fetch(contest_id: str) -> tuple[datetime, datetime]:
            raise PushGuardError("schedule unavailable")

        def input_value(prompt: str) -> str:
            nonlocal prompt_calls
            prompt_calls += 1
            self.assertEqual(
                load_state(self.path),
                [ContestLock("abc467", self.now, None)],
            )
            self.assertIn("JST", prompt)
            self.assertIn("YYYY-MM-DD HH:MM", prompt)
            return "2026-07-18 22:40"

        push_guard.register_contest(
            "abc467",
            self.path,
            fetch_schedule=fail_fetch,
            input_value=input_value,
            is_interactive=True,
            now=lambda: self.now,
        )

        self.assertEqual(prompt_calls, 1)
        self.assertEqual(
            load_state(self.path),
            [
                ContestLock(
                    "abc467",
                    self.now,
                    datetime(2026, 7, 18, 13, 40, tzinfo=timezone.utc),
                )
            ],
        )

    def test_invalid_input_eof_and_interrupt_preserve_unresolved_state(self) -> None:
        cases = {
            "invalid": lambda prompt: "not-a-time",
            "EOF": lambda prompt: (_ for _ in ()).throw(EOFError()),
            "interrupt": lambda prompt: (_ for _ in ()).throw(
                KeyboardInterrupt()
            ),
        }
        for name, input_value in cases.items():
            with self.subTest(name=name):
                if self.path.exists():
                    self.path.unlink()

                def fail_fetch(contest_id: str) -> tuple[datetime, datetime]:
                    raise PushGuardError("schedule unavailable")

                with self.assertRaises(PushGuardError) as raised:
                    push_guard.register_contest(
                        "abc467",
                        self.path,
                        fetch_schedule=fail_fetch,
                        input_value=input_value,
                        is_interactive=True,
                        now=lambda: self.now,
                    )

                self.assertNotIsInstance(raised.exception, StateError)
                self.assertEqual(
                    load_state(self.path),
                    [ContestLock("abc467", self.now, None)],
                )

    def test_noninteractive_failure_does_not_prompt_and_stays_unresolved(self) -> None:
        def fail_fetch(contest_id: str) -> tuple[datetime, datetime]:
            raise PushGuardError("schedule unavailable")

        def input_value(prompt: str) -> str:
            self.fail("noninteractive registration must not prompt")

        with self.assertRaises(PushGuardError):
            push_guard.register_contest(
                "abc467",
                self.path,
                fetch_schedule=fail_fetch,
                input_value=input_value,
                is_interactive=False,
                now=lambda: self.now,
            )

        self.assertEqual(
            load_state(self.path),
            [ContestLock("abc467", self.now, None)],
        )

    def test_unresolved_persistence_failure_is_hard_and_does_not_prompt(self) -> None:
        self.path.write_bytes(b"{corrupt\n")
        original = self.path.read_bytes()

        def fail_fetch(contest_id: str) -> tuple[datetime, datetime]:
            raise PushGuardError("schedule unavailable")

        def input_value(prompt: str) -> str:
            self.fail("registration must persist before prompting")

        with self.assertRaises(StateError):
            push_guard.register_contest(
                "abc467",
                self.path,
                fetch_schedule=fail_fetch,
                input_value=input_value,
                is_interactive=True,
                now=lambda: self.now,
            )

        self.assertEqual(self.path.read_bytes(), original)

    def test_unexpected_fetch_failure_propagates_without_state_mutation(self) -> None:
        unexpected = RuntimeError("programmer error")

        def fail_fetch(contest_id: str) -> tuple[datetime, datetime]:
            raise unexpected

        with self.assertRaises(RuntimeError) as raised:
            push_guard.register_contest(
                "abc467",
                self.path,
                fetch_schedule=fail_fetch,
                input_value=lambda prompt: "2026-07-18 22:40",
                is_interactive=True,
                now=lambda: self.now,
            )

        self.assertIs(raised.exception, unexpected)
        self.assertFalse(self.path.exists())


class TestRepositoryPaths(unittest.TestCase):
    def setUp(self) -> None:
        temporary_directory = TemporaryDirectory()
        self.addCleanup(temporary_directory.cleanup)
        self.root = Path(temporary_directory.name).resolve()

    def test_repository_root_uses_git_rev_parse(self) -> None:
        nested = self.root / "nested"
        nested.mkdir()
        completed = subprocess.CompletedProcess(
            args=[],
            returncode=0,
            stdout=f"{self.root}\n",
            stderr="",
        )

        with patch("tools.push_guard.subprocess.run", return_value=completed) as run:
            result = push_guard.repository_root(nested)

        self.assertEqual(result, self.root)
        run.assert_called_once_with(
            ["git", "rev-parse", "--show-toplevel"],
            cwd=nested,
            check=True,
            capture_output=True,
            text=True,
        )

    def test_repository_and_state_paths_use_real_git_repository(self) -> None:
        self._run_git(["init"])
        nested = self.root / "nested"
        nested.mkdir()

        self.assertEqual(push_guard.repository_root(nested), self.root)
        self.assertEqual(
            push_guard.state_path_for_repository(self.root),
            self.root / ".git" / push_guard.STATE_FILENAME,
        )

    def test_linked_worktree_uses_its_git_state_path(self) -> None:
        main = self.root / "main"
        linked = self.root / "linked"
        main.mkdir()
        self._run_git(["init"], cwd=main)
        tracked = main / "tracked.txt"
        tracked.write_text("initial\n", encoding="utf-8")
        self._run_git(["add", "tracked.txt"], cwd=main)
        self._run_git(
            [
                "-c",
                "user.name=Push Guard Test",
                "-c",
                "user.email=push-guard@example.invalid",
                "commit",
                "-m",
                "initial",
            ],
            cwd=main,
        )
        added = subprocess.run(
            [
                "git",
                "worktree",
                "add",
                "-b",
                "push-guard-linked-state-test",
                str(linked),
            ],
            cwd=main,
            check=False,
            capture_output=True,
            text=True,
        )
        if added.returncode != 0:
            self.skipTest(f"linked worktrees unavailable: {added.stderr.strip()}")

        try:
            nested = linked / "nested"
            nested.mkdir()
            raw_state_path = self._run_git(
                ["rev-parse", "--git-path", push_guard.STATE_FILENAME],
                cwd=linked,
            ).stdout.strip()
            expected = Path(raw_state_path)
            if not expected.is_absolute():
                expected = linked / expected

            self.assertEqual(push_guard.repository_root(nested), linked.resolve())
            self.assertEqual(
                push_guard.state_path_for_repository(linked),
                expected.resolve(),
            )
        finally:
            subprocess.run(
                ["git", "worktree", "remove", "--force", str(linked)],
                cwd=main,
                check=False,
                capture_output=True,
                text=True,
            )

    def test_state_path_resolves_relative_git_path_against_root(self) -> None:
        completed = subprocess.CompletedProcess(
            args=[],
            returncode=0,
            stdout=".git/atcoder-push-lock.json\n",
            stderr="",
        )

        with patch("tools.push_guard.subprocess.run", return_value=completed) as run:
            result = push_guard.state_path_for_repository(self.root)

        self.assertEqual(
            result,
            (self.root / ".git" / "atcoder-push-lock.json").resolve(),
        )
        run.assert_called_once_with(
            ["git", "rev-parse", "--git-path", "atcoder-push-lock.json"],
            cwd=self.root,
            check=True,
            capture_output=True,
            text=True,
        )

    def test_repository_paths_reject_git_failures_and_empty_output(self) -> None:
        git_error = subprocess.CalledProcessError(
            128,
            ["git", "rev-parse"],
            stderr="not a repository",
        )
        with patch("tools.push_guard.subprocess.run", side_effect=git_error):
            with self.assertRaises(PushGuardError):
                push_guard.repository_root(self.root)

        completed = subprocess.CompletedProcess(
            args=[], returncode=0, stdout="\n", stderr=""
        )
        with patch("tools.push_guard.subprocess.run", return_value=completed):
            with self.assertRaises(PushGuardError):
                push_guard.state_path_for_repository(self.root)

    def test_git_helpers_wrap_text_decode_failures(self) -> None:
        operations = {
            "repository root": lambda: push_guard.repository_root(self.root),
            "state path": lambda: push_guard.state_path_for_repository(self.root),
            "hook inspection": lambda: push_guard.guard_is_installed(self.root),
        }
        for name, operation in operations.items():
            decode_error = UnicodeDecodeError(
                "utf-8",
                b"\xff",
                0,
                1,
                "invalid start byte",
            )
            with self.subTest(name=name):
                with patch(
                    "tools.push_guard.subprocess.run",
                    side_effect=decode_error,
                ):
                    with self.assertRaises(PushGuardError) as raised:
                        operation()
                self.assertIs(raised.exception.__cause__, decode_error)

    def test_git_helpers_wrap_path_resolution_runtime_errors(self) -> None:
        operations = {
            "repository root": (
                f"{self.root}\n",
                lambda: push_guard.repository_root(self.root),
            ),
            "state path": (
                ".git/atcoder-push-lock.json\n",
                lambda: push_guard.state_path_for_repository(self.root),
            ),
            "hook inspection": (
                ".githooks\n",
                lambda: push_guard.guard_is_installed(self.root),
            ),
        }
        for name, (stdout, operation) in operations.items():
            completed = subprocess.CompletedProcess(
                args=[],
                returncode=0,
                stdout=stdout,
                stderr="",
            )
            resolution_error = RuntimeError("symlink loop")
            with self.subTest(name=name):
                with patch(
                    "tools.push_guard.subprocess.run",
                    return_value=completed,
                ):
                    with patch.object(
                        Path,
                        "resolve",
                        side_effect=resolution_error,
                    ):
                        with self.assertRaises(PushGuardError) as raised:
                            operation()
                self.assertIs(raised.exception.__cause__, resolution_error)

    def test_guard_installation_check_is_read_only_and_requires_executable_hook(
        self,
    ) -> None:
        hooks = self.root / ".githooks"
        hooks.mkdir()
        hook = hooks / "pre-push"
        hook.write_text("#!/bin/sh\nexit 0\n", encoding="utf-8")
        hook.chmod(0o755)
        configured = subprocess.CompletedProcess(
            args=[], returncode=0, stdout=".githooks\n", stderr=""
        )

        with patch("tools.push_guard.subprocess.run", return_value=configured):
            self.assertTrue(push_guard.guard_is_installed(self.root))

        hook.chmod(0o644)
        with patch("tools.push_guard.subprocess.run", return_value=configured):
            self.assertFalse(push_guard.guard_is_installed(self.root))

    def _run_git(
        self,
        arguments: list[str],
        *,
        cwd: Path | None = None,
    ) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            ["git", *arguments],
            cwd=self.root if cwd is None else cwd,
            check=True,
            capture_output=True,
            text=True,
        )


class TestHookInstallation(unittest.TestCase):
    def setUp(self) -> None:
        temporary_directory = TemporaryDirectory()
        self.addCleanup(temporary_directory.cleanup)
        self.directory = Path(temporary_directory.name).resolve()
        self.root = self.directory / "main"
        self.root.mkdir()
        self._run_git(["init"])

    def test_installs_unset_local_hooks_path(self) -> None:
        self._write_executable_hook()

        push_guard.install_hook(self.root)

        self.assertEqual(self._configured_hooks_path(), ".githooks")
        self.assertTrue(push_guard.guard_is_installed(self.root))

    def test_repeated_install_does_not_mutate_configuration(self) -> None:
        self._write_executable_hook()
        push_guard.install_hook(self.root)
        real_run = subprocess.run
        commands: list[list[str]] = []

        def record_run(command: list[str], **kwargs: object) -> object:
            commands.append(command)
            return real_run(command, **kwargs)

        with patch("tools.push_guard.subprocess.run", side_effect=record_run):
            push_guard.install_hook(self.root)

        set_command = [
            "git",
            "config",
            "--local",
            "core.hooksPath",
            push_guard.HOOKS_PATH_VALUE,
        ]
        self.assertNotIn(set_command, commands)
        self.assertEqual(self._configured_hooks_path(), ".githooks")

    def test_accepts_absolute_path_resolving_to_expected_hooks_directory(
        self,
    ) -> None:
        self._write_executable_hook()
        absolute_path = str((self.root / ".githooks").resolve())
        self._run_git(["config", "--local", "core.hooksPath", absolute_path])

        push_guard.install_hook(self.root)

        self.assertEqual(self._configured_hooks_path(), absolute_path)
        self.assertTrue(push_guard.guard_is_installed(self.root))

    def test_refuses_different_hooks_path_without_changing_it(self) -> None:
        self._write_executable_hook()
        self._run_git(["config", "--local", "core.hooksPath", "other-hooks"])

        with self.assertRaisesRegex(PushGuardError, "already configured"):
            push_guard.install_hook(self.root)

        self.assertEqual(self._configured_hooks_path(), "other-hooks")

    def test_does_not_trim_whitespace_from_configured_hooks_path(self) -> None:
        self._write_executable_hook()
        configured_value = " .githooks "
        self._run_git(
            ["config", "--local", "core.hooksPath", configured_value]
        )

        with self.assertRaisesRegex(PushGuardError, "already configured"):
            push_guard.install_hook(self.root)

        self.assertEqual(self._configured_hooks_path(), configured_value)

    def test_refuses_conflicting_effective_worktree_hooks_path(self) -> None:
        linked = self._create_linked_worktree()
        self._write_executable_hook(linked)
        self._run_git(
            ["config", "--worktree", "core.hooksPath", "other-hooks"],
            cwd=linked,
        )

        try:
            with self.assertRaisesRegex(PushGuardError, "already configured"):
                push_guard.install_hook(linked)

            self.assertFalse(push_guard.guard_is_installed(linked))
            self.assertEqual(
                self._git_config_value(
                    ["config", "--get", "core.hooksPath"], cwd=linked
                ),
                "other-hooks",
            )
            self.assertIsNone(
                self._git_config_value(
                    ["config", "--local", "--get", "core.hooksPath"],
                    cwd=linked,
                )
            )
        finally:
            self._remove_linked_worktree(linked)

    def test_accepts_expected_effective_worktree_path_without_local_write(
        self,
    ) -> None:
        linked = self._create_linked_worktree()
        self._write_executable_hook(linked)
        self._run_git(
            [
                "config",
                "--worktree",
                "core.hooksPath",
                push_guard.HOOKS_PATH_VALUE,
            ],
            cwd=linked,
        )

        try:
            push_guard.install_hook(linked)

            self.assertTrue(push_guard.guard_is_installed(linked))
            self.assertEqual(
                self._git_config_value(
                    ["config", "--get", "core.hooksPath"], cwd=linked
                ),
                push_guard.HOOKS_PATH_VALUE,
            )
            self.assertIsNone(
                self._git_config_value(
                    ["config", "--local", "--get", "core.hooksPath"],
                    cwd=linked,
                )
            )
        finally:
            self._remove_linked_worktree(linked)

    def test_refuses_effective_hooks_path_from_local_include(self) -> None:
        self._write_executable_hook()
        include_path = self.directory / "included-hooks.config"
        self._run_git(
            [
                "config",
                "-f",
                str(include_path),
                "core.hooksPath",
                "included-hooks",
            ]
        )
        self._run_git(
            ["config", "--local", "include.path", str(include_path)]
        )
        effective = self._git_config_value(
            ["config", "--get", "core.hooksPath"]
        )
        if effective != "included-hooks":
            self.skipTest("Git does not resolve core.hooksPath from local include")

        with self.assertRaisesRegex(PushGuardError, "already configured"):
            push_guard.install_hook(self.root)

        self.assertFalse(push_guard.guard_is_installed(self.root))
        self.assertEqual(
            self._git_config_value(["config", "--get", "core.hooksPath"]),
            "included-hooks",
        )
        self.assertIsNone(
            self._git_config_value(
                ["config", "--local", "--get", "core.hooksPath"]
            )
        )

    def test_missing_hook_prevents_configuration(self) -> None:
        with self.assertRaisesRegex(PushGuardError, "pre-push"):
            push_guard.install_hook(self.root)

        self.assertIsNone(self._configured_hooks_path())

    def test_directory_in_place_of_hook_prevents_configuration(self) -> None:
        hook = self.root / ".githooks" / "pre-push"
        hook.mkdir(parents=True)

        with self.assertRaisesRegex(PushGuardError, "regular file"):
            push_guard.install_hook(self.root)

        self.assertIsNone(self._configured_hooks_path())

    def test_non_executable_hook_prevents_configuration(self) -> None:
        hook = self._write_executable_hook()
        hook.chmod(0o644)

        with self.assertRaisesRegex(PushGuardError, "executable"):
            push_guard.install_hook(self.root)

        self.assertIsNone(self._configured_hooks_path())

    def test_non_repository_is_rejected_before_configuration(self) -> None:
        temporary_directory = TemporaryDirectory()
        self.addCleanup(temporary_directory.cleanup)
        non_repository = Path(temporary_directory.name).resolve()
        self._write_executable_hook(non_repository)

        with self.assertRaisesRegex(PushGuardError, "Git path resolution"):
            push_guard.install_hook(non_repository)

    def test_config_get_failures_are_wrapped_without_mutation(self) -> None:
        self._write_executable_hook()
        real_run = subprocess.run
        decode_error = UnicodeDecodeError(
            "utf-8", b"\xff", 0, 1, "invalid start byte"
        )
        failures: dict[str, object] = {
            "nonzero": subprocess.CompletedProcess(
                args=[], returncode=2, stdout="", stderr="config failed"
            ),
            "decode": decode_error,
        }
        get_command = [
            "git",
            "config",
            "--get",
            "core.hooksPath",
        ]
        for name, failure in failures.items():
            def fail_get(command: list[str], **kwargs: object) -> object:
                if command == get_command:
                    if isinstance(failure, BaseException):
                        raise failure
                    return failure
                return real_run(command, **kwargs)

            with self.subTest(name=name):
                with patch(
                    "tools.push_guard.subprocess.run", side_effect=fail_get
                ):
                    with self.assertRaises(PushGuardError):
                        push_guard.install_hook(self.root)
                self.assertIsNone(self._configured_hooks_path())

    def test_config_set_failure_is_wrapped_and_remains_unset(self) -> None:
        self._write_executable_hook()
        real_run = subprocess.run
        set_command = [
            "git",
            "config",
            "--local",
            "core.hooksPath",
            push_guard.HOOKS_PATH_VALUE,
        ]

        def fail_set(command: list[str], **kwargs: object) -> object:
            if command == set_command:
                return subprocess.CompletedProcess(
                    args=command,
                    returncode=5,
                    stdout="",
                    stderr="config write failed",
                )
            return real_run(command, **kwargs)

        with patch("tools.push_guard.subprocess.run", side_effect=fail_set):
            with self.assertRaisesRegex(PushGuardError, "config write failed"):
                push_guard.install_hook(self.root)

        self.assertIsNone(self._configured_hooks_path())

    def test_configured_path_resolution_failure_is_wrapped(self) -> None:
        self._write_executable_hook()
        self._run_git(["config", "--local", "core.hooksPath", ".githooks"])
        resolution_error = RuntimeError("symlink loop")

        with patch.object(Path, "resolve", side_effect=resolution_error):
            with self.assertRaises(PushGuardError) as raised:
                push_guard.install_hook(self.root)

        self.assertIs(raised.exception.__cause__, resolution_error)

    def test_committed_hook_has_exact_contents_mode_and_valid_syntax(self) -> None:
        hook = Path(__file__).parents[1] / ".githooks" / "pre-push"
        expected = (
            "#!/bin/sh\n"
            "\n"
            "repo_root=$(git rev-parse --show-toplevel) || exit 1\n"
            'exec python3 "$repo_root/tools/push_guard.py" check\n'
        )

        self.assertEqual(hook.read_text(encoding="utf-8"), expected)
        self.assertTrue(stat.S_ISREG(hook.stat().st_mode))
        self.assertTrue(os.access(hook, os.X_OK))
        subprocess.run(
            ["/bin/sh", "-n", str(hook)],
            check=True,
            capture_output=True,
            text=True,
        )

    def _write_executable_hook(self, root: Path | None = None) -> Path:
        target_root = self.root if root is None else root
        hooks = target_root / ".githooks"
        hooks.mkdir()
        hook = hooks / "pre-push"
        hook.write_text("#!/bin/sh\nexit 0\n", encoding="utf-8")
        hook.chmod(0o755)
        return hook

    def _configured_hooks_path(self) -> str | None:
        return self._git_config_value(
            ["config", "--local", "--get", "core.hooksPath"]
        )

    def _git_config_value(
        self,
        arguments: list[str],
        *,
        cwd: Path | None = None,
    ) -> str | None:
        result = self._run_git(arguments, check=False, cwd=cwd)
        if result.returncode == 1:
            return None
        self.assertEqual(result.returncode, 0, result.stderr)
        if result.stdout.endswith("\n"):
            return result.stdout[:-1]
        return result.stdout

    def _run_git(
        self,
        arguments: list[str],
        *,
        check: bool = True,
        cwd: Path | None = None,
    ) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            ["git", *arguments],
            cwd=self.root if cwd is None else cwd,
            check=check,
            capture_output=True,
            text=True,
        )

    def _create_linked_worktree(self) -> Path:
        self._run_git(
            [
                "-c",
                "user.name=Push Guard Test",
                "-c",
                "user.email=push-guard@example.invalid",
                "commit",
                "--allow-empty",
                "-m",
                "initial",
            ]
        )
        self._run_git(["config", "extensions.worktreeConfig", "true"])
        linked = self.directory / "linked"
        added = self._run_git(
            [
                "worktree",
                "add",
                "-b",
                "push-guard-install-linked-test",
                str(linked),
            ],
            check=False,
        )
        if added.returncode != 0:
            self.skipTest(f"linked worktrees unavailable: {added.stderr.strip()}")
        return linked

    def _remove_linked_worktree(self, linked: Path) -> None:
        self._run_git(
            ["worktree", "remove", "--force", str(linked)],
            check=False,
        )


class TestPushGuardCli(unittest.TestCase):
    def setUp(self) -> None:
        temporary_directory = TemporaryDirectory()
        self.addCleanup(temporary_directory.cleanup)
        self.root = Path(temporary_directory.name)
        self.path = self.root / "atcoder-push-lock.json"
        self.now = datetime(2026, 7, 18, 12, 0, tzinfo=timezone.utc)

    def test_check_allows_absent_empty_and_expired_state_silently(self) -> None:
        expired = ContestLock(
            "expired",
            self.now - timedelta(hours=2),
            self.now - timedelta(hours=1),
        )
        cases: dict[str, list[ContestLock] | None] = {
            "absent": None,
            "empty": [],
            "expired": [expired],
        }
        for name, locks in cases.items():
            with self.subTest(name=name):
                self.path.unlink(missing_ok=True)
                if locks is not None:
                    write_state(self.path, locks)

                result, stdout, stderr = self._run_main(["check"])

                self.assertEqual(result, 0)
                self.assertEqual(stdout, "")
                self.assertEqual(stderr, "")

    def test_check_prints_every_blocking_lock_sorted(self) -> None:
        write_state(
            self.path,
            [
                ContestLock(
                    "zzz-active",
                    self.now,
                    self.now + timedelta(hours=1),
                ),
                ContestLock("aaa-unresolved", self.now, None),
                ContestLock(
                    "future",
                    self.now + timedelta(hours=1),
                    self.now + timedelta(hours=2),
                ),
            ],
        )

        result, stdout, stderr = self._run_main(["check"])

        self.assertEqual(result, 1)
        self.assertEqual(stdout, "")
        self.assertEqual(
            stderr,
            "push blocked: aaa-unresolved (unresolved end time)\n"
            "push blocked: zzz-active (active until 2026-07-18T13:00:00Z)\n",
        )

    def test_check_corrupt_state_prints_one_state_diagnostic(self) -> None:
        self.path.write_bytes(b"{not-json\n")

        result, stdout, stderr = self._run_main(["check"])

        self.assertEqual(result, 1)
        self.assertEqual(stdout, "")
        self.assertTrue(stderr.startswith("push blocked: invalid state: malformed"))
        self.assertEqual(stderr.count("\n"), 1)
        self.assertNotIn("Traceback", stderr)

    def test_status_prints_installation_and_all_lock_statuses_sorted(self) -> None:
        write_state(
            self.path,
            [
                ContestLock(
                    "upcoming",
                    self.now + timedelta(hours=1),
                    self.now + timedelta(hours=2),
                ),
                ContestLock("unresolved", self.now, None),
                ContestLock(
                    "expired",
                    self.now - timedelta(hours=2),
                    self.now - timedelta(hours=1),
                ),
                ContestLock(
                    "active",
                    self.now,
                    self.now + timedelta(hours=1),
                ),
            ],
        )

        result, stdout, stderr = self._run_main(["status"], installed=True)

        self.assertEqual(result, 0)
        self.assertEqual(stderr, "")
        self.assertEqual(
            stdout,
            "hook: installed\n"
            "active: active (start=2026-07-18T12:00:00Z, "
            "end=2026-07-18T13:00:00Z)\n"
            "expired: expired (start=2026-07-18T10:00:00Z, "
            "end=2026-07-18T11:00:00Z)\n"
            "unresolved: unresolved (start=2026-07-18T12:00:00Z, "
            "end=unresolved)\n"
            "upcoming: upcoming (start=2026-07-18T13:00:00Z, "
            "end=2026-07-18T14:00:00Z)\n",
        )

    def test_status_invalid_state_keeps_hook_line_and_returns_nonzero(self) -> None:
        self.path.write_bytes(b"{not-json\n")

        result, stdout, stderr = self._run_main(["status"], installed=False)

        self.assertEqual(result, 1)
        self.assertEqual(stdout, "hook: not installed\n")
        self.assertTrue(stderr.startswith("invalid state: malformed"))
        self.assertEqual(stderr.count("\n"), 1)

    def test_set_end_command_resolves_only_unresolved_lock(self) -> None:
        write_state(self.path, [ContestLock("abc467", self.now, None)])

        result, stdout, stderr = self._run_main(
            ["set-end", "abc467", "2026-07-18 22:40"]
        )

        self.assertEqual(result, 0)
        self.assertEqual(stdout, "")
        self.assertEqual(stderr, "")
        self.assertEqual(
            load_state(self.path),
            [
                ContestLock(
                    "abc467",
                    self.now,
                    datetime(2026, 7, 18, 13, 40, tzinfo=timezone.utc),
                )
            ],
        )

    def test_operational_error_returns_one_without_traceback(self) -> None:
        result, stdout, stderr = self._run_main(
            ["set-end", "missing", "2026-07-18 22:40"]
        )

        self.assertEqual(result, 1)
        self.assertEqual(stdout, "")
        self.assertTrue(stderr.startswith("push-guard: "))
        self.assertNotIn("Traceback", stderr)

    def test_install_command_succeeds_first_and_repeated_time(self) -> None:
        self._initialize_git_repository_with_hook()

        first = self._run_unmocked_main(["install"])
        repeated = self._run_unmocked_main(["install"])

        self.assertEqual(first, (0, "", ""))
        self.assertEqual(repeated, (0, "", ""))
        configured = subprocess.run(
            ["git", "config", "--local", "--get", "core.hooksPath"],
            cwd=self.root,
            check=True,
            capture_output=True,
            text=True,
        )
        self.assertEqual(configured.stdout, ".githooks\n")

    def test_install_command_reports_conflict_without_changing_config(self) -> None:
        self._initialize_git_repository_with_hook()
        subprocess.run(
            ["git", "config", "--local", "core.hooksPath", "other-hooks"],
            cwd=self.root,
            check=True,
            capture_output=True,
            text=True,
        )

        result, stdout, stderr = self._run_unmocked_main(["install"])

        self.assertEqual(result, 1)
        self.assertEqual(stdout, "")
        self.assertTrue(stderr.startswith("push-guard: "))
        self.assertEqual(stderr.count("\n"), 1)
        configured = subprocess.run(
            ["git", "config", "--local", "--get", "core.hooksPath"],
            cwd=self.root,
            check=True,
            capture_output=True,
            text=True,
        )
        self.assertEqual(configured.stdout, "other-hooks\n")

    def test_install_command_reports_invalid_hook_without_configuration(
        self,
    ) -> None:
        subprocess.run(
            ["git", "init"],
            cwd=self.root,
            check=True,
            capture_output=True,
            text=True,
        )

        result, stdout, stderr = self._run_unmocked_main(["install"])

        self.assertEqual(result, 1)
        self.assertEqual(stdout, "")
        self.assertTrue(stderr.startswith("push-guard: "))
        self.assertIn("pre-push", stderr)
        configured = subprocess.run(
            ["git", "config", "--local", "--get", "core.hooksPath"],
            cwd=self.root,
            check=False,
            capture_output=True,
            text=True,
        )
        self.assertEqual(configured.returncode, 1)

    def test_repository_error_returns_one_without_traceback(self) -> None:
        stdout = StringIO()
        stderr = StringIO()
        with patch(
            "tools.push_guard.repository_root",
            side_effect=PushGuardError("not a repository"),
        ):
            with redirect_stdout(stdout), redirect_stderr(stderr):
                result = push_guard.main(
                    ["check"], cwd=self.root, now=lambda: self.now
                )

        self.assertEqual(result, 1)
        self.assertEqual(stdout.getvalue(), "")
        self.assertEqual(stderr.getvalue(), "push-guard: not a repository\n")

    def test_argparse_errors_keep_usage_exit_code_two(self) -> None:
        stderr = StringIO()
        with redirect_stderr(stderr):
            with self.assertRaises(SystemExit) as raised:
                push_guard.main([], cwd=self.root, now=lambda: self.now)

        self.assertEqual(raised.exception.code, 2)
        self.assertIn("usage:", stderr.getvalue())

    def test_script_entrypoint_preserves_argparse_exit_code_two(self) -> None:
        result = subprocess.run(
            [sys.executable, "tools/push_guard.py"],
            cwd=Path(__file__).parents[1],
            check=False,
            capture_output=True,
            text=True,
        )

        self.assertEqual(result.returncode, 2)
        self.assertIn("usage:", result.stderr)

    def _run_main(
        self,
        argv: list[str],
        *,
        installed: bool = False,
    ) -> tuple[int, str, str]:
        stdout = StringIO()
        stderr = StringIO()
        with patch("tools.push_guard.repository_root", return_value=self.root):
            with patch(
                "tools.push_guard.state_path_for_repository",
                return_value=self.path,
            ):
                with patch(
                    "tools.push_guard.guard_is_installed",
                    return_value=installed,
                ):
                    with redirect_stdout(stdout), redirect_stderr(stderr):
                        result = push_guard.main(
                            argv,
                            cwd=self.root,
                            now=lambda: self.now,
                        )
        return result, stdout.getvalue(), stderr.getvalue()

    def _run_unmocked_main(
        self,
        argv: list[str],
    ) -> tuple[int, str, str]:
        stdout = StringIO()
        stderr = StringIO()
        with redirect_stdout(stdout), redirect_stderr(stderr):
            result = push_guard.main(
                argv,
                cwd=self.root,
                now=lambda: self.now,
            )
        return result, stdout.getvalue(), stderr.getvalue()

    def _initialize_git_repository_with_hook(self) -> None:
        subprocess.run(
            ["git", "init"],
            cwd=self.root,
            check=True,
            capture_output=True,
            text=True,
        )
        hooks = self.root / ".githooks"
        hooks.mkdir()
        hook = hooks / "pre-push"
        hook.write_text("#!/bin/sh\nexit 0\n", encoding="utf-8")
        hook.chmod(0o755)


class TestStateRecovery(unittest.TestCase):
    def setUp(self) -> None:
        temporary_directory = TemporaryDirectory()
        self.addCleanup(temporary_directory.cleanup)
        self.root = Path(temporary_directory.name)
        self.path = self.root / "atcoder-push-lock.json"
        self.now = datetime(2026, 7, 18, 12, 0, tzinfo=timezone.utc)
        self.manual_end = "2026-07-18 22:40"
        self.corrupt_contents = b"{corrupt-state\n"

    def test_refuses_absent_or_valid_state_without_backup(self) -> None:
        with self.assertRaises(PushGuardError):
            push_guard.recover_state(
                self.path,
                "abc467",
                self.manual_end,
                self.now,
            )
        self.assertEqual(list(self.root.iterdir()), [])

        write_state(self.path, [])
        valid_contents = self.path.read_bytes()
        with self.assertRaises(PushGuardError):
            push_guard.recover_state(
                self.path,
                "abc467",
                self.manual_end,
                self.now,
            )
        self.assertEqual(self.path.read_bytes(), valid_contents)
        self.assertEqual(list(self.root.iterdir()), [self.path])

    def test_validates_contest_id_and_end_before_mutation(self) -> None:
        cases = {
            "invalid ID": ("../abc467", self.manual_end, self.now),
            "invalid end": ("abc467", "not-a-time", self.now),
            "past end": ("abc467", "2026-07-18 20:00", self.now),
            "naive now": (
                "abc467",
                self.manual_end,
                self.now.replace(tzinfo=None),
            ),
        }
        for name, (contest_id, manual_end, now) in cases.items():
            with self.subTest(name=name):
                self.path.write_bytes(self.corrupt_contents)

                with self.assertRaises(PushGuardError):
                    push_guard.recover_state(
                        self.path,
                        contest_id,
                        manual_end,
                        now,
                    )

                self.assertEqual(self.path.read_bytes(), self.corrupt_contents)
                self.assertEqual(list(self.root.iterdir()), [self.path])

    def test_copies_unique_timestamped_backup_before_atomic_replacement(
        self,
    ) -> None:
        self.path.write_bytes(self.corrupt_contents)
        events: list[str] = []
        real_copy2 = shutil.copy2
        real_replace = os.replace

        def observing_copy2(source: Path, destination: Path) -> Path:
            self.assertEqual(Path(source), self.path)
            self.assertTrue(self.path.exists())
            self.assertEqual(self.path.read_bytes(), self.corrupt_contents)
            events.append("copy")
            return Path(real_copy2(source, destination))

        def observing_replace(source: str, destination: Path) -> None:
            self.assertEqual(events, ["copy"])
            self.assertEqual(Path(destination), self.path)
            self.assertTrue(self.path.exists())
            self.assertEqual(self.path.read_bytes(), self.corrupt_contents)
            events.append("replace")
            real_replace(source, destination)

        with patch("tools.push_guard.shutil.copy2", side_effect=observing_copy2):
            with patch(
                "tools.push_guard.os.replace", side_effect=observing_replace
            ):
                push_guard.recover_state(
                    self.path,
                    "abc467",
                    self.manual_end,
                    self.now,
                )

        self.assertEqual(events, ["copy", "replace"])
        backups = list(
            self.root.glob("atcoder-push-lock.corrupt-*.json")
        )
        self.assertEqual(len(backups), 1)
        self.assertTrue(
            backups[0].name.startswith(
                "atcoder-push-lock.corrupt-20260718T120000Z-"
            )
        )
        self.assertEqual(backups[0].read_bytes(), self.corrupt_contents)
        self.assertEqual(
            load_state(self.path),
            [
                ContestLock(
                    "abc467",
                    self.now,
                    datetime(2026, 7, 18, 13, 40, tzinfo=timezone.utc),
                )
            ],
        )

    def test_same_timestamp_recoveries_never_overwrite_prior_backup(self) -> None:
        first_contents = b"{first-corrupt\n"
        second_contents = b"{second-corrupt\n"
        self.path.write_bytes(first_contents)
        push_guard.recover_state(
            self.path,
            "abc467",
            self.manual_end,
            self.now,
        )
        first_backup = next(
            self.root.glob("atcoder-push-lock.corrupt-*.json")
        )

        self.path.write_bytes(second_contents)
        push_guard.recover_state(
            self.path,
            "abc468",
            self.manual_end,
            self.now,
        )

        backups = list(
            self.root.glob("atcoder-push-lock.corrupt-*.json")
        )
        self.assertEqual(len(backups), 2)
        self.assertEqual(first_backup.read_bytes(), first_contents)
        self.assertEqual(
            {backup.read_bytes() for backup in backups},
            {first_contents, second_contents},
        )

    def test_copy_failure_deletes_reserved_backup_and_preserves_original(
        self,
    ) -> None:
        self.path.write_bytes(self.corrupt_contents)
        copy_error = OSError("copy failed")

        with patch("tools.push_guard.shutil.copy2", side_effect=copy_error):
            with self.assertRaises(StateError) as raised:
                push_guard.recover_state(
                    self.path,
                    "abc467",
                    self.manual_end,
                    self.now,
                )

        self.assertIs(raised.exception.__cause__, copy_error)
        self.assertEqual(self.path.read_bytes(), self.corrupt_contents)
        self.assertEqual(list(self.root.iterdir()), [self.path])

    def test_keyboard_interrupt_during_copy_deletes_backup_and_propagates(
        self,
    ) -> None:
        self.path.write_bytes(self.corrupt_contents)
        interrupt = KeyboardInterrupt()

        with patch("tools.push_guard.shutil.copy2", side_effect=interrupt):
            with self.assertRaises(KeyboardInterrupt) as raised:
                push_guard.recover_state(
                    self.path,
                    "abc467",
                    self.manual_end,
                    self.now,
                )

        self.assertIs(raised.exception, interrupt)
        self.assertEqual(self.path.read_bytes(), self.corrupt_contents)
        self.assertEqual(list(self.root.iterdir()), [self.path])

    def test_copy_and_cleanup_failures_report_incomplete_backup(self) -> None:
        self.path.write_bytes(self.corrupt_contents)
        copy_error = OSError("copy failed")
        cleanup_error = OSError("cleanup failed")

        with patch("tools.push_guard.shutil.copy2", side_effect=copy_error):
            with patch.object(
                Path,
                "unlink",
                side_effect=cleanup_error,
            ) as unlink:
                with self.assertRaises(StateError) as raised:
                    push_guard.recover_state(
                        self.path,
                        "abc467",
                        self.manual_end,
                        self.now,
                    )

        unlink.assert_called_once()
        self.assertIs(raised.exception.__cause__, copy_error)
        backups = list(self.root.glob("atcoder-push-lock.corrupt-*.json"))
        self.assertEqual(len(backups), 1)
        message = str(raised.exception)
        self.assertIn("copy failed", message)
        self.assertIn(str(backups[0]), message)
        self.assertIn("cleanup failed", message)
        self.assertEqual(self.path.read_bytes(), self.corrupt_contents)

    def test_raw_copy_failure_preserves_instance_and_chains_cleanup_context(
        self,
    ) -> None:
        self.path.write_bytes(self.corrupt_contents)
        copy_error = _RawCopyFailure("raw copy failure")
        cleanup_error = OSError("cleanup failed")

        with patch("tools.push_guard.shutil.copy2", side_effect=copy_error):
            with patch.object(
                Path,
                "unlink",
                side_effect=cleanup_error,
            ) as unlink:
                with self.assertRaises(_RawCopyFailure) as raised:
                    push_guard.recover_state(
                        self.path,
                        "abc467",
                        self.manual_end,
                        self.now,
                    )

        unlink.assert_called_once()
        self.assertIs(raised.exception, copy_error)
        cleanup_context = raised.exception.__cause__
        self.assertIsInstance(cleanup_context, StateError)
        backups = list(self.root.glob("atcoder-push-lock.corrupt-*.json"))
        self.assertEqual(len(backups), 1)
        self.assertIn(str(backups[0]), str(cleanup_context))
        self.assertIn("cleanup failed", str(cleanup_context))
        self.assertIs(cleanup_context.__cause__, cleanup_error)
        self.assertEqual(self.path.read_bytes(), self.corrupt_contents)

    def test_write_failure_preserves_original_and_completed_backup(self) -> None:
        self.path.write_bytes(self.corrupt_contents)
        replace_error = OSError("replace failed")

        with patch("tools.push_guard.os.replace", side_effect=replace_error):
            with self.assertRaises(StateError):
                push_guard.recover_state(
                    self.path,
                    "abc467",
                    self.manual_end,
                    self.now,
                )

        self.assertEqual(self.path.read_bytes(), self.corrupt_contents)
        backups = list(
            self.root.glob("atcoder-push-lock.corrupt-*.json")
        )
        self.assertEqual(len(backups), 1)
        self.assertEqual(backups[0].read_bytes(), self.corrupt_contents)
        self.assertEqual(
            sorted(item.name for item in self.root.iterdir()),
            sorted([self.path.name, backups[0].name]),
        )

    def test_recover_state_cli_replaces_corrupt_state(self) -> None:
        self.path.write_bytes(self.corrupt_contents)
        stdout = StringIO()
        stderr = StringIO()
        with patch("tools.push_guard.repository_root", return_value=self.root):
            with patch(
                "tools.push_guard.state_path_for_repository",
                return_value=self.path,
            ):
                with redirect_stdout(stdout), redirect_stderr(stderr):
                    result = push_guard.main(
                        [
                            "recover-state",
                            "abc467",
                            self.manual_end,
                        ],
                        cwd=self.root,
                        now=lambda: self.now,
                    )

        self.assertEqual(result, 0)
        self.assertEqual(stdout.getvalue(), "")
        self.assertEqual(stderr.getvalue(), "")
        self.assertEqual(load_state(self.path)[0].contest_id, "abc467")


class TestScheduleParsing(unittest.TestCase):
    def test_parses_scoped_times_and_normalizes_to_utc(self) -> None:
        html = (FIXTURES / "atcoder_contest_duration.html").read_text(
            encoding="utf-8"
        )

        start_at, end_at = parse_contest_schedule(html)

        self.assertEqual(
            start_at,
            datetime(2026, 7, 18, 12, 0, tzinfo=timezone.utc),
        )
        self.assertEqual(
            end_at,
            datetime(2026, 7, 18, 13, 40, tzinfo=timezone.utc),
        )

    def test_rejects_unscoped_only_time(self) -> None:
        html = (
            "<time class='fixtime fixtime-full'>"
            "2026-01-01 00:00:00+0900"
            "</time>"
        )

        with self.assertRaises(PushGuardError):
            parse_contest_schedule(html)

    def test_rejects_wrong_number_of_scoped_times(self) -> None:
        cases = {
            "one": ["2026-07-18 21:00:00+0900"],
            "three": [
                "2026-07-18 21:00:00+0900",
                "2026-07-18 22:00:00+0900",
                "2026-07-18 22:40:00+0900",
            ],
        }
        for name, values in cases.items():
            times = "".join(
                f"<time class='fixtime fixtime-full'>{value}</time>"
                for value in values
            )
            html = f"<small class='contest-duration'>{times}</small>"

            with self.subTest(name=name), self.assertRaises(PushGuardError):
                parse_contest_schedule(html)

    def test_rejects_malformed_timestamp(self) -> None:
        html = self._duration_html(
            "not-a-timestamp",
            "2026-07-18 22:40:00+0900",
        )

        with self.assertRaises(PushGuardError):
            parse_contest_schedule(html)

    def test_rejects_nonincreasing_interval(self) -> None:
        cases = {
            "equal": (
                "2026-07-18 21:00:00+0900",
                "2026-07-18 21:00:00+0900",
            ),
            "reversed": (
                "2026-07-18 22:40:00+0900",
                "2026-07-18 21:00:00+0900",
            ),
        }
        for name, (start, end) in cases.items():
            with self.subTest(name=name), self.assertRaises(PushGuardError):
                parse_contest_schedule(self._duration_html(start, end))

    def test_accepts_supported_slug_forms(self) -> None:
        self.assertEqual(
            contest_url("adt_all_20260701_2"),
            "https://atcoder.jp/contests/adt_all_20260701_2",
        )
        self.assertEqual(
            contest_url("tessoku-book"),
            "https://atcoder.jp/contests/tessoku-book",
        )

    def test_accepts_maximum_length_contest_id(self) -> None:
        contest_id = "a" * 100

        self.assertEqual(
            contest_url(contest_id),
            f"https://atcoder.jp/contests/{contest_id}",
        )

    def test_rejects_url_syntax(self) -> None:
        for value in ("ABC467", "../abc467", "abc467?lang=ja", ""):
            with self.subTest(value=value), self.assertRaises(PushGuardError):
                contest_url(value)

    def test_rejects_contest_id_boundary_violations(self) -> None:
        for value in ("_abc467", "-abc467", "a" * 101, "abc467\n"):
            with self.subTest(value=value), self.assertRaises(PushGuardError):
                contest_url(value)

    @staticmethod
    def _duration_html(start: str, end: str) -> str:
        return (
            "<small class='contest-duration'>"
            f"<time class='fixtime fixtime-full'>{start}</time>"
            f"<time class='fixtime fixtime-full'>{end}</time>"
            "</small>"
        )


class _Response:
    def __init__(self, body: bytes) -> None:
        self.body = body
        self.entered = False
        self.exited = False

    def __enter__(self) -> "_Response":
        self.entered = True
        return self

    def __exit__(self, exc_type: object, exc: object, traceback: object) -> None:
        self.exited = True

    def read(self) -> bytes:
        return self.body


class _ReadFailureResponse(_Response):
    def __init__(self, error: Exception) -> None:
        super().__init__(b"")
        self.error = error

    def read(self) -> bytes:
        raise self.error


class TestScheduleFetching(unittest.TestCase):
    def test_fetches_validated_https_url_and_delegates_decoded_html(self) -> None:
        response = _Response("<small>コンテスト</small>".encode("utf-8"))
        captured: dict[str, object] = {}

        def open_url(request: object, *, timeout: float) -> _Response:
            captured["request"] = request
            captured["timeout"] = timeout
            return response

        expected = (
            datetime(2026, 7, 18, 12, 0, tzinfo=timezone.utc),
            datetime(2026, 7, 18, 13, 40, tzinfo=timezone.utc),
        )
        with patch(
            "tools.push_guard.parse_contest_schedule", return_value=expected
        ) as parse_schedule:
            result = fetch_contest_schedule("abc467", open_url=open_url)

        request = captured["request"]
        self.assertEqual(request.full_url, "https://atcoder.jp/contests/abc467")
        self.assertEqual(
            request.get_header("User-agent"),
            "atcoder-local-push-guard/1",
        )
        self.assertEqual(captured["timeout"], 10.0)
        self.assertTrue(math.isfinite(captured["timeout"]))
        self.assertTrue(response.entered)
        self.assertTrue(response.exited)
        parse_schedule.assert_called_once_with("<small>コンテスト</small>")
        self.assertEqual(result, expected)

    def test_rejects_invalid_contest_id_before_opening_url(self) -> None:
        def open_url(request: object, *, timeout: float) -> _Response:
            self.fail("open_url must not be called for an invalid contest ID")

        with self.assertRaises(PushGuardError):
            fetch_contest_schedule("../abc467", open_url=open_url)

    def test_converts_network_and_http_failures(self) -> None:
        errors = {
            "os": OSError("network unavailable"),
            "http": HTTPError(
                "https://atcoder.jp/contests/abc467",
                503,
                "Service Unavailable",
                None,
                None,
            ),
        }
        for name, error in errors.items():
            def open_url(
                request: object,
                *,
                timeout: float,
                error: OSError = error,
            ) -> _Response:
                raise error

            with self.subTest(name=name):
                with self.assertRaises(PushGuardError):
                    fetch_contest_schedule("abc467", open_url=open_url)

    def test_converts_http_protocol_failures_from_response_read(self) -> None:
        errors = {
            "incomplete read": IncompleteRead(b"partial", 10),
            "bad status line": BadStatusLine("not-http"),
        }
        for name, error in errors.items():
            response = _ReadFailureResponse(error)

            def open_url(
                request: object,
                *,
                timeout: float,
                response: _ReadFailureResponse = response,
            ) -> _ReadFailureResponse:
                return response

            with self.subTest(name=name):
                with self.assertRaises(PushGuardError):
                    fetch_contest_schedule("abc467", open_url=open_url)
                self.assertTrue(response.entered)
                self.assertTrue(response.exited)

    def test_converts_utf8_decode_failure(self) -> None:
        def open_url(request: object, *, timeout: float) -> _Response:
            return _Response(b"\xff")

        with self.assertRaises(PushGuardError):
            fetch_contest_schedule("abc467", open_url=open_url)

    def test_converts_malformed_schedule_html(self) -> None:
        bodies = {
            "missing duration": b"<html><body>no duration</body></html>",
            "valueless class": b"<small class><time class>invalid</time></small>",
        }
        for name, body in bodies.items():
            def open_url(
                request: object,
                *,
                timeout: float,
                body: bytes = body,
            ) -> _Response:
                return _Response(body)

            with self.subTest(name=name):
                with self.assertRaises(PushGuardError):
                    fetch_contest_schedule("abc467", open_url=open_url)


if __name__ == "__main__":
    unittest.main()
