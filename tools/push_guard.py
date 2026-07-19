from __future__ import annotations

from datetime import datetime, timezone
from html.parser import HTMLParser
import re
from urllib.error import HTTPError
from urllib.request import Request, urlopen


ATCODER_BASE_URL = "https://atcoder.jp/contests"
CONTEST_ID_PATTERN = re.compile(r"\A[a-z0-9][a-z0-9_-]{0,99}\Z")
ATCODER_TIME_FORMAT = "%Y-%m-%d %H:%M:%S%z"


class PushGuardError(Exception):
    """Expected push-guard operational or validation error."""


class _ContestDurationParser(HTMLParser):
    def __init__(self) -> None:
        super().__init__()
        self.values: list[str] = []
        self._duration_depth = 0
        self._time_parts: list[str] | None = None

    def handle_starttag(
        self, tag: str, attrs: list[tuple[str, str | None]]
    ) -> None:
        class_value = dict(attrs).get("class") or ""
        classes = set(class_value.split())
        if tag == "small":
            if self._duration_depth:
                self._duration_depth += 1
            elif "contest-duration" in classes:
                self._duration_depth = 1

        if (
            self._duration_depth
            and tag == "time"
            and {"fixtime", "fixtime-full"} <= classes
        ):
            self._time_parts = []

    def handle_endtag(self, tag: str) -> None:
        if tag == "time" and self._time_parts is not None:
            self.values.append("".join(self._time_parts).strip())
            self._time_parts = None

        if tag == "small" and self._duration_depth:
            self._duration_depth -= 1

    def handle_data(self, data: str) -> None:
        if self._time_parts is not None:
            self._time_parts.append(data)


def contest_url(contest_id: str) -> str:
    if CONTEST_ID_PATTERN.fullmatch(contest_id) is None:
        raise PushGuardError(f"invalid AtCoder contest ID: {contest_id!r}")
    return f"{ATCODER_BASE_URL}/{contest_id}"


def parse_contest_schedule(html: str) -> tuple[datetime, datetime]:
    parser = _ContestDurationParser()
    parser.feed(html)
    parser.close()
    if len(parser.values) != 2:
        raise PushGuardError("contest duration must contain exactly two timestamps")

    try:
        values = [
            datetime.strptime(value, ATCODER_TIME_FORMAT).astimezone(timezone.utc)
            for value in parser.values
        ]
    except ValueError as exc:
        raise PushGuardError(f"invalid contest duration timestamp: {exc}") from exc

    if values[0] >= values[1]:
        raise PushGuardError("contest start must be before contest end")
    return values[0], values[1]


def fetch_contest_schedule(
    contest_id: str,
    *,
    open_url=urlopen,
    timeout: float = 10.0,
) -> tuple[datetime, datetime]:
    request = Request(
        contest_url(contest_id),
        headers={"User-Agent": "atcoder-local-push-guard/1"},
    )
    try:
        with open_url(request, timeout=timeout) as response:
            body = response.read()
        html = body.decode("utf-8")
    except (HTTPError, OSError, UnicodeError) as exc:
        raise PushGuardError(f"failed to fetch AtCoder contest schedule: {exc}") from exc

    return parse_contest_schedule(html)
