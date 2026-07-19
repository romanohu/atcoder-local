from __future__ import annotations

from collections.abc import Iterable, Sequence
from dataclasses import dataclass
from datetime import datetime, timezone
from html.parser import HTMLParser
from http.client import HTTPException
import json
import os
from pathlib import Path
import re
from tempfile import NamedTemporaryFile
from urllib.error import HTTPError
from urllib.request import Request, urlopen


ATCODER_BASE_URL = "https://atcoder.jp/contests"
CONTEST_ID_PATTERN = re.compile(r"\A[a-z0-9][a-z0-9_-]{0,99}\Z")
ATCODER_TIME_FORMAT = "%Y-%m-%d %H:%M:%S%z"
STATE_VERSION = 1
STATE_TIMESTAMP_FORMAT = "%Y-%m-%dT%H:%M:%SZ"
STATE_TIMESTAMP_PATTERN = re.compile(
    r"\A\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}Z\Z"
)
STATE_TOP_LEVEL_KEYS = {"version", "contests"}
STATE_RECORD_KEYS = {"contest_id", "start_at", "end_at"}


class PushGuardError(Exception):
    """Expected push-guard operational or validation error."""


class StateError(PushGuardError):
    """State could not be read, validated, or replaced safely."""


@dataclass(frozen=True)
class ContestLock:
    contest_id: str
    start_at: datetime
    end_at: datetime | None


def _decision_time_in_utc(value: datetime) -> datetime:
    if value.tzinfo is None or value.utcoffset() is None:
        raise ValueError("decision timestamps must be timezone-aware")
    return value.astimezone(timezone.utc)


def status_for_lock(lock: ContestLock, now: datetime) -> str:
    if lock.end_at is None:
        return "unresolved"
    now_utc = _decision_time_in_utc(now)
    start_at_utc = _decision_time_in_utc(lock.start_at)
    end_at_utc = _decision_time_in_utc(lock.end_at)
    if now_utc < start_at_utc:
        return "upcoming"
    if now_utc < end_at_utc:
        return "active"
    return "expired"


def blocking_contests(
    locks: Iterable[ContestLock], now: datetime
) -> list[ContestLock]:
    return sorted(
        (
            lock
            for lock in locks
            if status_for_lock(lock, now) in {"active", "unresolved"}
        ),
        key=lambda lock: lock.contest_id,
    )


def _parse_state_timestamp(value: object, field_name: str) -> datetime:
    if (
        not isinstance(value, str)
        or STATE_TIMESTAMP_PATTERN.fullmatch(value) is None
    ):
        raise StateError(f"{field_name} must be a canonical UTC timestamp")
    try:
        parsed = datetime.strptime(value, STATE_TIMESTAMP_FORMAT)
    except ValueError as exc:
        raise StateError(f"invalid {field_name}: {exc}") from exc
    return parsed.replace(tzinfo=timezone.utc)


def _validate_datetime(value: object, field_name: str) -> datetime:
    if not isinstance(value, datetime):
        raise StateError(f"{field_name} must be a datetime")
    if value.tzinfo is None:
        raise StateError(f"{field_name} must be timezone-aware")
    if value.microsecond != 0:
        raise StateError(f"{field_name} must have whole-second precision")
    try:
        if value.utcoffset() is None:
            raise StateError(f"{field_name} must be timezone-aware")
        return value.astimezone(timezone.utc)
    except (ValueError, OverflowError) as exc:
        raise StateError(f"failed to convert {field_name} to UTC: {exc}") from exc


def _validate_lock(lock: object) -> ContestLock:
    if not isinstance(lock, ContestLock):
        raise StateError("state entries must be ContestLock instances")
    if (
        not isinstance(lock.contest_id, str)
        or CONTEST_ID_PATTERN.fullmatch(lock.contest_id) is None
    ):
        raise StateError(f"invalid contest ID in state: {lock.contest_id!r}")

    start_at = _validate_datetime(lock.start_at, "start_at")
    end_at: datetime | None = None
    if lock.end_at is not None:
        end_at = _validate_datetime(lock.end_at, "end_at")
        if start_at >= end_at:
            raise StateError("contest start must be before contest end")
    return ContestLock(lock.contest_id, start_at, end_at)


def _validate_locks(locks: Iterable[object]) -> list[ContestLock]:
    validated: list[ContestLock] = []
    contest_ids: set[str] = set()
    for lock in locks:
        valid_lock = _validate_lock(lock)
        if valid_lock.contest_id in contest_ids:
            raise StateError(f"duplicate contest ID: {valid_lock.contest_id}")
        contest_ids.add(valid_lock.contest_id)
        validated.append(valid_lock)
    return validated


