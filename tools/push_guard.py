from __future__ import annotations

import argparse
from collections.abc import Callable, Iterable, Sequence
from dataclasses import dataclass
from datetime import datetime, timedelta, timezone
from html.parser import HTMLParser
from http.client import HTTPException
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
from tempfile import NamedTemporaryFile
from urllib.error import HTTPError
from urllib.request import Request, urlopen


ATCODER_BASE_URL = "https://atcoder.jp/contests"
CONTEST_ID_PATTERN = re.compile(r"\A[a-z0-9][a-z0-9_-]{0,99}\Z")
ATCODER_TIME_FORMAT = "%Y-%m-%d %H:%M:%S%z"
JST = timezone(timedelta(hours=9), name="JST")
MANUAL_END_FORMAT = "%Y-%m-%d %H:%M"
MANUAL_END_PATTERN = re.compile(r"\A\d{4}-\d{2}-\d{2} \d{2}:\d{2}\Z")
STATE_VERSION = 1
STATE_FILENAME = "atcoder-push-lock.json"
HOOKS_PATH_VALUE = ".githooks"
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


def parse_manual_end(value: str, now: datetime) -> datetime:
    if not isinstance(now, datetime):
        raise PushGuardError("current time must be timezone-aware")
    try:
        now_utc = _decision_time_in_utc(now)
    except (ValueError, OverflowError) as exc:
        raise PushGuardError("current time must be timezone-aware") from exc

    if not isinstance(value, str) or MANUAL_END_PATTERN.fullmatch(value) is None:
        raise PushGuardError("manual end must use YYYY-MM-DD HH:MM in JST")
    try:
        end_at = datetime.strptime(value, MANUAL_END_FORMAT).replace(tzinfo=JST)
        end_at_utc = end_at.astimezone(timezone.utc)
    except (ValueError, OverflowError) as exc:
        raise PushGuardError(f"invalid manual end time: {exc}") from exc
    if end_at_utc <= now_utc:
        raise PushGuardError("manual end time must be in the future")
    return end_at_utc


def set_manual_end(
    state_path: Path,
    contest_id: str,
    end_value: str,
    now: datetime,
) -> None:
    locks = load_state(state_path)
    matching_lock = next(
        (lock for lock in locks if lock.contest_id == contest_id),
        None,
    )
    if matching_lock is None:
        raise PushGuardError(f"contest is not registered: {contest_id}")
    if matching_lock.end_at is not None:
        raise PushGuardError(f"contest already has an end time: {contest_id}")

    end_at = parse_manual_end(end_value, now)
    if _decision_time_in_utc(matching_lock.start_at) >= end_at:
        raise PushGuardError("contest start must be before manual end time")

    replacement = ContestLock(contest_id, matching_lock.start_at, end_at)
    write_state(
        state_path,
        [replacement if lock.contest_id == contest_id else lock for lock in locks],
    )


def register_contest(
    contest_id: str,
    state_path: Path,
    *,
    fetch_schedule: Callable[[str], tuple[datetime, datetime]],
    input_value: Callable[[str], str],
    is_interactive: bool,
    now: Callable[[], datetime],
) -> None:
    try:
        start_at, end_at = fetch_schedule(contest_id)
    except PushGuardError as acquisition_error:
        failure_time = now()
        upsert_lock(state_path, ContestLock(contest_id, failure_time, None))

        if not is_interactive:
            raise PushGuardError(
                f"schedule unavailable for {contest_id}; end time is unresolved"
            ) from acquisition_error

        try:
            end_value = input_value(
                f"End time for {contest_id} in JST (YYYY-MM-DD HH:MM): "
            )
        except (EOFError, KeyboardInterrupt) as exc:
            raise PushGuardError(
                f"manual end not provided for {contest_id}; end time is unresolved"
            ) from exc

        try:
            set_manual_end(state_path, contest_id, end_value, failure_time)
        except StateError:
            raise
        except PushGuardError as exc:
            raise PushGuardError(
                f"invalid manual end for {contest_id}; end time is unresolved: {exc}"
            ) from exc
        return

    upsert_lock(state_path, ContestLock(contest_id, start_at, end_at))


