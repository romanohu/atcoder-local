# Contest Push Guard Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Prevent ordinary pushes from this repository to every remote and branch while any registered AtCoder contest is active or has an unresolved end time.

**Architecture:** Extend the existing `acc` wrapper so `acc new` records official AtCoder start/end timestamps in versioned state under Git metadata. A committed repository-local `pre-push` hook calls a standard-library-only Python checker; the checker uses cached state only and fails closed on active, unresolved, unreadable, or invalid state.

**Tech Stack:** Python 3.10 standard library, `unittest`, Zsh/POSIX shell, Git hooks, `uv`.

---

## Scope and File Structure

- Create `tools/push_guard.py`: schedule acquisition, state, decisions, installation, recovery, and CLI.
- Create `.githooks/pre-push`: minimal adapter from Git to `push_guard.py check`.
- Modify `tools/acc_wrapper.py`: installation preflight and contest registration after successful `acc new`.
- Create `tests/fixtures/atcoder_contest_duration.html`: stable parser fixture.
- Create `tests/test_push_guard.py`: unit and repository-command tests.
- Modify `tests/test_acc_wrapper.py`: wrapper orchestration tests.
- Create `tests/test_push_guard_integration.py`: real local Git push tests.
- Modify `README.md`: setup, recovery, and protection boundary.

Keep the feature standard-library-only so the hook can run with `python3` without activating uv or accessing the network.

### Task 1: Parse and fetch official contest schedules

**Files:**
- Create: `tests/fixtures/atcoder_contest_duration.html`
- Create: `tests/test_push_guard.py`
- Create: `tools/push_guard.py`

- [ ] **Step 1: Add a representative official-page fixture**

Create `tests/fixtures/atcoder_contest_duration.html`:

```html
<html><body>
  <time class="fixtime fixtime-full">2000-01-01 00:00:00+0900</time>
  <small class="contest-duration">
    Contest Duration:
    <a><time class="fixtime fixtime-full">2026-07-18 21:00:00+0900</time></a>
    -
    <a><time class="fixtime fixtime-full">2026-07-18 22:40:00+0900</time></a>
    (100 minutes)
  </small>
</body></html>
```

- [ ] **Step 2: Write failing parser and identifier tests**

Create `tests/test_push_guard.py` with `unittest` and this API contract:

```python
from datetime import datetime, timezone
from pathlib import Path
import unittest

from tools.push_guard import PushGuardError, contest_url, parse_contest_schedule

FIXTURES = Path(__file__).parent / "fixtures"


class TestScheduleParsing(unittest.TestCase):
    def test_parses_scoped_times_and_normalizes_to_utc(self) -> None:
        html = (FIXTURES / "atcoder_contest_duration.html").read_text()
        start_at, end_at = parse_contest_schedule(html)
        self.assertEqual(start_at, datetime(2026, 7, 18, 12, 0, tzinfo=timezone.utc))
        self.assertEqual(end_at, datetime(2026, 7, 18, 13, 40, tzinfo=timezone.utc))

    def test_rejects_unscoped_only_time(self) -> None:
        html = "<time class='fixtime fixtime-full'>2026-01-01 00:00:00+0900</time>"
        with self.assertRaises(PushGuardError):
            parse_contest_schedule(html)

    def test_accepts_supported_slug_forms(self) -> None:
        self.assertEqual(contest_url("adt_all_20260701_2"),
                         "https://atcoder.jp/contests/adt_all_20260701_2")
        self.assertEqual(contest_url("tessoku-book"),
                         "https://atcoder.jp/contests/tessoku-book")

    def test_rejects_url_syntax(self) -> None:
        for value in ("ABC467", "../abc467", "abc467?lang=ja", ""):
            with self.subTest(value=value), self.assertRaises(PushGuardError):
                contest_url(value)
```

Add table-driven cases for one scoped time, three scoped times, malformed timestamp text, and `start_at >= end_at`.

- [ ] **Step 3: Run parser tests and verify RED**

Run: `uv run python -m unittest tests.test_push_guard.TestScheduleParsing -v`

Expected: import failure because `tools.push_guard` does not exist.

- [ ] **Step 4: Implement identifier validation and scoped parsing**

Create `tools/push_guard.py` with:

```python
from __future__ import annotations

from datetime import datetime, timezone
from html.parser import HTMLParser
import re

ATCODER_BASE_URL = "https://atcoder.jp/contests"
CONTEST_ID_PATTERN = re.compile(r"\A[a-z0-9][a-z0-9_-]{0,99}\Z")
ATCODER_TIME_FORMAT = "%Y-%m-%d %H:%M:%S%z"


class PushGuardError(Exception):
    """Expected push-guard operational or validation error."""


class _ContestDurationParser(HTMLParser):
    # Implement nested-depth tracking for small.contest-duration and collect
    # stripped text only from time elements containing fixtime and fixtime-full.
    ...


def contest_url(contest_id: str) -> str:
    if CONTEST_ID_PATTERN.fullmatch(contest_id) is None:
        raise PushGuardError(f"invalid AtCoder contest ID: {contest_id!r}")
    return f"{ATCODER_BASE_URL}/{contest_id}"


def parse_contest_schedule(html: str) -> tuple[datetime, datetime]:
    parser = _ContestDurationParser()
    parser.feed(html)
    if len(parser.values) != 2:
        raise PushGuardError("contest duration must contain exactly two timestamps")
    try:
        values = [datetime.strptime(v, ATCODER_TIME_FORMAT).astimezone(timezone.utc)
                  for v in parser.values]
    except ValueError as exc:
        raise PushGuardError(f"invalid contest duration timestamp: {exc}") from exc
    if values[0] >= values[1]:
        raise PushGuardError("contest start must be before contest end")
    return values[0], values[1]
```

Replace the parser ellipsis with the minimal depth-aware implementation. Ignore unrelated times and require exactly two scoped values.

- [ ] **Step 5: Add failing fetch tests**

Define `fetch_contest_schedule(contest_id, *, open_url=urlopen, timeout=10.0)`. With an injected context-manager response, test URL, descriptive `User-Agent`, timeout, UTF-8 decoding, and parser delegation. Verify `OSError`, HTTP errors, decode errors, and malformed HTML become `PushGuardError`.

- [ ] **Step 6: Run fetch tests and verify RED**

Run: `uv run python -m unittest tests.test_push_guard.TestScheduleFetching -v`

Expected: FAIL because `fetch_contest_schedule` is missing.

- [ ] **Step 7: Implement minimal fetching**

Use `Request(contest_url(contest_id), headers={"User-Agent": "atcoder-local-push-guard/1"})`, call the injected opener with the timeout, decode UTF-8, and delegate to `parse_contest_schedule`. Convert expected fetch/decode failures to concise `PushGuardError` messages.

- [ ] **Step 8: Run Task 1 tests and verify GREEN**

Run: `uv run python -m unittest tests.test_push_guard.TestScheduleParsing tests.test_push_guard.TestScheduleFetching -v`

Expected: all Task 1 tests pass.

- [ ] **Step 9: Commit Task 1**

```sh
git add tools/push_guard.py tests/test_push_guard.py tests/fixtures/atcoder_contest_duration.html
git commit -m "feat: parse AtCoder contest schedules"
```

### Task 2: Add versioned state and fail-closed decisions

**Files:**
- Modify: `tools/push_guard.py`
- Modify: `tests/test_push_guard.py`

- [ ] **Step 1: Write failing decision tests**

Define a frozen `ContestLock` and test exact boundaries:

```python
class TestGuardDecision(unittest.TestCase):
    def setUp(self) -> None:
        self.start = datetime(2026, 7, 18, 12, 0, tzinfo=timezone.utc)
        self.end = datetime(2026, 7, 18, 13, 40, tzinfo=timezone.utc)
        self.lock = ContestLock("abc467", self.start, self.end)

    def test_blocks_closed_open_interval(self) -> None:
        self.assertEqual(blocking_contests([self.lock], self.start), [self.lock])
        almost_end = self.end - timedelta(microseconds=1)
        self.assertEqual(blocking_contests([self.lock], almost_end), [self.lock])

    def test_allows_before_start_and_at_end(self) -> None:
        self.assertEqual(blocking_contests([self.lock], self.start - timedelta(seconds=1)), [])
        self.assertEqual(blocking_contests([self.lock], self.end), [])

    def test_unresolved_always_blocks(self) -> None:
        unresolved = ContestLock("abc467", self.start, None)
        self.assertEqual(blocking_contests([unresolved], self.start - timedelta(days=1)),
                         [unresolved])
```

Add multiple-record sorting and `status_for_lock` cases for `upcoming`, `active`, `expired`, and `unresolved`.

- [ ] **Step 2: Run decision tests and verify RED**

