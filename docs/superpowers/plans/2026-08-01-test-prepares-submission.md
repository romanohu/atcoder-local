# `acc test` Submission Artifact Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make both forms of `acc test` build and verify `submission.cpp`, while `acc submit` only accepts and submits a fresh verified artifact.

**Architecture:** Keep command orchestration in `tools/atcoder_workflow/commands.py`. Add a private test-preparation pipeline that uses a temporary bundled candidate and publishes it atomically only after samples pass; add a separate, side-effect-free freshness validator used by submission. Preserve the existing process-runner boundary so unit tests can verify external command arguments without invoking AtCoder tools.

**Tech Stack:** Python 3.9+, `pathlib`, `pytest`, `unittest.mock`, existing `oj-bundle`, `oj test`, and `atcoder-cli` adapters.

## Global Constraints

- Both `acc test` and `acc test --debug` must generate and test a bundled submission candidate.
- The C++ template must quote repository-only `atcoder_local` includes so `oj-bundle` 5.6.0 expands them.
- Bundling must reject any residual angle-bracket `atcoder_local` include instead of publishing a repository-dependent artifact.
- Standard AtCoder Library angle-bracket includes remain supported because AtCoder provides ACL in the judge environment.
- Publish `.atcoder-local/build/<contest>/<task>/submission.cpp` only after all samples pass.
- A failed test stage must leave no published or pending submission artifact.
- Release tests use `submission-main`; debug tests use `submission-main-debug` with the existing debug flags.
- `acc build` and `acc run` continue compiling `main.cpp` directly.
- `acc submit` must not bundle, compile, or run samples.
- A submission artifact is fresh only when it is at least as new as `main.cpp` and every regular file under `library/`.
- Freshness uses modification times and no sidecar metadata.
- Missing or stale artifacts must fail before confirmation and tell the user to run `acc test`.
- Preserve the existing explicit confirmation, raw `atcoder-cli` invocation, exit-code propagation, and no-fallback policy.
- Implement every behavior change with a failing test first.

---

## File Structure

- `tools/atcoder_workflow/commands.py`: orchestrates the new test pipeline, manages pending/published artifact paths, validates freshness, and simplifies submission.
- `tests/test_workflow_commands.py`: specifies release/debug test pipelines, failure cleanup, freshness behavior, and submission boundaries.
- `README.md`: documents the new ownership split between `acc test` and `acc submit`.

### Task 0: Make repository-local header bundling reliable

**Files:**
- Modify: `template/cpp/main.cpp:4-6`
- Modify: `tools/atcoder_workflow/cpp.py:42-72`
- Test: `tests/test_workflow_cpp.py:115-175`
- Test: `tests/test_cpp_library.py:82-145`

**Interfaces:**
- Consumes: `oj-bundle` output captured by `bundle_cpp`.
- Produces: quoted `atcoder_local` template includes; private `_reject_unbundled_local_includes(source: str, source_path: Path) -> None` called before writing the bundled artifact.

- [ ] **Step 1: Write failing tests for quoted template includes and residual local-header rejection**

Add a template contract test:

```python
def test_cpp_template_quotes_repository_local_headers() -> None:
    template = (REPOSITORY_ROOT / "template/cpp/main.cpp").read_text(
        encoding="utf-8"
    )

    assert '#include "atcoder_local/core.hpp"' in template
    assert '#include "atcoder_local/debug.hpp"' in template
    assert '#include "atcoder_local/io.hpp"' in template
    assert "#include <atcoder_local/" not in template
```

Add a `bundle_cpp` boundary test whose fake runner returns:

```python
'#line 1 "main.cpp"\n#include <atcoder_local/core.hpp>\nint main() {}\n'
```

Assert that `bundle_cpp` raises
`WorkflowError` matching `unbundled local include.*use quotes`, preserves an
existing output file, and removes its temporary file.

- [ ] **Step 2: Run the focused tests and verify RED**

Run:

```sh
uv run python -m pytest \
  tests/test_workflow_cpp.py \
  tests/test_cpp_library.py \
  -k 'unbundled_local_include or template_quotes' \
  -v
```

