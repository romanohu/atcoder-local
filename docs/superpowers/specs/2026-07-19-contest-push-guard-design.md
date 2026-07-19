# Contest Push Guard Design

## Summary

Add a repository-local push guard that blocks every push from this repository while any registered AtCoder contest is in progress. The existing `acc` wrapper registers each contest's official start and end times after `acc new` succeeds. A committed `pre-push` hook checks the locally stored schedule without making a network request.

The guard is fail-closed when schedule acquisition or local state validation fails. It protects normal Git and IDE push flows that honor hooks. It does not attempt to prevent deliberate bypasses such as `git push --no-verify`, removing the hook configuration, or editing files under `.git`.

## Goals

- Block pushes to every remote and every branch during a registered contest.
- Use the official contest start and end times automatically.
- Allow pushes before the start time and at or after the end time without a manual unlock step.
- Keep contest participation state local and untracked.
- Avoid network access in the `pre-push` path.
- Fail closed when an expected schedule cannot be acquired or validated.
- Provide deterministic unit and integration tests without live network or wall-clock dependencies.

## Non-goals

- Preventing an intentional `--no-verify` bypass or repository reconfiguration.
- Enforcing a server-side GitHub policy.
- Determining whether the user actually registered for or submitted to a contest.
- Blocking commits, local branches, AtCoder submissions, fetches, or pulls.
- Managing hooks that already use a different `core.hooksPath`.

## Existing Repository Context

The project uses `tools/acc-wrapper.zsh` to replace the `acc` shell command with `tools/acc_wrapper.py`. The wrapper currently delegates to `acc` and creates `memo.md` after a successful `acc new`. The repository has no active Git hook and no local `core.hooksPath` setting.

The push guard extends this wrapper rather than introducing a separate contest-start workflow.

## User Flow

### One-time installation

After cloning the repository, the user runs:

```sh
uv run python tools/push_guard.py install
source "$(pwd)/tools/acc-wrapper.zsh"
```

`install` verifies that it is running in this repository, verifies `.githooks/pre-push`, and sets the repository-local Git configuration:

```text
core.hooksPath=.githooks
```

The operation is idempotent when the setting already resolves to `.githooks`. If `core.hooksPath` points elsewhere, installation fails with an explanation and does not overwrite it.

For `acc new`, the wrapper performs a preflight check before invoking the underlying `acc` command. If the committed hook is not active, it exits with installation instructions. This prevents a contest from being prepared under the false assumption that push protection is active.

### Successful schedule acquisition

For `acc new abc467`:

1. The wrapper verifies that the push guard is installed.
2. The wrapper delegates to the real `acc` command.
3. If `acc` fails, its exit code is returned and no schedule is registered.
4. Existing `memo.md` post-processing runs with its current best-effort behavior.
5. The wrapper fetches `https://atcoder.jp/contests/abc467`.
6. It parses and validates the two official timestamps in the contest-duration element.
7. It converts both timestamps to UTC and upserts the contest record in the local state file.

AtCoder currently renders the duration as two `time.fixtime.fixtime-full` elements inside the contest-duration section, with timezone-bearing values such as `2026-07-18 21:00:00+0900` and `2026-07-18 22:40:00+0900`. The parser must scope extraction to the duration section, require exactly two parseable values, and validate `start_at < end_at`. This behavior will be covered by an HTML fixture so markup changes fail visibly rather than silently selecting unrelated times. The official page is the source of truth: <https://atcoder.jp/contests/abc467>.

Registering a past contest is valid but creates no active block. Registering the same contest ID again replaces its existing record. Different contest IDs coexist; one active or unresolved record is sufficient to block a push.

### Schedule acquisition failure

If the official schedule cannot be fetched or validated after `acc new` succeeds:

1. The wrapper atomically upserts an unresolved record before asking for input.
2. The unresolved record uses the current UTC time as `start_at` and `null` as `end_at`, so it blocks immediately and indefinitely.
3. In an interactive terminal, the wrapper prompts once for an end time in JST using `YYYY-MM-DD HH:MM`.
4. A valid future value replaces `end_at` and keeps `start_at` at the failure time.
5. Invalid input, EOF, or interruption leaves the unresolved record intact and makes the wrapper return a nonzero exit code.