def _git_rev_parse(cwd: Path, arguments: Sequence[str]) -> str:
    command = ["git", "rev-parse", *arguments]
    try:
        result = subprocess.run(
            command,
            cwd=cwd,
            check=True,
            capture_output=True,
            text=True,
        )
    except (OSError, UnicodeError, subprocess.CalledProcessError) as exc:
        detail = str(exc)
        if isinstance(exc, subprocess.CalledProcessError) and exc.stderr:
            detail = exc.stderr.strip() or detail
        raise PushGuardError(f"Git path resolution failed: {detail}") from exc

    value = result.stdout.strip()
    if not value:
        raise PushGuardError("Git path resolution returned empty output")
    return value


def _resolve_path(path: Path, description: str) -> Path:
    try:
        return path.resolve()
    except (OSError, RuntimeError) as exc:
        raise PushGuardError(f"failed to resolve {description}: {exc}") from exc


def repository_root(cwd: Path) -> Path:
    return _resolve_path(
        Path(_git_rev_parse(cwd, ["--show-toplevel"])),
        "repository root",
    )


def state_path_for_repository(root: Path) -> Path:
    raw_path = Path(_git_rev_parse(root, ["--git-path", STATE_FILENAME]))
    if not raw_path.is_absolute():
        raw_path = root / raw_path
    return _resolve_path(raw_path, "push-guard state path")


def guard_is_installed(root: Path) -> bool:
    try:
        result = subprocess.run(
            ["git", "config", "--local", "--get", "core.hooksPath"],
            cwd=root,
            check=False,
            capture_output=True,
            text=True,
        )
    except (OSError, UnicodeError) as exc:
        raise PushGuardError(f"failed to inspect Git hook configuration: {exc}") from exc

    if result.returncode == 1:
        return False
    if result.returncode != 0:
        detail = result.stderr.strip() or f"exit status {result.returncode}"
        raise PushGuardError(
            f"failed to inspect Git hook configuration: {detail}"
        )

    configured_value = result.stdout.strip()
    if not configured_value:
        return False
    try:
        configured_path = Path(configured_value).expanduser()
    except (OSError, RuntimeError) as exc:
        raise PushGuardError(f"failed to resolve Git hook path: {exc}") from exc
    if not configured_path.is_absolute():
        configured_path = root / configured_path

    expected_hooks_path = _resolve_path(root / HOOKS_PATH_VALUE, "Git hook path")
    configured_hooks_path = _resolve_path(
        configured_path,
        "configured Git hook path",
    )
    expected_hook = expected_hooks_path / "pre-push"
    return (
        configured_hooks_path == expected_hooks_path
        and expected_hook.is_file()
        and os.access(expected_hook, os.X_OK)
    )


def utc_now() -> datetime:
    return datetime.now(timezone.utc).replace(microsecond=0)


def _copy_corrupt_state_backup(path: Path, timestamp: str) -> Path:
    backup_path: Path | None = None
    try:
        with NamedTemporaryFile(
            mode="wb",
            dir=path.parent,
            prefix=f"atcoder-push-lock.corrupt-{timestamp}-",
            suffix=".json",
            delete=False,
        ) as backup_file:
            backup_path = Path(backup_file.name)
    except OSError as exc:
        raise StateError(f"failed to reserve corrupt-state backup: {exc}") from exc

    try:
        shutil.copy2(path, backup_path)
    except BaseException as copy_error:
        try:
            backup_path.unlink()
        except FileNotFoundError:
            pass
        except OSError as cleanup_error:
            cleanup_message = (
                "failed to delete incomplete corrupt-state backup at "
                f"{backup_path}: {cleanup_error}"
            )
            if isinstance(copy_error, OSError):
                raise StateError(
                    "failed to back up corrupt push-guard state: "
                    f"{copy_error}; {cleanup_message}"
                ) from copy_error
            cleanup_context = StateError(cleanup_message)
            cleanup_context.__cause__ = cleanup_error
            raise copy_error from cleanup_context

        if isinstance(copy_error, OSError):
            raise StateError(
                f"failed to back up corrupt push-guard state: {copy_error}"
            ) from copy_error
        raise

    return backup_path


