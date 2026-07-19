from datetime import datetime, timedelta, timezone, tzinfo
from http.client import BadStatusLine, IncompleteRead
import json
import math
import os
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest
from unittest.mock import patch
from urllib.error import HTTPError
from zoneinfo import ZoneInfo

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