Run: `uv run python -m unittest tests.test_push_guard.TestGuardDecision -v`

Expected: import failure for the new model and functions.

- [ ] **Step 3: Implement the pure model and decisions**

```python
@dataclass(frozen=True)
class ContestLock:
    contest_id: str
    start_at: datetime
    end_at: datetime | None


def status_for_lock(lock: ContestLock, now: datetime) -> str:
    if lock.end_at is None:
        return "unresolved"
    if now < lock.start_at:
        return "upcoming"
    if now < lock.end_at:
        return "active"
    return "expired"


def blocking_contests(locks: Iterable[ContestLock], now: datetime) -> list[ContestLock]:
    return sorted(
        (lock for lock in locks if lock.end_at is None or lock.start_at <= now < lock.end_at),
        key=lambda lock: lock.contest_id,
    )
```

- [ ] **Step 4: Write failing state tests**

Cover absent file, version-1 round trip, canonical UTC serialization, unresolved `null`, malformed JSON, unknown version, missing/extra fields, invalid ID, noncanonical timestamp, duplicate ID, invalid interval, upsert replacement, invalid-state preservation, and atomic same-directory replacement. Mock focused `Path.read_text` failures to verify non-absence `OSError` and Unicode decode errors are wrapped as `StateError`.

- [ ] **Step 5: Run state tests and verify RED**

Run: `uv run python -m unittest tests.test_push_guard.TestStatePersistence -v`

Expected: FAIL because persistence functions are missing.

- [ ] **Step 6: Implement strict persistence**

Add `STATE_VERSION = 1`, `StateError(PushGuardError)`, and:

```python
def load_state(path: Path) -> list[ContestLock]: ...
def write_state(path: Path, locks: Sequence[ContestLock]) -> None: ...
def upsert_lock(path: Path, new_lock: ContestLock) -> None: ...
```

Require exact key sets. Parse `YYYY-MM-DDTHH:MM:SSZ` strictly as UTC. Validate IDs, uniqueness, and intervals. Absence alone maps to `[]`; wrap every other read or decode failure as `StateError`. Sort records by ID and add a trailing newline. Write with `NamedTemporaryFile` in `path.parent`, flush, `os.fsync`, and `os.replace`; clean an unconsumed temporary file in `finally`. `upsert_lock` must call `load_state` first so invalid state is never overwritten.

- [ ] **Step 7: Run Task 2 tests and verify GREEN**

Run: `uv run python -m unittest tests.test_push_guard.TestGuardDecision tests.test_push_guard.TestStatePersistence -v`

Expected: all Task 2 tests pass.

- [ ] **Step 8: Commit Task 2**

```sh
git add tools/push_guard.py tests/test_push_guard.py
git commit -m "feat: persist contest push guard state"
```

### Task 3: Add registration, CLI decisions, and corrupt-state recovery

**Files:**
- Modify: `tools/push_guard.py`
- Modify: `tests/test_push_guard.py`

- [ ] **Step 1: Write failing manual-time and registration tests**

Define and test:

```python
JST = timezone(timedelta(hours=9), name="JST")

def parse_manual_end(value: str, now: datetime) -> datetime: ...
def register_contest(
    contest_id: str,
    state_path: Path,
    *,
    fetch_schedule: Callable[[str], tuple[datetime, datetime]],
    input_value: Callable[[str], str],
    is_interactive: bool,
    now: Callable[[], datetime],
) -> None: ...
```

Test official success without prompting; fetch failure persisting `ContestLock(id, now(), None)` before input; valid interactive JST replacement; invalid input, EOF, interrupt, and noninteractive behavior preserving the unresolved record and raising `PushGuardError`; future-end validation; and `set_manual_end` requiring an existing unresolved ID while preserving its start.

- [ ] **Step 2: Run registration tests and verify RED**

Run: `uv run python -m unittest tests.test_push_guard.TestContestRegistration -v`

Expected: FAIL because registration APIs are missing.

- [ ] **Step 3: Implement unresolved-first registration**

Catch expected acquisition failures only around the fetch. On failure, persist the unresolved record before inspecting interactivity or reading input. Convert `YYYY-MM-DD HH:MM` JST to UTC. If a valid end is not obtained, raise a concise `PushGuardError` and never roll back the unresolved record.

- [ ] **Step 4: Write failing CLI and repository-path tests**

Test `main(argv, *, cwd=Path.cwd(), now=utc_now)` with captured streams:

- `check` allows absent/empty/expired state silently;
- `check` prints every sorted active/unresolved record and returns 1;
- corrupt state prints one state-level diagnostic and returns 1;
- `status` prints installation state and all four statuses;
- `set-end` updates only an unresolved record using future JST;
- expected errors return 1 without tracebacks;
- argparse usage retains exit code 2.

Test `repository_root(cwd)` using `git rev-parse --show-toplevel` and `state_path_for_repository(root)` using `git rev-parse --git-path atcoder-push-lock.json`; relative Git-path output must resolve against the root.

- [ ] **Step 5: Run CLI tests and verify RED**

Run: `uv run python -m unittest tests.test_push_guard.TestPushGuardCli -v`

Expected: FAIL because CLI dispatch is missing.

- [ ] **Step 6: Implement paths and initial CLI**

Add:

```python
STATE_FILENAME = "atcoder-push-lock.json"

def repository_root(cwd: Path) -> Path: ...
def state_path_for_repository(root: Path) -> Path: ...
def utc_now() -> datetime:
    return datetime.now(timezone.utc)
def main(argv: Sequence[str] | None = None, *, cwd: Path | None = None,
         now: Callable[[], datetime] = utc_now) -> int: ...
```

Use `argparse` subcommands. Run Git with `cwd=root`. Send block/error diagnostics to stderr. Do not expose clock injection through CLI arguments or environment; `now` exists only at the Python function boundary.

- [ ] **Step 7: Write failing corrupt-state recovery tests**

For `recover_state(path, contest_id, manual_end, now)`, verify absent or valid state is refused; invalid state is copied to a timestamped sibling before replacement; the original remains present until `os.replace`; replacement contains one immediate manual lock; copy/write failure leaves the corrupt original; and CLI exposes `recover-state`.

- [ ] **Step 8: Run recovery tests and verify RED**

Run: `uv run python -m unittest tests.test_push_guard.TestStateRecovery -v`

Expected: FAIL because recovery is missing.

- [ ] **Step 9: Implement copy-before-replace recovery**

Reserve a unique sibling with `NamedTemporaryFile(delete=False, prefix="atcoder-push-lock.corrupt-20260718T120000Z-", suffix=".json", dir=path.parent)`, close it, and use `shutil.copy2` into that reserved path. This prevents overwriting an earlier backup even when recoveries share a timestamp. Delete an incomplete reserved backup if copying fails. Only after the copy succeeds, atomically replace the original through `write_state` with one record starting at `now` and ending at the future manual UTC time. Refuse unless an existing path causes `load_state` to raise `StateError`.

- [ ] **Step 10: Run Task 3 tests and verify GREEN**

Run: `uv run python -m unittest tests.test_push_guard.TestContestRegistration tests.test_push_guard.TestPushGuardCli tests.test_push_guard.TestStateRecovery -v`

Expected: all Task 3 tests pass.

- [ ] **Step 11: Commit Task 3**

```sh
git add tools/push_guard.py tests/test_push_guard.py
git commit -m "feat: add push guard recovery commands"
```

### Task 4: Install and execute the repository-local hook

**Files:**
- Create: `.githooks/pre-push`
- Modify: `tools/push_guard.py`
- Modify: `tests/test_push_guard.py`

- [ ] **Step 1: Write failing installation tests**

Using temporary real Git repositories, test:

- unset `core.hooksPath` becomes `.githooks` when an executable expected hook exists;
- repeated install is idempotent;
- an absolute path resolving to the same `.githooks` is accepted;
- a different hooks path is unchanged and produces an error;
- missing/non-executable hook prevents configuration;
- `guard_is_installed(root)` checks both resolved config and executable hook.

- [ ] **Step 2: Run installation tests and verify RED**

Run: `uv run python -m unittest tests.test_push_guard.TestHookInstallation -v`

Expected: FAIL because install APIs and the hook are missing.

- [ ] **Step 3: Implement installation and preflight checks**

Add:

```python
HOOKS_PATH_VALUE = ".githooks"

def guard_is_installed(root: Path) -> bool: ...
def install_hook(root: Path) -> None: ...
```

Read `git config --local --get core.hooksPath`; exit code 1 means unset. Resolve relative values against the root and compare canonical paths. Validate the expected executable hook before configuration. Only write `git config --local core.hooksPath .githooks` when unset.

- [ ] **Step 4: Create the executable hook**

Create `.githooks/pre-push`:

```sh
#!/bin/sh

repo_root=$(git rev-parse --show-toplevel) || exit 1
exec python3 "$repo_root/tools/push_guard.py" check
```

Run `chmod +x .githooks/pre-push`. Do not inspect remote arguments or stdin refs. Missing Git/Python/script execution must return nonzero. Add no network or clock override.

- [ ] **Step 5: Extend CLI tests for `install`**

Verify first/repeated installation returns 0. Conflict or invalid hook returns 1 without mutation.

- [ ] **Step 6: Run Task 4 tests and syntax check**

```sh
uv run python -m unittest tests.test_push_guard.TestHookInstallation tests.test_push_guard.TestPushGuardCli -v
/bin/sh -n .githooks/pre-push
```

Expected: tests pass and shell syntax exits 0.

- [ ] **Step 7: Commit Task 4**

```sh
git add .githooks/pre-push tools/push_guard.py tests/test_push_guard.py
git commit -m "feat: install contest pre-push guard"
```

### Task 5: Register schedules through the existing `acc` wrapper

**Files:**
- Modify: `tools/acc_wrapper.py`
- Modify: `tests/test_acc_wrapper.py`

- [ ] **Step 1: Add wrapper characterization tests and an injectable runner**

Before guard behavior, mock subprocess delegation and characterize:

- non-`new` commands delegate once and return the exact `acc` code;
- failed `acc new` returns the exact code and skips post-processing;
- successful `acc new` retains best-effort memo behavior.

Refactor without behavior change:

```python
def main(
    argv: list[str] | None = None,
    *,
    cwd: Path | None = None,
    run_acc: Callable[[list[str]], int] | None = None,
) -> int:
    ...
```

The production default remains `subprocess.run(["acc", *args], check=False).returncode`.

- [ ] **Step 2: Run characterization tests before and after refactor**

Run: `uv run python -m unittest tests.test_acc_wrapper -v`

Expected: original 10 tests plus characterization tests pass.

- [ ] **Step 3: Write failing guard orchestration tests**

For `new`/`n` with an extracted ID, test:

- inactive hook fails before `run_acc` and prints the install command;
- active hook runs `acc`, memo post-processing, then registration;
- failed `acc` returns its exact code without registration;
- memo failure is reported but registration still runs;
- successful registration returns 0;
- unresolved/persistence failure after successful `acc` returns 1;
- non-`new` commands neither preflight nor register.

Inject `guard_is_installed` and `register_contest` keyword collaborators so tests do not fetch or mutate real state. Support both package import (`from . import push_guard`) and direct script execution (`import push_guard`) without changing `tools/acc-wrapper.zsh`.

- [ ] **Step 4: Run orchestration tests and verify RED**

Run: `uv run python -m unittest tests.test_acc_wrapper.TestAccWrapperGuardIntegration -v`

Expected: FAIL because orchestration is absent.

- [ ] **Step 5: Implement preflight and post-success registration**

For recognized `new` with a contest ID:

1. resolve the repository from current/injected cwd;
2. require an active hook before invoking `acc`;
3. return the underlying nonzero `acc` code unchanged;
4. run memo processing in its own best-effort `try` block;
5. call `register_contest` using official fetching, `input`, `sys.stdin.isatty()`, and UTC-now defaults;
6. report `PushGuardError` and return 1 if registration remains unresolved or persistence fails.

If `new` has no extractable ID, intentionally skip both hook preflight and registration, let real `acc` validate the arguments, and return its code. This is the sole `new` preflight exception because no contest can be registered before `acc` has reported the malformed invocation.

- [ ] **Step 6: Run wrapper and guard unit tests**

Run: `uv run python -m unittest tests.test_acc_wrapper tests.test_push_guard -v`

Expected: all tests pass with no network access.

- [ ] **Step 7: Commit Task 5**

```sh
git add tools/acc_wrapper.py tests/test_acc_wrapper.py
git commit -m "feat: register contest push locks from acc"
```

### Task 6: Verify real local Git pushes

**Files:**
- Create: `tests/test_push_guard_integration.py`

- [ ] **Step 1: Build a temporary-repository harness**

Create helpers that initialize a working repo and two local bare remotes; configure test identity; copy `tools/push_guard.py` and `.githooks/pre-push`; preserve executable mode; set `core.hooksPath=.githooks`; create commits; write state through `write_state`; and run `git push` with captured output.

Use `TemporaryDirectory`. Never modify the feature worktree's Git config or state.