def _strict_json_object(pairs: list[tuple[str, object]]) -> dict[str, object]:
    result: dict[str, object] = {}
    for key, value in pairs:
        if key in result:
            raise StateError(f"duplicate JSON key: {key}")
        result[key] = value
    return result


def load_state(path: Path) -> list[ContestLock]:
    try:
        raw = path.read_text(encoding="utf-8")
    except FileNotFoundError:
        return []
    except (OSError, UnicodeError) as exc:
        raise StateError(f"failed to read push-guard state: {exc}") from exc

    try:
        payload = json.loads(raw, object_pairs_hook=_strict_json_object)
    except (ValueError, RecursionError) as exc:
        raise StateError(f"malformed push-guard state JSON: {exc}") from exc

    if not isinstance(payload, dict) or set(payload) != STATE_TOP_LEVEL_KEYS:
        raise StateError("state must contain exactly version and contests")
    if type(payload["version"]) is not int:
        raise StateError("state version must be an integer")
    if payload["version"] != STATE_VERSION:
        raise StateError(f"unsupported state version: {payload['version']}")

    records = payload["contests"]
    if not isinstance(records, list):
        raise StateError("state contests must be a list")

    locks: list[ContestLock] = []
    for record in records:
        if not isinstance(record, dict) or set(record) != STATE_RECORD_KEYS:
            raise StateError(
                "contest record must contain exactly contest_id, start_at, and end_at"
            )
        contest_id = record["contest_id"]
        if (
            not isinstance(contest_id, str)
            or CONTEST_ID_PATTERN.fullmatch(contest_id) is None
        ):
            raise StateError(f"invalid contest ID in state: {contest_id!r}")
        start_at = _parse_state_timestamp(record["start_at"], "start_at")
        raw_end_at = record["end_at"]
        end_at = (
            None
            if raw_end_at is None
            else _parse_state_timestamp(raw_end_at, "end_at")
        )
        locks.append(ContestLock(contest_id, start_at, end_at))

    return _validate_locks(locks)


def _format_state_timestamp(value: datetime) -> str:
    return value.astimezone(timezone.utc).strftime(STATE_TIMESTAMP_FORMAT)


def write_state(path: Path, locks: Sequence[ContestLock]) -> None:
    validated = sorted(_validate_locks(locks), key=lambda lock: lock.contest_id)
    payload = {
        "version": STATE_VERSION,
        "contests": [
            {
                "contest_id": lock.contest_id,
                "start_at": _format_state_timestamp(lock.start_at),
                "end_at": (
                    None
                    if lock.end_at is None
                    else _format_state_timestamp(lock.end_at)
                ),
            }
            for lock in validated
        ],
    }
    serialized = json.dumps(payload, indent=2) + "\n"

    temporary_path: Path | None = None
    primary_error: OSError | None = None
    cleanup_error: OSError | None = None
    try:
        with NamedTemporaryFile(
            mode="w",
            encoding="utf-8",
            dir=path.parent,
            prefix=f".{path.name}.",
            suffix=".tmp",
            delete=False,
        ) as temporary_file:
            temporary_path = Path(temporary_file.name)
            temporary_file.write(serialized)
            temporary_file.flush()
            os.fsync(temporary_file.fileno())
        os.replace(temporary_path, path)
        temporary_path = None
    except OSError as exc:
        primary_error = exc
    finally:
        if temporary_path is not None:
            try:
                temporary_path.unlink()
            except FileNotFoundError:
                pass
            except OSError as exc:
                cleanup_error = exc

    if primary_error is not None:
        raise StateError(
            f"failed to replace push-guard state: {primary_error}"
        ) from primary_error
    if cleanup_error is not None:
        raise StateError(
            f"failed to clean temporary push-guard state: {cleanup_error}"
        ) from cleanup_error


def upsert_lock(path: Path, new_lock: ContestLock) -> None:
    existing_locks = load_state(path)
    valid_new_lock = _validate_lock(new_lock)
    locks_by_id = {lock.contest_id: lock for lock in existing_locks}
    locks_by_id[valid_new_lock.contest_id] = valid_new_lock
    write_state(path, list(locks_by_id.values()))


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
    except (HTTPError, HTTPException, OSError, UnicodeError) as exc:
        raise PushGuardError(f"failed to fetch AtCoder contest schedule: {exc}") from exc

    return parse_contest_schedule(html)
