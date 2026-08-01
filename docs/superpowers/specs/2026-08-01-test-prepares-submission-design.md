# `acc test` Submission Artifact Design

## Goal

Move submission artifact preparation and verification from `acc submit` to
`acc test`. A successful test command leaves a bundled `submission.cpp` that
`acc submit` can submit without rebuilding or rerunning samples.

## Command Behavior

### Bundle compatibility

`oj-bundle` 5.6.0 only expands quoted includes. The C++ template therefore
uses quoted includes for repository-only `atcoder_local` headers. Standard
AtCoder Library includes may remain angle-bracket includes because AtCoder
provides ACL in the judge environment.

After bundling, the workflow rejects any remaining angle-bracket
`atcoder_local` include with a clear error instead of publishing an artifact
that only compiles while the repository's `library/` directory is present.

### `acc test`

Both `acc test` and `acc test --debug` perform these stages in order:

1. Validate the source type and invalidate any previously verified
   `submission.cpp`.
2. Detect the configured C++ compiler.
3. Bundle `main.cpp` and repository-local headers into a temporary source with
   `oj-bundle`.
4. Compile the temporary bundled source.
5. Run the task's sample cases with `oj test`.
6. After all samples pass, atomically publish the temporary source as
   `.atcoder-local/build/<contest>/<task>/submission.cpp`.

The release command writes the executable as `submission-main`. The debug
command writes it as `submission-main-debug` and retains the existing debug
compiler flags, including `LOCAL`, AddressSanitizer, and
UndefinedBehaviorSanitizer.

Each later stage runs only if the preceding stage succeeded. `submission.cpp`
exists only after the complete test pipeline succeeds. A bundle, compilation,
or sample failure therefore cannot leave an artifact that `acc submit` mistakes
for verified output. Temporary source files are cleaned up on success and
failure.

`acc build` and `acc run` keep their current behavior and continue compiling
`main.cpp` directly.

### `acc submit`

`acc submit` no longer bundles, compiles, or runs samples. It performs these
stages in order:

1. Require an interactive terminal.
2. Locate the expected `submission.cpp`.
3. Verify that the artifact is fresh.
4. Display the contest, task, and exact artifact path.
5. Ask for confirmation, defaulting to No.
6. On an explicit `y` or `yes`, invoke the installed raw `atcoder-cli` with the
   artifact path, contest ID, and full task ID.

If the artifact is missing or stale, the command fails before prompting or
invoking `atcoder-cli`, with an error that tells the user to run `acc test`.
Submission failures from `atcoder-cli` continue to propagate without browser,
clipboard, or Web-submission fallback.

## Freshness Rule

Freshness uses filesystem modification times and has no sidecar metadata.
`submission.cpp` is fresh only when it is at least as new as:

- the task's `main.cpp`; and
- every regular file below the repository's `library/` directory.

This includes both vendored ACL files and `atcoder_local` headers. A changed
header therefore cannot be omitted accidentally from the submitted artifact.
A missing `library/` directory, or an unreadable artifact or input timestamp,
is treated as a workflow error, not as fresh output.

## Internal Structure

A focused helper validates the source type, invalidates the published artifact
before compiler detection, bundles a temporary submission candidate, compiles
it with the selected build mode, and returns the candidate source and
executable paths. `run_tests` runs samples against that executable and
publishes the candidate source only when `oj test` returns zero. A `finally`
cleanup removes any unpublished candidate.

A separate freshness helper validates the artifact for `run_submit`. It has no
side effects and reports missing, stale, and timestamp-access errors as
`WorkflowError` values. Keeping preparation and validation separate prevents
`acc submit` from regaining an implicit test or rebuild path.

## Error Handling

- Unsupported non-C++ sources fail before compiler detection or bundling.
- Compiler detection failure leaves no verified artifact.
- Bundle failure stops compilation and sample execution.
- A residual angle-bracket `atcoder_local` include is a bundle failure and
  tells the user to use a quoted include.
- Compile failure stops sample execution and leaves no verified artifact.
- Sample failure returns the `oj test` exit code and leaves no verified
  artifact.
- Missing or stale submission artifacts stop before confirmation.
- Interrupted confirmation remains a workflow error.
- Raw `atcoder-cli` spawn failures and exit codes retain their current
  behavior.

## Tests

Automated tests cover:

- release and debug `acc test` stage order;
- release and debug executable names and build modes;
- selected compiler propagation to `oj-bundle`;
- stopping after bundle, compile, and sample failures;
- publication of `submission.cpp` only after successful samples;
- temporary-source cleanup and invalidation of a previous verified artifact on
  every failed test stage;
- missing, stale-by-source, stale-by-library, and fresh artifacts;
- refusal before prompt and submission for invalid artifacts;
- `acc submit` invoking no compiler, bundler, or sample runner;
- existing prompt, TTY, raw-command argument, interruption, and exit-code
  behavior; and
- README command descriptions and troubleshooting guidance.

## Documentation

The README states that both forms of `acc test` generate and test the bundled
artifact. It states that `acc submit` only accepts a fresh artifact produced by
`acc test` and instructs the user to rerun the test after changing `main.cpp`
or repository-local headers.