The user can repair an unresolved record later with:

```sh
uv run python tools/push_guard.py set-end abc467 "2026-07-18 22:40"
```

This intentionally blocks from the acquisition failure time, even if the real contest has not started yet. It is the agreed safe fallback when official timing is unavailable.

## Components

### `tools/push_guard.py`

This standard-library-only Python module owns:

- repository and Git-state path resolution;
- hook installation and installation checks;
- contest ID validation;
- official page fetching with a finite timeout;
- scoped HTML timestamp extraction;
- timezone parsing and UTC normalization;
- state loading, validation, atomic writing, and contest upsert;
- push decisions at an injected or real current time;
- CLI commands and user-facing diagnostics.

The network fetch uses HTTPS, a finite timeout, and a descriptive user agent. Before interpolation into a URL, a contest ID must match `\A[a-z0-9][a-z0-9_-]{0,99}\Z`. This accepts the lowercase alphanumeric, underscore, and hyphen forms used by this repository and official AtCoder contest slugs while rejecting path separators and URL syntax.

The module exposes small functions for parsing, persistence, and decisions so tests do not need to invoke the CLI for every case.

### `tools/acc_wrapper.py`

The existing wrapper keeps its delegation and memo behavior. For `new` and `n`, it additionally:

- requires an active push guard before calling `acc`;
- registers the contest schedule after a successful `acc` invocation;
- creates an unresolved fail-closed record and prompts for a manual end time when acquisition fails;
- returns nonzero if the schedule remains unresolved.

Memo creation remains best-effort and must not weaken or suppress push-guard failures.

### `.githooks/pre-push`

The committed executable hook:

1. Resolves the repository root using Git.
2. Runs `python3 tools/push_guard.py check` from that root.
3. Returns the check command's status unchanged.

The hook does not inspect the remote name, remote URL, destination branch, or refs on standard input. Consequently, a block applies uniformly to every push. A missing `python3`, missing script, or other execution failure returns nonzero and blocks the push.

### Local state

The state path is resolved through Git and corresponds to `.git/atcoder-push-lock.json` in a normal checkout. It is never committed. The versioned format is:

```json
{
  "version": 1,
  "contests": [
    {
      "contest_id": "abc467",
      "start_at": "2026-07-18T12:00:00Z",
      "end_at": "2026-07-18T13:40:00Z"
    }
  ]
}
```

`end_at` is `null` only for an unresolved record. Writes use a temporary file in the same Git state directory followed by an atomic replace. Existing invalid state is never silently overwritten.

The guard does not need to delete expired records. The list is bounded by contests explicitly prepared in this repository, and a later registration of the same contest replaces its record.

## Decision Rules

For each valid contest record:

- `end_at is null`: blocked.
- `now < start_at`: allowed by this record.
- `start_at <= now < end_at`: blocked.
- `now >= end_at`: allowed by this record.

The overall result is blocked if any record is blocked. Otherwise it is allowed.

The following state behavior applies:

- State file absent: allow. No contest has been registered yet.
- Valid state with no contests: allow.
- Malformed JSON, unsupported version, invalid field, invalid timestamp, or invalid interval: block with a diagnostic.
- State read error other than absence: block with a diagnostic.

All comparisons use aware UTC datetimes. Tests inject `now`; production uses the system clock in UTC.

## CLI Contract

```text
push_guard.py install
push_guard.py check
push_guard.py status
push_guard.py set-end CONTEST_ID "YYYY-MM-DD HH:MM"
push_guard.py recover-state CONTEST_ID "YYYY-MM-DD HH:MM"
```

- `install` configures and verifies the repository-local hook. It exits nonzero without changing an existing different hooks path.
- `check` prints nothing when pushes are allowed. When blocked by valid state, it prints every active or unresolved record sorted by contest ID, including the reason and end time if known, then exits nonzero. Invalid state produces one state-level diagnostic.
- `status` shows installation state and every registered contest as upcoming, active, expired, or unresolved. Invalid state produces a diagnostic and nonzero exit.
- `set-end` accepts JST, requires a future time and an existing unresolved contest ID, then atomically updates that record.
- `recover-state` accepts a contest ID and future JST end time only when the current state exists but cannot be parsed or validated. It first copies the invalid file to a timestamped sibling backup, then atomically replaces the original with a single immediate-start manual record. The original path must remain present throughout recovery so there is no transient allow window. The command refuses to operate on absent or valid state.