def recover_state(
    path: Path,
    contest_id: str,
    manual_end: str,
    now: datetime,
) -> None:
    if (
        not isinstance(contest_id, str)
        or CONTEST_ID_PATTERN.fullmatch(contest_id) is None
    ):
        raise PushGuardError(f"invalid AtCoder contest ID: {contest_id!r}")
    end_at = parse_manual_end(manual_end, now)
    start_at = _decision_time_in_utc(now).replace(microsecond=0)

    if not path.exists():
        raise PushGuardError("push-guard state does not exist")
    try:
        load_state(path)
    except StateError:
        pass
    else:
        raise PushGuardError("push-guard state is valid; recovery is refused")

    timestamp = start_at.strftime("%Y%m%dT%H%M%SZ")
    _copy_corrupt_state_backup(path, timestamp)

    write_state(path, [ContestLock(contest_id, start_at, end_at)])


def _create_argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="AtCoder contest push guard")
    subparsers = parser.add_subparsers(dest="command", required=True)
    subparsers.add_parser("check")
    subparsers.add_parser("status")

    set_end_parser = subparsers.add_parser("set-end")
    set_end_parser.add_argument("contest_id", metavar="CONTEST_ID")
    set_end_parser.add_argument("end_value", metavar="YYYY-MM-DD HH:MM")

    recover_parser = subparsers.add_parser("recover-state")
    recover_parser.add_argument("contest_id", metavar="CONTEST_ID")
    recover_parser.add_argument("end_value", metavar="YYYY-MM-DD HH:MM")
    return parser


def _run_check(state_path: Path, current_time: datetime) -> int:
    try:
        locks = load_state(state_path)
    except StateError as exc:
        print(f"push blocked: invalid state: {exc}", file=sys.stderr)
        return 1

    blocked = blocking_contests(locks, current_time)
    for lock in blocked:
        if lock.end_at is None:
            reason = "unresolved end time"
        else:
            reason = f"active until {_format_state_timestamp(lock.end_at)}"
        print(f"push blocked: {lock.contest_id} ({reason})", file=sys.stderr)
    return 1 if blocked else 0


def _run_status(root: Path, state_path: Path, current_time: datetime) -> int:
    installed = guard_is_installed(root)
    print(f"hook: {'installed' if installed else 'not installed'}")
    try:
        locks = load_state(state_path)
    except StateError as exc:
        print(f"invalid state: {exc}", file=sys.stderr)
        return 1

    for lock in sorted(locks, key=lambda item: item.contest_id):
        status = status_for_lock(lock, current_time)
        end_at = (
            "unresolved"
            if lock.end_at is None
            else _format_state_timestamp(lock.end_at)
        )
        print(
            f"{lock.contest_id}: {status} "
            f"(start={_format_state_timestamp(lock.start_at)}, end={end_at})"
        )
    return 0


def main(
    argv: Sequence[str] | None = None,
    *,
    cwd: Path | None = None,
    now: Callable[[], datetime] = utc_now,
) -> int:
    arguments = _create_argument_parser().parse_args(argv)
    working_directory = Path.cwd() if cwd is None else cwd
    try:
        root = repository_root(working_directory)
        state_path = state_path_for_repository(root)
        if arguments.command == "check":
            return _run_check(state_path, now())
        if arguments.command == "status":
            return _run_status(root, state_path, now())
        if arguments.command == "set-end":
            set_manual_end(
                state_path,
                arguments.contest_id,
                arguments.end_value,
                now(),
            )
            return 0
        if arguments.command == "recover-state":
            recover_state(
                state_path,
                arguments.contest_id,
                arguments.end_value,
                now(),
            )
            return 0
    except PushGuardError as exc:
        print(f"push-guard: {exc}", file=sys.stderr)
        return 1

    raise AssertionError(f"unhandled command: {arguments.command}")


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


if __name__ == "__main__":
    raise SystemExit(main())
