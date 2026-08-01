# Push Guard and Memo Generation Removal Design

## Goal

Remove the repository's push-blocking feature and the automatic `memo.md`
creation performed after `acc new`. The repository is intended to be created
as a private repository from the template, so a contest-aware push restriction
is unnecessary and obstructs work on permanent contests.

## Selected Approach

Delete the features completely instead of retaining disabled implementations
or a compatibility command. This removes the tracked pre-push hook, its Python
implementation, its tests and fixture, and all active documentation for the
feature.

Historical design specifications and implementation plans remain unchanged as
an archival record. Existing contest `memo.md` files are user data and are not
deleted.

## Command Behavior

### `acc new`

The wrapper no longer treats `new` or `n` specially. It forwards the original
arguments to raw `atcoder-cli` exactly once and returns its exit code. It does
not:

- require or inspect a Git hook;
- fetch an AtCoder contest schedule;
- register a contest lock;
- parse `contest.acc.json`; or
- create `memo.md`.

The local workflow commands `build`, `run`, `test`, `submit`, and `doctor`
continue to use the repository workflow dispatcher. Every other command,
including `new`, remains a transparent `atcoder-cli` delegation.

### `acc doctor`

The doctor command no longer checks push-guard installation. Its remaining
checks keep their existing order and behavior: `uv`, `atcoder-cli`, `oj`,
`oj-bundle`, the compiler, bundled libraries, and the active shell wrapper.

### Git push

Delete `.githooks/pre-push`. Repositories that previously ran the installer may
retain `core.hooksPath=.githooks`, but Git has no tracked pre-push hook to run,
so pushes are not blocked. No uninstaller is retained because the leftover
configuration is harmless for this repository and does not justify another
compatibility surface.

## Removed Code and Data

- `.githooks/pre-push`;
- `tools/push_guard.py`;
- push-guard unit and integration tests;
- the HTML fixture used only by push-guard schedule parsing;
- wrapper helpers and collaborators used only for guard preflight, contest
  registration, contest directory discovery, and memo generation; and
- doctor dependencies and checks used only by push-guard.

Active README sections and troubleshooting entries for installation, status,
recovery, contest registration, push blocking, and automatic memo generation
are removed or rewritten. Historical files under `docs/superpowers/` remain.

## Error Handling

Removing these features also removes their error paths. `acc new` no longer
fails because a hook is absent, a contest schedule cannot be fetched, a lock
cannot be stored, `contest.acc.json` cannot be parsed, or `memo.md` cannot be
written. Raw `atcoder-cli` spawn failures and exit codes retain their current
behavior.

## Tests

Automated tests cover:

- `acc new` forwarding its arguments exactly once and returning the exact raw
  `atcoder-cli` exit code;
- all non-workflow commands continuing to bypass the local dispatcher;
- the five local workflow commands continuing to use the dispatcher;
- `acc doctor` reporting the seven remaining checks in stable order;
- the full existing build, run, test, and submit suite; and
- active code and README content containing no push-guard or memo-generation
  references.