There is no ordinary command that removes an active record or shortens an official contest interval.

## Error Handling

- Network, HTTP, decoding, HTML parsing, and timestamp validation errors all enter the unresolved-record flow.
- Failure to persist the unresolved record is a hard error. The wrapper must state that protection could not be established and return nonzero.
- State corruption blocks `check`; recovery must be explicit rather than silently resetting the file.
- The supported corrupt-state recovery is `push_guard.py recover-state CONTEST_ID "YYYY-MM-DD HH:MM"`. Recovery remains fail-closed: it replaces invalid state with a manual record that blocks immediately until the supplied future end time and retains the invalid source as a backup. A user who intentionally removes or edits Git metadata is outside the protection boundary.
- Hook installation errors do not mutate an existing hooks configuration.
- Diagnostics go to standard error when they describe a block or failure.
- No exception traceback is shown for expected operational errors; the CLI reports concise corrective commands.

## Documentation Changes

Update `README.md` to:

- make push-guard installation part of setup;
- explain that the `acc` wrapper is required for automatic registration;
- document automatic timing, fail-closed manual recovery, `status`, and the exact `[start, end)` blocking interval;
- state that all remotes and branches are covered;
- state the `--no-verify` and local-tampering limitation clearly.

The existing `contests/README.md` only documents contest-local commands and does not need push-guard details.

## Testing Strategy

### Unit tests

- Parse a representative AtCoder duration HTML fixture.
- Reject missing, extra, malformed, reversed, or unscoped timestamps.
- Validate contest IDs before URL construction.
- Decide correctly before start, exactly at start, immediately before end, and exactly at end.
- Block when one of multiple contests is active or unresolved.
- Allow past contests and an absent state file.
- Block malformed JSON, unknown versions, invalid fields, and invalid intervals.
- Upsert duplicate contest IDs without removing other contests.
- Preserve an unresolved record on invalid manual input or EOF.
- Accept a valid future JST manual end time and convert it to UTC.
- Recover corrupt state only through a backup plus an immediate manual lock, and refuse recovery for absent or valid state.
- Verify atomic state replacement behavior at the function boundary.
- Verify install idempotency and refusal to overwrite another `core.hooksPath`.

### Wrapper tests

- Preserve existing `acc` return codes and memo behavior.
- Refuse `acc new` when the hook is inactive.
- Register after a successful `acc new`.
- Do not register after a failed `acc new`.
- Create an unresolved record before prompting when acquisition fails.
- After a successful underlying `acc` command, return nonzero only when the guard remains unresolved or cannot be persisted. A failing underlying `acc` command continues to return its own nonzero exit code.

All subprocesses, network responses, user input, and current times are injected or mocked.

### Integration tests

Use temporary local Git repositories and a local bare remote. Enable the committed hook and verify that:

- a push succeeds with no state;
- a push is rejected for an active record;
- a push is rejected for an unresolved or corrupt record;
- the same decision applies to a second remote and a non-default branch.

Integration tests create active and expired intervals relative to the real current time and make no internet connection. Exact start and end boundaries are covered at the decision-function level with an injected `now`; the installed hook and CLI expose no clock override.

## Acceptance Criteria

- After one-time installation, `acc new CONTEST_ID` automatically registers official contest times.
- Any ordinary push from this repository to any remote or branch fails during `[start_at, end_at)` for any registered contest.
- The same push succeeds before `start_at` and at or after `end_at` without manual action.
- A fetch or parse failure establishes an immediate unresolved block before prompting for a manual end time.
- Corrupt or unreadable existing state blocks rather than allowing a push.
- The state remains under Git metadata and is not included in commits.
- Existing memo generation continues to work.
- Tests cover boundary times, failure paths, multiple contests, hook installation, and real local Git push behavior.
