# Installable `acc` Wrapper and Task-Local Submission Design

## Goal

Keep the existing `acc test`-driven submission workflow while removing the need
to source a shell wrapper in every new shell. Publish the tested bundled source
as `submission.cpp` directly in the selected task directory, such as
`contests/abc123/a/submission.cpp`.

## Scope

This change covers two related usability improvements:

- install the repository wrapper as an `acc` executable with a one-time `uv`
  tool installation; and
- relocate the verified submission artifact from the repository-wide hidden
  build directory to the selected task directory.

Binary artifacts remain under `.atcoder-local/build/`. Content-hash validation,
debug-versus-release verification, and changes to the external submission
transport are outside this change.

## Installed Command

The Python project exposes a console script named `acc`. The documented setup
command is:

```sh
uv tool install --editable .
```

After this one-time installation, a new shell can run `acc build`, `acc run`,
`acc test`, `acc submit`, and native atcoder-cli commands without sourcing
`tools/acc-wrapper.zsh` or `tools/acc-wrapper.bash`.

Installing the wrapper shadows the upstream atcoder-cli executable with the same
name. The wrapper therefore resolves the next executable named `acc` on `PATH`,
excluding its own console-script path. Local workflow commands continue to use
the repository implementation. All other commands, and the final submission
stage, invoke the resolved upstream atcoder-cli executable. Failure to find a
distinct upstream executable is a workflow error instead of a recursive call.

The existing shell wrappers remain available for compatibility, but README
setup recommends the installed console script.

## Submission Artifact Location

`TaskContext` exposes the task-local submission artifact path:

```text
<task-directory>/submission.cpp
```

For example:

```text
contests/abc123/a/submission.cpp
contests/abc123/b/submission.cpp
```

Compiled executables remain at:

```text
.atcoder-local/build/<contest-id>/<task-id>/
```

The configured source path must not resolve to the generated
`submission.cpp`. The workflow rejects that configuration before deleting or
writing either path.

## `acc test` Data Flow

1. Resolve the task, source, tests, task-local `submission.cpp`, and hidden build
   directory.
2. Reject a source/output collision.
3. Remove any previously verified task-local `submission.cpp` so a failed test
   cannot leave an old file appearing current.
4. Bundle into `<task-directory>/.submission.cpp.pending`.
5. Compile the pending source into the existing hidden build directory.
6. Run all samples against that binary.
7. If every sample succeeds, atomically replace the task-local
   `submission.cpp` with the pending source.
8. Remove the pending source on all success and failure paths.

`acc test --debug` retains the same publication behavior as today; only the
artifact location changes in this scope.

## `acc submit` Data Flow

`acc submit` validates and displays the task-local `submission.cpp`, asks for
interactive confirmation, and passes that exact path to the upstream
atcoder-cli executable. It does not bundle, compile, or run samples.

Freshness checks continue to compare the task-local artifact against `main.cpp`
and files below `library/`. Moving the artifact does not change the current
freshness policy.

## Git Ignore Rules

The default documented contest layout is `contests/<contest-id>/<task>/`.
Generated submission artifacts and interrupted pending files in that layout are
ignored:

```gitignore
contests/**/submission.cpp
contests/**/.submission.cpp.pending
```

The rule is intentionally limited to `contests/` so an unrelated
`submission.cpp` elsewhere in the repository is not silently ignored.

## Legacy Artifacts

Existing `.atcoder-local/build/<contest>/<task>/submission.cpp` files are no
longer read or submitted. The change does not delete them automatically because
they are already ignored build artifacts. A later successful `acc test` creates
the new task-local artifact.

## Error Handling

- Missing or ambiguous task context remains a workflow error.
- A configured source path equal to the generated artifact path fails before
  artifact invalidation.
- Bundle, compile, or sample failure leaves no task-local verified artifact.
- Failure to publish or clean the pending source is reported with the affected
  task-local path.
- Failure to locate an upstream `acc` distinct from the installed wrapper is an
  explicit error.
- Upstream atcoder-cli stdio and exit-code handling otherwise retains the
  current adapter behavior.

## Tests

Automated coverage includes:

- task contexts resolve `submission.cpp` below each task directory;
- release and debug tests publish only after successful samples;
- failures remove both the verified task-local artifact and pending source;
- `acc submit` displays and passes the task-local path;
- source/output collisions fail before destructive operations;
- generated task-local files are ignored by Git while unrelated files are not;
- the installed entry point dispatches local commands;
- native command delegation and submission find an upstream `acc` while
  excluding the wrapper itself;
- missing upstream `acc` fails without recursion; and
- existing bash/zsh wrapper compatibility tests continue to pass.

The host-independent suite is run before completion. Existing real-compiler and
bundle smoke tests continue to cover the C++ toolchain when their executables
are available.

## Documentation

README setup leads with the one-time `uv tool install --editable .` command and
shows how to update an existing editable installation. Manual shell sourcing is
retained only as a compatibility alternative. Workflow examples and
troubleshooting describe the task-local `submission.cpp` path and make clear
that the file is generated and ignored by Git.