Expected: FAIL because the template uses angle brackets and `bundle_cpp`
accepts residual local includes.

- [ ] **Step 3: Implement the minimal template and bundle validation changes**

Change only the three `atcoder_local` lines in `template/cpp/main.cpp` to
quoted includes. In `cpp.py`, validate `result.stdout` after a successful
`oj-bundle` exit and before `temporary.write_text`:

```python
UNBUNDLED_LOCAL_INCLUDE = re.compile(
    r'^\s*#\s*include\s*<atcoder_local/[^>]+>\s*$', re.MULTILINE
)


def _reject_unbundled_local_includes(source: str, source_path: Path) -> None:
    if UNBUNDLED_LOCAL_INCLUDE.search(source):
        raise WorkflowError(
            f"bundle left an unbundled local include in {source_path}; "
            'use quotes for atcoder_local headers, for example '
            '#include "atcoder_local/core.hpp"'
        )
```

Import `re`. Let the existing `finally` cleanup preserve the old output and
remove the temporary file.

- [ ] **Step 4: Run the focused tests and verify GREEN**

Run the Step 2 command again. Expected: PASS.

- [ ] **Step 5: Correct the integration test to reflect the supported judge boundary**

Rename `test_bundled_submission_is_self_contained` to
`test_bundled_submission_inlines_repository_local_headers`. Change its
`atcoder_local/core.hpp` include to quotes, keep `<atcoder/dsu>` as the
standard AtCoder-provided ACL include, assert the bundled source does not
contain `#include <atcoder_local/` or `#include "atcoder_local/`, and compile
with `-I <repository>/library` so the local test environment supplies ACL.

- [ ] **Step 6: Run the baseline suite and verify the pre-existing failure is resolved**

Run:

```sh
uv run python -m pytest -q
```

Expected: 257 tests pass with 93 subtests and no failures.

- [ ] **Step 7: Commit Task 0**

```sh
git add template/cpp/main.cpp tools/atcoder_workflow/cpp.py tests/test_workflow_cpp.py tests/test_cpp_library.py
git commit -m "fix: bundle repository-local headers"
```

### Task 1: Move bundling and artifact publication into `acc test`

**Files:**
- Modify: `tools/atcoder_workflow/commands.py:176-275`
- Test: `tests/test_workflow_commands.py:150-310`

**Interfaces:**
- Consumes: `TaskContext`, `WorkflowDependencies`, `BuildMode`, `detect_compiler`, `bundle_cpp`, `compile_cpp`, and `run_samples`.
- Produces: `run_tests(context, dependencies, debug=False) -> int` with the new pipeline; private `_prepare_submission_candidate(context, dependencies, mode) -> tuple[Path, Path]`; private `_discard_generated_file(path, description) -> None`.

- [ ] **Step 1: Replace the existing direct-build test with a failing release-pipeline test**

Add a temporary-filesystem test that records the real orchestration order and writes the bundle candidate through the patched external boundary:

```python
def test_run_tests_bundles_compiles_samples_then_publishes_release_artifact(
    tmp_path: Path,
) -> None:
    context = submission_context(tmp_path)
    events: list[str] = []
    published = context.build_dir / "submission.cpp"
    candidate = context.build_dir / ".submission.cpp.pending"
    binary = context.build_dir / "submission-main"

    def bundle_stage(**kwargs: object) -> Path:
        events.append("bundle")
        output = kwargs["output_path"]
        assert isinstance(output, Path)
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text("bundled source\n", encoding="utf-8")
        return output

    with (
        patch("tools.atcoder_workflow.commands.detect_compiler", return_value=GCC15),
        patch("tools.atcoder_workflow.commands.bundle_cpp", side_effect=bundle_stage) as bundle,
        patch(
            "tools.atcoder_workflow.commands.compile_cpp",
            side_effect=lambda **kwargs: events.append("compile") or kwargs["output_path"],
        ) as compile_,
        patch(
            "tools.atcoder_workflow.commands.run_samples",
            side_effect=lambda *args: events.append("samples") or 0,
        ) as samples,
    ):
        result = run_tests(context, DEPENDENCIES)

    assert result == 0
    assert events == ["bundle", "compile", "samples"]
    assert published.read_text(encoding="utf-8") == "bundled source\n"
    assert not candidate.exists()
    bundle.assert_called_once_with(
        source_path=context.source_path,
        output_path=candidate,
        working_dir=context.task_dir,
        library_dir=context.repository_root / "library",
        runner=DEPENDENCIES.runner,
        environment={**DEPENDENCIES.environ, "CXX": GCC15.executable},
    )
    assert compile_.call_args.kwargs["source_path"] == candidate
    assert compile_.call_args.kwargs["output_path"] == binary
    assert compile_.call_args.kwargs["mode"] is BuildMode.RELEASE
    samples.assert_called_once_with(
        binary, context.test_dir, context.task_dir, DEPENDENCIES.runner
    )


def test_run_tests_compiler_failure_invalidates_previous_submission(
    tmp_path: Path,
) -> None:
    context = submission_context(tmp_path)
    published = context.build_dir / "submission.cpp"
    published.parent.mkdir(parents=True, exist_ok=True)
    published.write_text("old verified source\n", encoding="utf-8")

    with patch(
        "tools.atcoder_workflow.commands.detect_compiler",
        side_effect=WorkflowError("compiler detection failed"),
    ):
        with pytest.raises(WorkflowError, match="compiler detection failed"):
            run_tests(context, DEPENDENCIES)

    assert not published.exists()
```

Define `submission_context(tmp_path)` beside the existing test constants. It creates `main.cpp`, `test/`, `library/atcoder_local/core.hpp`, and a matching `TaskContext` rooted below `tmp_path`.

```python
def submission_context(tmp_path: Path) -> TaskContext:
    repository_root = tmp_path / "repo"
    task_dir = repository_root / "contests/abc999/a"
    source_path = task_dir / "main.cpp"
    test_dir = task_dir / "test"
    library_header = repository_root / "library/atcoder_local/core.hpp"
    test_dir.mkdir(parents=True)
    library_header.parent.mkdir(parents=True)
    source_path.write_text("int main() {}\n", encoding="utf-8")
    library_header.write_text("#pragma once\n", encoding="utf-8")
    return TaskContext(
        repository_root=repository_root,
        contest_id="abc999",
        task_id="abc999_a",
        task_label="A",
        contest_dir=task_dir.parent,
        task_dir=task_dir,
        source_path=source_path,
        test_dir=test_dir,
        build_dir=repository_root / ".atcoder-local/build/abc999/abc999_a",
    )
```

- [ ] **Step 2: Run the release-pipeline test and verify RED**

Run:

```sh
uv run pytest \
  tests/test_workflow_commands.py::test_run_tests_bundles_compiles_samples_then_publishes_release_artifact \
  tests/test_workflow_commands.py::test_run_tests_compiler_failure_invalidates_previous_submission \
  -v
```

Expected: both FAIL because `run_tests` still calls `run_build`, never calls
`bundle_cpp`, does not publish `submission.cpp`, and leaves the previous
artifact in place when compiler detection fails.

- [ ] **Step 3: Implement the minimal release candidate pipeline**

In `commands.py`, make `run_tests` select a build mode and call a private preparation helper. The helper must:

```python
def _prepare_submission_candidate(
    context: TaskContext,
    dependencies: WorkflowDependencies,
    mode: BuildMode,
) -> tuple[Path, Path]:
    _require_cpp_source(context)
    submission_path = context.build_dir / "submission.cpp"
    candidate_path = context.build_dir / ".submission.cpp.pending"
    binary_path = context.build_dir / "submission-main"
    library_dir = context.repository_root / "library"

    _discard_generated_file(submission_path, "verified submission artifact")
    _discard_generated_file(candidate_path, "pending submission artifact")
    compiler = detect_compiler(
        dependencies.environ, dependencies.runner, dependencies.which
    )
    bundle_environment = dict(dependencies.environ)
    bundle_environment["CXX"] = compiler.executable
    bundle_cpp(
        source_path=context.source_path,
        output_path=candidate_path,
        working_dir=context.task_dir,
        library_dir=library_dir,
        runner=dependencies.runner,
        environment=bundle_environment,
    )
    compile_cpp(
        source_path=candidate_path,
        output_path=binary_path,
        working_dir=context.task_dir,
        compiler=compiler,
        mode=mode,
        library_dir=library_dir,
        runner=dependencies.runner,
    )
    return candidate_path, binary_path
```