- [ ] **Step 2: Add end-to-end cases**

Test:

- no state: first push to `origin` succeeds;
- active interval (`now - 1 hour` to `now + 1 hour`): push fails and remote ref is unchanged;
- unresolved record: push fails;
- corrupt JSON: push fails with a state diagnostic;
- expired interval (`now - 2 hours` to `now - 1 hour`): push succeeds;
- non-default branch to a second remote fails under an active record.

Use hour-wide real-clock margins. Exact equality remains in pure tests; no CLI/hook time override is allowed.

- [ ] **Step 3: Run integration tests and verify RED or expose gaps**

Run: `uv run python -m unittest tests.test_push_guard_integration -v`

Expected before final adjustment: any hook path, script execution, or diagnostic gap fails visibly.

- [ ] **Step 4: Make only integration-required production fixes**

Limit changes to `tools/push_guard.py` or `.githooks/pre-push`. Add no clock override, remote-specific behavior, or network call.

- [ ] **Step 5: Run integration and unit suites**

Run: `uv run python -m unittest discover -s tests -v`

Expected: every test passes.

- [ ] **Step 6: Commit Task 6**

```sh
git add tests/test_push_guard_integration.py tools/push_guard.py .githooks/pre-push
git commit -m "test: verify contest push guard end to end"
```

### Task 7: Document setup and run final verification

**Files:**
- Modify: `README.md`

- [ ] **Step 1: Document the one-time setup**

Add “コンテスト中のpush防止” with:

```sh
uv run python tools/push_guard.py install
source "$(pwd)/tools/acc-wrapper.zsh"
```

Document that `acc new` caches official times; every remote/branch is blocked during `[start, end)`; the hook is offline and unlocks automatically; acquisition failure writes an immediate unresolved lock before manual JST input; and another existing `core.hooksPath` is not overwritten.

- [ ] **Step 2: Document status and recovery commands**

Include exact examples:

```sh
uv run python tools/push_guard.py status
uv run python tools/push_guard.py set-end abc467 "2026-07-18 22:40"
uv run python tools/push_guard.py recover-state abc467 "2026-07-18 22:40"
```

State clearly that `git push --no-verify`, removing hook configuration, or editing Git metadata intentionally bypasses the local guard. Explain that sourcing the wrapper is required for automatic registration while memo creation remains its other behavior.

- [ ] **Step 3: Check README/CLI consistency**

```sh
rg -n "push_guard.py (install|status|set-end|recover-state)|--no-verify|core.hooksPath" README.md
uv run python tools/push_guard.py --help
```

Expected: every documented subcommand appears in help and the protection limitation is present.

- [ ] **Step 4: Run the complete suite**

Run: `uv run python -m unittest discover -s tests -v`

Expected: all tests pass, including the original 10 wrapper tests.

- [ ] **Step 5: Run syntax and repository checks**

```sh
/bin/sh -n .githooks/pre-push
zsh -n tools/acc-wrapper.zsh
uv run python -m compileall -q tools tests
git diff --check
git status --short
```

Expected: syntax checks exit 0, `git diff --check` prints nothing, and status lists only intended files.

- [ ] **Step 6: Review the final diff against the spec**

```sh
git diff --stat HEAD~6
git diff HEAD~6 -- README.md tools/acc_wrapper.py tools/push_guard.py .githooks/pre-push tests
```

Confirm every acceptance criterion in `docs/superpowers/specs/2026-07-19-contest-push-guard-design.md` has implementation and test evidence. Confirm CLI/hook expose no test clock.

- [ ] **Step 7: Commit documentation**

```sh
git add README.md
git commit -m "docs: explain contest push guard"
```

- [ ] **Step 8: Verify committed state**

```sh
uv run python -m unittest discover -s tests -v
/bin/sh -n .githooks/pre-push
zsh -n tools/acc-wrapper.zsh
git diff --check
git status --short --branch
```

Expected: all tests pass; checks exit 0; no uncommitted changes remain; branch is `feature/contest-push-guard`.

## Execution Notes

- Use `superpowers:test-driven-development` for each behavior: observe RED, implement minimally, verify GREEN.
- Use `superpowers:systematic-debugging` for unexpected failures.
- Use `superpowers:verification-before-completion` before any completion claim.
- Use `superpowers:requesting-code-review` after implementation and before integration.
- Use `superpowers:finishing-a-development-branch` after verification to choose merge, PR, or cleanup.
