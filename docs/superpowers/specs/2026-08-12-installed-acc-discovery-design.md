# Installed `acc` Discovery Fix Design

## Problem

`uv tool install --force --editable .` installs the repository wrapper as
`~/.local/bin/acc`. The native npm `atcoder-cli` executable can exist at
`~/.npm-global/bin/acc` without that directory being present in `PATH`, so the
wrapper reports that the upstream executable is missing. The wrapper's isolated
uv tool environment also contains `oj` and `oj-bundle`, but its `bin` directory
is not necessarily inherited through `PATH`; both `acc doctor` and native
`atcoder-cli` therefore report those installed tools as missing.

An editable uv tool installation also keeps an absolute reference to the source
checkout. Moving or deleting that checkout makes the installed console script
fail to import `tools.acc_wrapper`.

## Goals

- Keep the public command names, including `acc test`, unchanged.
- Require no shell `source` command for the normal installation.
- Discover the pinned npm `atcoder-cli` installation even when npm's global
  `bin` directory is absent from the incoming `PATH`.
- Make `oj` and `oj-bundle` installed in the same uv tool environment visible to
  both local workflow commands and delegated native `atcoder-cli` commands.
- Make the documented normal installation independent of the continued
  existence or location of the source checkout.

## Non-goals

- Changing the `acc test`, build, bundle, or submission behavior.
- Editing shell startup files automatically.
- Installing Node.js, npm, a compiler, or `atcoder-cli` automatically.
- Supporting arbitrary unpinned `atcoder-cli` versions.

## Design

### Native `acc` discovery

The existing locator continues to prefer `ATCODER_LOCAL_RAW_ACC`, then scans
`PATH` while rejecting the active wrapper and other installed repository
wrappers. If those sources do not produce a native executable, it locates `npm`
from the incoming `PATH`, executes `npm prefix --global`, and checks
`<prefix>/bin/acc` with the same executable and wrapper-rejection rules.

Failure to execute npm, a non-zero npm exit, empty or malformed output, and a
missing or wrapper-valued candidate are treated as "not found". The caller keeps
the current concise error message; no traceback is exposed for discovery
failures.

### uv tool executable visibility

The installed console entry point prepends the directory containing
`sys.executable` to `PATH` before dispatch. For a uv tool installation this is
the isolated tool environment's `bin` directory, where dependency entry points
such as `oj` and `oj-bundle` are installed. Existing `PATH` entries retain their
relative order after this directory.

This single environment adjustment covers both local subprocesses and native
`atcoder-cli`, whose own online-judge-tools check relies on `which oj`.

### Installation documentation

The normal setup and refresh commands become `uv tool install .` and
`uv tool install --force .`. A non-editable installation copies the package into
the uv tool environment and remains usable if the checkout moves. Editable
installation is not presented as the normal user workflow.

## Testing

Tests first reproduce each regression:

- the native locator finds a fake npm-global `acc` through a fake `npm` even
  when that global bin directory is absent from `PATH`;
- the console entry point prepends its interpreter directory before calling the
  dispatcher;
- README assertions require the non-editable installation commands and reject
  the editable recommendation.

After the focused tests pass, run the complete host-independent pytest suite.
Then build/install the package into an isolated temporary uv tool directory and
run the installed command outside the source checkout to verify that imports and
same-environment dependency discovery work without changing the user's global
tool installation.

## Safety

Discovery is read-only apart from running the locally installed `npm` query.
Tests use temporary directories and do not write credentials or AtCoder runtime
state. Installation verification uses temporary uv directories rather than
overwriting the user's existing `acc` command.