For this first GREEN step, `run_tests` runs samples and atomically replaces
`submission.cpp` only on return code zero. Do not add failure cleanup or the
debug-specific executable name yet; the following RED cycles specify those
behaviors. Normalize unlink/replace `OSError` values into `WorkflowError`
messages that name the affected path.

Use this exact cleanup primitive:

```python
def _discard_generated_file(path: Path, description: str) -> None:
    try:
        path.unlink(missing_ok=True)
    except OSError as error:
        raise WorkflowError(f"cannot remove {description}: {path}") from error
```

Wrap `candidate_path.replace(submission_path)` and raise
`WorkflowError(f"cannot publish submission artifact: {submission_path}")`
from any `OSError`.

- [ ] **Step 4: Run the release-pipeline test and verify GREEN**

Run:

```sh
uv run pytest \
  tests/test_workflow_commands.py::test_run_tests_bundles_compiles_samples_then_publishes_release_artifact \
  tests/test_workflow_commands.py::test_run_tests_compiler_failure_invalidates_previous_submission \
  -v
```

Expected: PASS.

- [ ] **Step 5: Add failing debug-output and failure-cleanup tests**

Replace the old `run_build`-based debug test and submit-stage failure test with:

```python
def test_run_tests_debug_compiles_pending_bundle_to_debug_output(tmp_path: Path) -> None:
    context = submission_context(tmp_path)
    with patched_test_stages(context) as (events, _, compile_, samples):
        assert run_tests(context, DEPENDENCIES, debug=True) == 0

    assert events == ["bundle", "compile", "samples"]
    assert compile_.call_args.kwargs["source_path"] == (
        context.build_dir / ".submission.cpp.pending"
    )
    assert compile_.call_args.kwargs["output_path"] == (
        context.build_dir / "submission-main-debug"
    )
    assert compile_.call_args.kwargs["mode"] is BuildMode.DEBUG
    samples.assert_called_once()


@pytest.mark.parametrize("failed_stage", ["bundle", "compile", "samples"])
def test_run_tests_failure_invalidates_submission_and_cleans_candidate(
    tmp_path: Path,
    failed_stage: str,
) -> None:
    context = submission_context(tmp_path)
    published = context.build_dir / "submission.cpp"
    candidate = context.build_dir / ".submission.cpp.pending"
    published.parent.mkdir(parents=True, exist_ok=True)
    published.write_text("old verified source\n", encoding="utf-8")

    with patched_test_stages(context, failed_stage=failed_stage) as (events, _, _, _):
        if failed_stage == "samples":
            assert run_tests(context, DEPENDENCIES) == 17
        else:
            with pytest.raises(WorkflowError, match=f"{failed_stage} failed"):
                run_tests(context, DEPENDENCIES)

    assert not published.exists()
    assert not candidate.exists()
    assert events == {
        "bundle": ["bundle"],
        "compile": ["bundle", "compile"],
        "samples": ["bundle", "compile", "samples"],
    }[failed_stage]


```

Implement the stage boundary helper as follows:

```python
@contextmanager
def patched_test_stages(
    context: TaskContext,
    *,
    failed_stage: str | None = None,
) -> Iterator[tuple[list[str], Mock, Mock, Mock]]:
    events: list[str] = []

    def bundle_stage(**kwargs: object) -> Path:
        events.append("bundle")
        if failed_stage == "bundle":
            raise WorkflowError("bundle failed")
        output = kwargs["output_path"]
        assert isinstance(output, Path)
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text("bundled source\n", encoding="utf-8")
        return output

    def compile_stage(**kwargs: object) -> Path:
        events.append("compile")
        if failed_stage == "compile":
            raise WorkflowError("compile failed")
        output = kwargs["output_path"]
        assert isinstance(output, Path)
        return output

    def sample_stage(*args: object, **kwargs: object) -> int:
        del args, kwargs
        events.append("samples")
        return 17 if failed_stage == "samples" else 0

    with (
        patch("tools.atcoder_workflow.commands.detect_compiler", return_value=GCC15),
        patch("tools.atcoder_workflow.commands.bundle_cpp", side_effect=bundle_stage) as bundle,
        patch("tools.atcoder_workflow.commands.compile_cpp", side_effect=compile_stage) as compile_,
        patch("tools.atcoder_workflow.commands.run_samples", side_effect=sample_stage) as samples,
    ):
        yield events, bundle, compile_, samples
```

