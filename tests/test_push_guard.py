from datetime import datetime, timezone
import math
from pathlib import Path
import unittest
from unittest.mock import patch
from urllib.error import HTTPError

from tools.push_guard import (
    PushGuardError,
    contest_url,
    fetch_contest_schedule,
    parse_contest_schedule,
)


FIXTURES = Path(__file__).parent / "fixtures"


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

    def test_rejects_url_syntax(self) -> None:
        for value in ("ABC467", "../abc467", "abc467?lang=ja", ""):
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
                with self.assertRaises(PushGuardError) as raised:
                    fetch_contest_schedule("abc467", open_url=open_url)
                self.assertIs(type(raised.exception), PushGuardError)

    def test_converts_utf8_decode_failure(self) -> None:
        def open_url(request: object, *, timeout: float) -> _Response:
            return _Response(b"\xff")

        with self.assertRaises(PushGuardError) as raised:
            fetch_contest_schedule("abc467", open_url=open_url)

        self.assertIs(type(raised.exception), PushGuardError)

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
                with self.assertRaises(PushGuardError) as raised:
                    fetch_contest_schedule("abc467", open_url=open_url)
                self.assertIs(type(raised.exception), PushGuardError)


if __name__ == "__main__":
    unittest.main()