- [ ] **Step 6: Run the new tests and verify RED**

Run:

```sh
uv run pytest tests/test_workflow_commands.py -k 'run_tests and (debug or failure)' -v
```

Expected: the debug test FAILS because the release-only implementation writes
`submission-main`; the compile- and sample-failure cases FAIL because the
pending source remains.

- [ ] **Step 7: Complete cleanup and debug behavior, then verify GREEN**

Select the executable name by build mode and wrap preparation, sample
execution, and publication in `try`/`finally`:

```python
mode = BuildMode.DEBUG if debug else BuildMode.RELEASE
candidate_path = context.build_dir / ".submission.cpp.pending"
submission_path = context.build_dir / "submission.cpp"
try:
    prepared_candidate, binary_path = _prepare_submission_candidate(
        context, dependencies, mode
    )
    assert prepared_candidate == candidate_path
    sample_returncode = run_samples(
        binary_path, context.test_dir, context.task_dir, dependencies.runner
    )
    if sample_returncode != 0:
        return sample_returncode
    try:
        candidate_path.replace(submission_path)
    except OSError as error:
        raise WorkflowError(
            f"cannot publish submission artifact: {submission_path}"
        ) from error
    return 0
finally:
    _discard_generated_file(candidate_path, "pending submission artifact")
```

Change `_prepare_submission_candidate` to choose:

```python
binary_name = (
    "submission-main-debug" if mode is BuildMode.DEBUG else "submission-main"
)
```

The published artifact is removed before bundling begins and is never restored
after a failed stage.

Rename `test_failed_build_stops_before_later_stage` to
`test_failed_build_stops_before_run` and leave only the `run_program` case;
the new parametrized pipeline test now owns all `run_tests` failure behavior.

Run:

```sh
uv run pytest tests/test_workflow_commands.py -k 'run_tests or failed_build' -v
```

Expected: PASS, including the unchanged `run_program` build-failure behavior.

- [ ] **Step 8: Commit Task 1**

```sh
git add tools/atcoder_workflow/commands.py tests/test_workflow_commands.py
git commit -m "feat: prepare submission artifact during tests"
```

### Task 2: Require a fresh artifact and simplify `acc submit`

**Files:**
- Modify: `tools/atcoder_workflow/commands.py:212-285`
- Test: `tests/test_workflow_commands.py:310-735`

**Interfaces:**
- Consumes: published `submission.cpp`, `TaskContext.source_path`, and regular files below `<repository_root>/library`.
- Produces: private `_require_fresh_submission(context: TaskContext) -> Path`; `run_submit(context, dependencies) -> int` with no build or test stages.

- [ ] **Step 1: Write failing black-box tests for missing and stale artifacts**

Add `import os`, then add tests using real temporary files and modification
times:

```python
def set_mtime(path: Path, value: int) -> None:
    path.touch(exist_ok=True)
    os.utime(path, ns=(value, value))


def test_submit_rejects_missing_artifact_before_prompt(tmp_path: Path) -> None:
    context = submission_context(tmp_path)
    events: list[str] = []

    with pytest.raises(WorkflowError, match=r"run acc test first"):
        run_submit(context, submit_dependencies(events))

    assert events == []


@pytest.mark.parametrize("newer_input", ["source", "library"])
def test_submit_rejects_artifact_older_than_submission_input(
    tmp_path: Path,
    newer_input: str,
) -> None:
    context = submission_context(tmp_path)
    events: list[str] = []
    submission = context.build_dir / "submission.cpp"
    submission.parent.mkdir(parents=True, exist_ok=True)
    header = context.repository_root / "library/atcoder_local/core.hpp"
    set_mtime(context.source_path, 100)
    set_mtime(header, 100)
    set_mtime(submission, 200)
    set_mtime(context.source_path if newer_input == "source" else header, 300)

    with pytest.raises(WorkflowError, match=r"stale.*run acc test first"):
        run_submit(context, submit_dependencies(events))

    assert events == []


def test_submit_accepts_artifact_newer_than_source_and_library(
    tmp_path: Path,
) -> None:
    context = submission_context(tmp_path)
    events: list[str] = []
    submission = context.build_dir / "submission.cpp"
    submission.parent.mkdir(parents=True, exist_ok=True)
    header = context.repository_root / "library/atcoder_local/core.hpp"
    set_mtime(context.source_path, 100)
    set_mtime(header, 100)
    set_mtime(submission, 200)

    assert run_submit(context, submit_dependencies(events)) == 0
    assert events == ["prompt", "submit"]


def test_submit_rejects_missing_library_before_prompt(tmp_path: Path) -> None:
    context = submission_context(tmp_path)
    events: list[str] = []
    submission = context.build_dir / "submission.cpp"
    submission.parent.mkdir(parents=True, exist_ok=True)
    submission.write_text("bundled source\n", encoding="utf-8")
    library_dir = context.repository_root / "library"
    library_dir.rename(context.repository_root / "library-hidden")

    with pytest.raises(WorkflowError, match=r"library directory"):
        run_submit(context, submit_dependencies(events))

    assert events == []
```

- [ ] **Step 2: Run the artifact-validation tests and verify RED**

Run:

```sh
uv run pytest tests/test_workflow_commands.py -k 'submit_rejects_missing_artifact or submit_rejects_artifact_older or submit_rejects_missing_library or submit_accepts_artifact_newer' -v
```

Expected: FAIL because current `run_submit` starts compiler detection and bundling instead of checking the existing artifact.

- [ ] **Step 3: Implement `_require_fresh_submission` and remove verification stages from submit**

Implement the validator with this control flow:

```python
def _require_fresh_submission(context: TaskContext) -> Path:
    submission_path = context.build_dir / "submission.cpp"
    library_dir = context.repository_root / "library"
    if not submission_path.is_file():
        raise WorkflowError(
            f"submission artifact is missing: {submission_path}; run acc test first"
        )
    if not library_dir.is_dir():
        raise WorkflowError(f"library directory does not exist: {library_dir}")

    try:
        submission_mtime = submission_path.stat().st_mtime_ns
        inputs = [
            context.source_path,
            *(path for path in library_dir.rglob("*") if path.is_file()),
        ]
        stale_input = next(
            (path for path in inputs if path.stat().st_mtime_ns > submission_mtime),
            None,
        )
    except OSError as error:
        raise WorkflowError(
            f"cannot validate submission artifact: {submission_path}"
        ) from error

    if stale_input is not None:
        raise WorkflowError(
            f"submission artifact is stale: {stale_input} is newer; "
            "run acc test first"
        )
    return submission_path
```

Keep `_require_cpp_source(context)` first. Then require an interactive terminal, call `_require_fresh_submission`, print the existing summary, prompt, and invoke raw `acc`. Delete compiler detection, bundle, compilation, and sample calls from `run_submit`.

- [ ] **Step 4: Run the validation tests and verify GREEN**

Run:

```sh
uv run pytest tests/test_workflow_commands.py -k 'submit_rejects_missing_artifact or submit_rejects_artifact_older or submit_rejects_missing_library or submit_accepts_artifact_newer' -v
```

Expected: PASS.

- [ ] **Step 5: Rewrite existing submit tests around a fresh-artifact fixture**

Create a context manager that patches only the validator for command-boundary tests:

```python
@contextmanager
def patched_fresh_submission(context: TaskContext = CONTEXT) -> Iterator[Path]:
    submission = context.build_dir / "submission.cpp"
    with patch(
        "tools.atcoder_workflow.commands._require_fresh_submission",
        return_value=submission,
    ) as validate:
        yield submission
    validate.assert_called_once_with(context)
```

Update prompt, confirmation, raw-command, spawn-error, control-flow, and exit-code tests to use this helper. Their expected event lists become `[]`, `["prompt"]`, or `["prompt", "submit"]`; none may contain `bundle`, `compile`, or `samples`.

Add an explicit boundary test:

```python
def test_submit_never_builds_or_runs_samples() -> None:
    events: list[str] = []
    with (
        patched_fresh_submission(),
        patch("tools.atcoder_workflow.commands.detect_compiler") as detect,
        patch("tools.atcoder_workflow.commands.bundle_cpp") as bundle,
        patch("tools.atcoder_workflow.commands.compile_cpp") as compile_,
        patch("tools.atcoder_workflow.commands.run_samples") as samples,
    ):
        assert run_submit(CONTEXT, submit_dependencies(events)) == 0

    detect.assert_not_called()
    bundle.assert_not_called()
    compile_.assert_not_called()
    samples.assert_not_called()
    assert events == ["prompt", "submit"]
```

- [ ] **Step 6: Run all submit tests and verify GREEN**

Run:

```sh
uv run pytest tests/test_workflow_commands.py -k submit -v
```

Expected: PASS. Confirm that the exact raw command remains:

```text
acc submit <build-dir>/submission.cpp -c <contest-id> -t <full-task-id>
```

- [ ] **Step 7: Commit Task 2**

```sh
git add tools/atcoder_workflow/commands.py tests/test_workflow_commands.py
git commit -m "feat: submit only fresh tested artifacts"
```

### Task 3: Update user documentation and run full verification

**Files:**
- Modify: `README.md:103-147`
- Test: `tests/test_workflow_commands.py:44-100`

**Interfaces:**
- Consumes: final command behavior from Tasks 1 and 2.
- Produces: documented workflow contract and a regression assertion for its required wording.

- [ ] **Step 1: Write a failing README contract test**

Add:

```python
def test_readme_documents_test_owned_submission_artifact() -> None:
    readme = (ROOT / "README.md").read_text(encoding="utf-8")

    assert "`acc test` と `acc test --debug` は" in readme
    assert "`submission.cpp`" in readme
    assert "サンプルがすべて成功した場合だけ" in readme
    assert "`acc submit` は bundle、コンパイル、サンプルテストを実行しません" in readme
    assert "先に `acc test` を実行" in readme
```

- [ ] **Step 2: Run the README test and verify RED**

Run:

```sh
uv run pytest tests/test_workflow_commands.py::test_readme_documents_test_owned_submission_artifact -v
```

Expected: FAIL because the README still assigns bundling and sample verification to `acc submit`.

- [ ] **Step 3: Update README behavior and troubleshooting text**

Replace the command bullets so they state:

```markdown
- `acc test` と `acc test --debug` は `main.cpp` とローカルヘッダーを
  `submission.cpp` へ bundle し、その単一ファイルを各モードで
  コンパイルしてサンプルを実行します。`submission.cpp` はサンプルが
  すべて成功した場合だけ公開されます。
- `acc submit` は bundle、コンパイル、サンプルテストを実行しません。
  先に `acc test` を実行して生成した、新しい `submission.cpp` だけを
  確認後に提出します。
```

Document that changes to `main.cpp` or any repository-local header make the artifact stale and require another `acc test`. Keep the CAPTCHA and no-browser/no-clipboard limitations unchanged.

- [ ] **Step 4: Run documentation and focused workflow tests**

Run:

```sh
uv run pytest tests/test_workflow_commands.py -v
```

Expected: PASS.

- [ ] **Step 5: Run the complete suite and repository checks**

Run:

```sh
uv run pytest -v
git diff --check
git status --short
```

Expected: all tests PASS; `git diff --check` prints nothing; status lists only the intended implementation, tests, README, and this plan if it has not already been committed.

- [ ] **Step 6: Commit Task 3**

```sh
git add README.md tests/test_workflow_commands.py docs/superpowers/plans/2026-08-01-test-prepares-submission.md
git commit -m "docs: explain test-built submission workflow"
```
