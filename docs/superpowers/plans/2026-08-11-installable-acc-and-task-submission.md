# Installable `acc` Wrapper and Task-Local Submission Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Install the repository workflow as `acc`, publish prepared bundles in each task directory, and allow build and submission preparation when samples are unavailable.

**Architecture:** Make sample availability optional in `TaskContext` and give each context an explicit task-local `submission_path`. Keep binaries in the hidden build tree, skip only the sample-runner stage when `test_dir` is absent, and share one upstream-`acc` locator between native command delegation and submission. Package the Python code as an editable uv tool so a new shell needs no `source` command.

**Tech Stack:** Python 3.10+, setuptools, uv 0.11+, pytest 9.1+, atcoder-cli 2.2.0, bash, zsh, C++23

## Global Constraints

- Keep the public command name exactly `acc`.
- Use `uv tool install --editable .` as the primary one-time installation command.
- Keep upstream npm-installed atcoder-cli as the adapter for native commands and final submission.
- Generate the prepared source at `<task-directory>/submission.cpp`.
- Generate its pending file at `<task-directory>/.submission.cpp.pending`.
- Keep compiled binaries below `.atcoder-local/build/<contest-id>/<task-id>/`.
- Reject a configured source that resolves to the generated `submission.cpp` before deleting or writing either path.
- Treat missing `directory.testdir` metadata and a missing configured sample directory as `test_dir=None`.
- Reject a configured sample path that exists but is not a directory.
- With `test_dir=None`, bundle and compile, skip `oj test`, warn on standard error, publish `submission.cpp`, and return zero.
- A bundle, compile, or actual sample failure must leave no prepared task-local artifact.
- Ignore generated sources only below the documented `contests/` layout.
- Preserve stdio and exit codes for upstream atcoder-cli delegation.
- Preserve bash and zsh shell wrappers as compatibility alternatives.
- Do not change mtime freshness, debug publication, CAPTCHA handling, browser fallback, or clipboard behavior.
- Do not delete legacy hidden `submission.cpp` files automatically.
- Run host-independent tests before claiming completion.

---

### Task 1: Resolve task-local output and optional samples

**Files:**
- Modify: `tools/atcoder_workflow/context.py:16-242`
- Modify: `tests/test_workflow_context.py:12-206`
- Modify: `tests/test_workflow_commands.py:38-72,574-598`

**Interfaces:**
- Consumes: existing `resolve_task_context(cwd, contest_id=None, task_label=None)`.
- Produces: `TaskContext.submission_path: Path`.
- Changes: `TaskContext.test_dir` from `Path` to `Path | None`.
- Produces: a collision `WorkflowError` before generated-file operations.

- [ ] **Step 1: Extend the context test helper for missing sample metadata**

Add a `testdir` parameter to `write_contest`:

```python
def write_contest(
    root: Path,
    relative_directory: str = "contests/abc999",
    *,
    submit: str = "main.cpp",
    contest_id: str = "abc999",
    task_id: str = "abc999_a",
    testdir: str | None = "test",
) -> tuple[Path, Path]:
```

Create the sample directory only when `testdir is not None`, and add the JSON
field only in that case:

```python
directory = {"path": "a", "submit": submit}
if testdir is not None:
    (task_dir / testdir).mkdir()
    directory["testdir"] = testdir
```

- [ ] **Step 2: Write failing tests for task-local output and optional samples**

Extend the normal resolution assertion:

```python
assert context.submission_path == task_dir / "submission.cpp"
assert context.test_dir == task_dir / "test"
```

Add these tests:

```python
def test_resolves_missing_testdir_metadata_as_no_samples(tmp_path: Path) -> None:
    root = repository(tmp_path)
    _, task_dir = write_contest(root, testdir=None)

    context = resolve_task_context(task_dir)

    assert context.test_dir is None


def test_resolves_missing_configured_test_directory_as_no_samples(
    tmp_path: Path,
) -> None:
    root = repository(tmp_path)
    _, task_dir = write_contest(root)
    (task_dir / "test").rmdir()

    context = resolve_task_context(task_dir)

    assert context.test_dir is None


def test_rejects_configured_test_path_that_is_not_directory(
    tmp_path: Path,
) -> None:
    root = repository(tmp_path)
    _, task_dir = write_contest(root)
    (task_dir / "test").rmdir()
    (task_dir / "test").write_text("not a directory", encoding="utf-8")

    with pytest.raises(WorkflowError, match="test path is not a directory"):
        resolve_task_context(task_dir)


def test_rejects_source_named_generated_submission(tmp_path: Path) -> None:
    root = repository(tmp_path)
    _, task_dir = write_contest(root, submit="submission.cpp")

    with pytest.raises(
        WorkflowError,
        match="submit source conflicts with generated submission artifact",
    ):
        resolve_task_context(task_dir)
```

Update all direct `TaskContext(...)` fixtures in
`tests/test_workflow_commands.py` with a task-local `submission_path`.

- [ ] **Step 3: Run the focused tests and verify the red state**

Run:

```sh
uv run python -m pytest tests/test_workflow_context.py -q
```

Expected: FAIL because `submission_path` and optional `test_dir` do not exist.

- [ ] **Step 4: Change the context model and resolver**

Define the fields as:

```python
source_path: Path
submission_path: Path
test_dir: Path | None
build_dir: Path
```

Add an optional metadata reader:

```python
def _optional_directory_value(
    task: dict[str, Any], key: str, config_path: Path
) -> str | None:
    directory = task.get("directory")
    if not isinstance(directory, dict):
        raise WorkflowError(f"task directory is missing from {config_path}")
    value = directory.get(key)
    if value is None:
        return None
    if not isinstance(value, str):
        raise WorkflowError(f"task directory {key} is invalid in {config_path}")
    return value
```

In `_build_context`, resolve output and samples independently:

```python
submission_path = (task_dir / "submission.cpp").resolve()
if source_path == submission_path:
    raise WorkflowError(
        "submit source conflicts with generated submission artifact: "
        f"{submission_path}"
    )

testdir = _optional_directory_value(task, "testdir", config_path)
test_dir: Path | None = None
if testdir is not None:
    configured_test_dir = _resolve_within_owner(
        task_dir, testdir, "test directory"
    )
    if configured_test_dir.exists() and not configured_test_dir.is_dir():
        raise WorkflowError(f"test path is not a directory: {configured_test_dir}")
    if configured_test_dir.is_dir():
        test_dir = configured_test_dir
```

Remove the old unconditional `test_dir.is_dir()` error and return both fields.

- [ ] **Step 5: Run context and command tests**

Run:

```sh
uv run python -m pytest \
  tests/test_workflow_context.py \
  tests/test_workflow_commands.py \
  -q
```

Expected: PASS after direct fixtures provide the new field.

- [ ] **Step 6: Commit the context change**

```sh
git add tools/atcoder_workflow/context.py \
  tests/test_workflow_context.py \
  tests/test_workflow_commands.py
git commit -m "feat: resolve optional task samples"
```

---

### Task 2: Publish task-local submissions with or without samples

**Files:**
- Modify: `tools/atcoder_workflow/commands.py:1-345`
- Modify: `tests/test_workflow_commands.py:79-873`

**Interfaces:**
- Consumes: `TaskContext.submission_path` and optional `TaskContext.test_dir`.
- Produces: task-local pending and prepared sources.
- Produces: warning `[warning] sample tests are unavailable; submission.cpp was not sample-tested` on standard error when samples are absent.

- [ ] **Step 1: Change existing path expectations to task-local paths**

Replace test derivations with:

```python
submission = context.submission_path
published = context.submission_path
candidate = context.task_dir / ".submission.cpp.pending"
```

Keep binaries at:

```python
binary = context.build_dir / "submission-main"
debug_binary = context.build_dir / "submission-main-debug"
```

Update submit display, prompt, and argv expectations to use
`CONTEXT.submission_path`.

- [ ] **Step 2: Add failing no-sample build and test cases**

Add a helper that returns a copy of a context with no samples:

```python
from dataclasses import replace


def without_samples(context: TaskContext) -> TaskContext:
    return replace(context, test_dir=None)
```

Add tests:

```python
def test_build_does_not_require_sample_directory() -> None:
    context = without_samples(CONTEXT)
    with patch(
        "tools.atcoder_workflow.commands.detect_compiler", return_value=GCC15
    ), patch(
        "tools.atcoder_workflow.commands.compile_cpp",
        return_value=context.build_dir / "main",
    ) as compile_:
        assert run_build(context, DEPENDENCIES) == context.build_dir / "main"

    compile_.assert_called_once()


def test_run_does_not_require_sample_directory() -> None:
    context = without_samples(CONTEXT)
    binary = context.build_dir / "main"
    with patch(
        "tools.atcoder_workflow.commands.run_build", return_value=binary
    ) as build, patch(
        "tools.atcoder_workflow.commands.run_binary", return_value=19
    ) as run:
        assert run_program(context, DEPENDENCIES) == 19

    build.assert_called_once_with(context, DEPENDENCIES, mode=BuildMode.RELEASE)
    run.assert_called_once_with(binary, context.task_dir, DEPENDENCIES.runner)


@pytest.mark.parametrize("debug", [False, True])
def test_run_tests_without_samples_warns_and_publishes(
    tmp_path: Path,
    capsys: pytest.CaptureFixture[str],
    debug: bool,
) -> None:
    context = without_samples(submission_context(tmp_path))
    published = context.submission_path

    with patched_test_stages(context) as (events, _, _, samples):
        assert run_tests(context, DEPENDENCIES, debug=debug) == 0

    assert events == ["bundle", "compile"]
    samples.assert_not_called()
    assert published.read_text(encoding="utf-8") == "bundled source\n"
    assert capsys.readouterr().err == (
        "[warning] sample tests are unavailable; "
        "submission.cpp was not sample-tested\n"
    )


def test_submit_accepts_fresh_artifact_without_samples() -> None:
    context = without_samples(CONTEXT)
    events: list[str] = []

    with patched_fresh_submission():
        assert run_submit(context, submit_dependencies(events)) == 0

    assert events == ["prompt", "submit"]
```

Adjust `patched_test_stages` so the sample mock records an event only when
called; the no-sample expectation must prove it was skipped.

- [ ] **Step 3: Run command tests and verify the red state**

Run:

```sh
uv run python -m pytest tests/test_workflow_commands.py -q
```

Expected: FAIL because paths still use `build_dir` and `run_samples` cannot
accept `None`.

- [ ] **Step 4: Move source artifacts and branch on sample availability**

Import `sys`. In `run_tests` and `_prepare_submission_candidate`, use:

```python
candidate_path = context.task_dir / ".submission.cpp.pending"
submission_path = context.submission_path
```

After compilation, replace the unconditional sample call with:

```python
if context.test_dir is None:
    print(
        "[warning] sample tests are unavailable; "
        "submission.cpp was not sample-tested",
        file=sys.stderr,
    )
else:
    sample_returncode = run_samples(
        binary_path, context.test_dir, context.task_dir, dependencies.runner
    )
    if sample_returncode != 0:
        return sample_returncode
```

Then keep the existing atomic publication and `finally` cleanup. In
`_require_fresh_submission`, use:

```python
submission_path = context.submission_path
```

Do not make `run_samples` accept `None`; the command layer owns this branch.

- [ ] **Step 5: Run workflow regression tests**

Run:

```sh
uv run python -m pytest \
  tests/test_workflow_commands.py \
  tests/test_workflow_context.py \
  tests/test_workflow_cpp.py \
  -q
```

Expected: PASS. Sample-backed tasks still invoke `oj test`; sampleless tasks
publish only after bundle and compile succeed.

- [ ] **Step 6: Commit the workflow change**

```sh
git add tools/atcoder_workflow/commands.py tests/test_workflow_commands.py
git commit -m "feat: prepare submissions without samples"
```

---

### Task 3: Install `acc` and locate upstream atcoder-cli safely

**Files:**
- Create: `tools/atcoder_workflow/acc_locator.py`
- Create: `tests/test_acc_locator.py`
- Modify: `tools/acc_wrapper.py:1-36`
- Modify: `tools/atcoder_workflow/cli.py:1-75`
- Modify: `tools/acc-wrapper.bash:1-7`
- Modify: `tools/acc-wrapper.zsh:1-7`
- Modify: `tests/test_acc_wrapper.py:1-155`
- Modify: `tests/test_workflow_commands.py:876-1006`
- Modify: `pyproject.toml:1-14`
- Modify: `uv.lock`

**Interfaces:**
- Produces: `find_upstream_acc(environ, wrapper_path=None) -> str | None`.
- Consumes: optional `ATCODER_LOCAL_RAW_ACC` captured by shell wrappers.
- Produces: `console_main() -> NoReturn`, registered as console script `acc`.

- [ ] **Step 1: Write failing locator tests**

Create `tests/test_acc_locator.py`:

```python
from __future__ import annotations

import os
from pathlib import Path

from tools.atcoder_workflow.acc_locator import find_upstream_acc


def executable(path: Path) -> Path:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("#!/bin/sh\nexit 0\n", encoding="utf-8")
    path.chmod(0o755)
    return path


def test_skips_active_wrapper_and_returns_next_acc(tmp_path: Path) -> None:
    wrapper = executable(tmp_path / "wrapper/acc")
    upstream = executable(tmp_path / "upstream/acc")
    environ = {"PATH": os.pathsep.join([str(wrapper.parent), str(upstream.parent)])}

    assert find_upstream_acc(environ, wrapper) == str(upstream.resolve())


def test_prefers_valid_shell_override(tmp_path: Path) -> None:
    upstream = executable(tmp_path / "upstream/acc")

    assert find_upstream_acc(
        {"PATH": "", "ATCODER_LOCAL_RAW_ACC": str(upstream)}
    ) == str(upstream.resolve())


def test_falls_back_when_override_is_missing(tmp_path: Path) -> None:
    upstream = executable(tmp_path / "upstream/acc")
    environ = {
        "PATH": str(upstream.parent),
        "ATCODER_LOCAL_RAW_ACC": str(tmp_path / "missing"),
    }

    assert find_upstream_acc(environ) == str(upstream.resolve())


def test_returns_none_without_distinct_acc(tmp_path: Path) -> None:
    wrapper = executable(tmp_path / "wrapper/acc")

    assert find_upstream_acc({"PATH": str(wrapper.parent)}, wrapper) is None
```

- [ ] **Step 2: Write failing wrapper and default-dependency tests**

Change default raw delegation to expect an absolute located executable:

```python
with (
    patch(
        "tools.acc_wrapper.find_upstream_acc",
        return_value="/opt/native/bin/acc",
    ),
    patch("tools.acc_wrapper.subprocess.run", return_value=completed) as run,
):
    result = main(["new", "abc454"])

run.assert_called_once_with(
    ["/opt/native/bin/acc", "new", "abc454"], check=False
)
```

Add a missing-upstream test expecting return code 1 and:

```text
[acc-wrapper] upstream acc executable was not found
```

Add a `console_main` test that patches `main` to return 23 and asserts a
`SystemExit(23)`.

Add a CLI test that patches `find_upstream_acc` and `shutil.which`, then asserts
the default dependency locator uses the former only for `acc` and the latter
for `g++`.

- [ ] **Step 3: Run focused tests and verify the red state**

Run:

```sh
uv run python -m pytest \
  tests/test_acc_locator.py \
  tests/test_acc_wrapper.py \
  -q
```

Expected: collection FAIL because the locator and console entry point are
missing.

- [ ] **Step 4: Implement the side-effect-free upstream locator**

Create `tools/atcoder_workflow/acc_locator.py`:

```python
from __future__ import annotations

from collections.abc import Mapping
import os
from pathlib import Path
import sys


RAW_ACC_ENVIRONMENT_VARIABLE = "ATCODER_LOCAL_RAW_ACC"


def find_upstream_acc(
    environ: Mapping[str, str],
    wrapper_path: Path | str | None = None,
) -> str | None:
    active = Path(sys.argv[0] if wrapper_path is None else wrapper_path).resolve()
    override = environ.get(RAW_ACC_ENVIRONMENT_VARIABLE)
    if override:
        candidate = Path(override).expanduser().resolve()
        if _is_executable(candidate) and candidate != active:
            return str(candidate)

    for directory in environ.get("PATH", "").split(os.pathsep):
        if not directory:
            continue
        candidate = (Path(directory).expanduser() / "acc").resolve()
        if candidate != active and _is_executable(candidate):
            return str(candidate)
    return None


def _is_executable(path: Path) -> bool:
    return path.is_file() and os.access(path, os.X_OK)
```

- [ ] **Step 5: Wire the wrapper and workflow CLI to the locator**

In `tools/acc_wrapper.py`, import the locator for both package and direct-script
execution:

```python
from typing import NoReturn

if __package__:
    from .atcoder_workflow.acc_locator import find_upstream_acc
else:
    from atcoder_workflow.acc_locator import find_upstream_acc
```

Replace default raw delegation:

```python
raw_acc = find_upstream_acc(os.environ)
if raw_acc is None:
    print("[acc-wrapper] upstream acc executable was not found", file=sys.stderr)
    return 1
return subprocess.run([raw_acc, *args], check=False).returncode
```

Add:

```python
def console_main() -> NoReturn:
    raise SystemExit(main())
```

In `tools/atcoder_workflow/cli.py`, add:

```python
def _locate_executable(name: str) -> str | None:
    if name == "acc":
        return find_upstream_acc(os.environ)
    return shutil.which(name)
```

Pass `_locate_executable` as `WorkflowDependencies.which`.

- [ ] **Step 6: Preserve shell-wrapper compatibility**

Before each shell function definition, capture the current native executable:

```sh
_atcoder_local_raw_acc="$(command -v acc 2>/dev/null || true)"
```

Pass it into the Python process:

```sh
ATCODER_LOCAL_WRAPPER=1 \
ATCODER_LOCAL_RAW_ACC="${_atcoder_local_raw_acc}" \
uv run python "${_atcoder_local_root}/tools/acc_wrapper.py" "$@"
```

Update shell tests with fake `uv` and fake `acc`, and assert both environment
markers are forwarded. The lookup must occur before the function definition.

- [ ] **Step 7: Add package metadata and install the development project**

Add to `pyproject.toml`:

```toml
[project.scripts]
acc = "tools.acc_wrapper:console_main"

[build-system]
requires = ["setuptools>=80,<81"]
build-backend = "setuptools.build_meta"

[tool.setuptools.packages.find]
include = ["tools*"]
```

Run:

```sh
uv lock
uv sync --group dev
```

Expected: `.venv/bin/acc` exists and imports this checkout.

- [ ] **Step 8: Verify tests and package build**

Run:

```sh
uv run python -m pytest \
  tests/test_acc_locator.py \
  tests/test_acc_wrapper.py \
  tests/test_workflow_commands.py \
  -q
uv build
```

Expected: tests PASS, and both wheel and sdist build. Inspect the wheel with:

```sh
unzip -p dist/atcoder_local-0.1.0-py3-none-any.whl '*/entry_points.txt'
```

Expected entry:

```ini
[console_scripts]
acc = tools.acc_wrapper:console_main
```

- [ ] **Step 9: Commit the installable wrapper**

```sh
git add pyproject.toml uv.lock \
  tools/acc_wrapper.py \
  tools/acc-wrapper.bash \
  tools/acc-wrapper.zsh \
  tools/atcoder_workflow/acc_locator.py \
  tools/atcoder_workflow/cli.py \
  tests/test_acc_locator.py \
  tests/test_acc_wrapper.py \
  tests/test_workflow_commands.py
git commit -m "feat: install repository acc wrapper"
```

---

### Task 4: Ignore generated files, document setup, and verify end to end

**Files:**
- Modify: `.gitignore:16-21`
- Modify: `README.md:18-133`
- Modify: `tests/test_workflow_commands.py:237-287`

**Interfaces:**
- Consumes: task-local publication, optional samples, and installed entry point.
- Produces: documented one-time install/update process and scoped ignore rules.

- [ ] **Step 1: Write failing README and ignore-rule tests**

Add:

```python
def test_readme_documents_installed_acc_and_task_local_submission() -> None:
    readme = (REPOSITORY_ROOT / "README.md").read_text(encoding="utf-8")

    assert "uv tool install --editable ." in readme
    assert "uv tool install --force --editable ." in readme
    assert "各タスクディレクトリの `submission.cpp`" in readme
    assert "サンプルテストがない場合" in readme
    assert "サンプルテストを省略" in readme


def test_gitignore_scopes_generated_submission_to_contests() -> None:
    ignore = (REPOSITORY_ROOT / ".gitignore").read_text(encoding="utf-8")
    lines = {line.strip() for line in ignore.splitlines()}

    assert "contests/**/submission.cpp" in lines
    assert "contests/**/.submission.cpp.pending" in lines
    assert "**/submission.cpp" not in lines
```

Update old assertions that say every source artifact lives below
`.atcoder-local/`.

- [ ] **Step 2: Run focused tests and verify the red state**

Run:

```sh
uv run pytest \
  tests/test_workflow_commands.py::test_readme_documents_installed_acc_and_task_local_submission \
  tests/test_workflow_commands.py::test_gitignore_scopes_generated_submission_to_contests \
  -v
```

Expected: FAIL because documentation and ignore rules still describe the old
layout. `uv run pytest` must now collect without the former `tools` import
error because the project is installed.

- [ ] **Step 3: Add scoped ignore rules**

Append:

```gitignore
# Generated submission artifacts
contests/**/submission.cpp
contests/**/.submission.cpp.pending
```

Do not add a global `submission.cpp` pattern.

- [ ] **Step 4: Rewrite README setup and workflow sections**

Document primary setup in this order:

```sh
uv sync --group dev
npm install -g atcoder-cli@2.2.0
uv tool install --editable .
```

Document metadata/dependency refresh:

```sh
uv tool install --force --editable .
```

State that new shells can immediately use `acc`; move manual bash/zsh sourcing
to a compatibility subsection and tell users not to combine both entry paths in
one shell.

Document that:

- `acc build` and `acc run` do not need downloaded samples;
- `acc test` publishes beside `main.cpp` after successful samples;
- without samples it bundles, compiles, warns, skips `oj test`, and publishes;
- `acc submit` reads the task-local prepared file;
- generated source files are ignored under `contests/`; and
- binaries remain below `.atcoder-local/build/`.

Retain the current freshness, confirmation, CAPTCHA, and submission-history
warnings.

- [ ] **Step 5: Run the complete host-independent suite**

Run:

```sh
uv run pytest -q
```

Expected: all tests PASS with no collection errors.

- [ ] **Step 6: Verify concrete ignore paths**

Run:

```sh
git check-ignore -v \
  contests/abc999/a/submission.cpp \
  contests/abc999/a/.submission.cpp.pending
```

Expected: both paths match the new rules.

Run:

```sh
git check-ignore submission.cpp
```

Expected: exit code 1 and no output.

- [ ] **Step 7: Smoke-test an isolated editable tool install**

Use a temporary tool home so the user's real `acc` is untouched:

```sh
tool_tmp="$(mktemp -d)"
mkdir -p "$tool_tmp/tool-dir" "$tool_tmp/bin" "$tool_tmp/native"
FAKE_ACC_PATH="$tool_tmp/native/acc" uv run python -c \
  'import os; from pathlib import Path; Path(os.environ["FAKE_ACC_PATH"]).write_text("#!/bin/sh\necho upstream:$*\nexit 23\n", encoding="utf-8")'
chmod +x "$tool_tmp/native/acc"
UV_TOOL_DIR="$tool_tmp/tool-dir" \
UV_TOOL_BIN_DIR="$tool_tmp/bin" \
uv tool install --offline --editable .
PATH="$tool_tmp/bin:$tool_tmp/native:$PATH" \
"$tool_tmp/bin/acc" config default-task-choice
```

Expected stdout is `upstream:config default-task-choice` and exit code is 23,
proving delegation skips the wrapper. Remove only the validated temporary path:

```sh
rm -rf "$tool_tmp"
```

- [ ] **Step 8: Commit documentation and ignore rules**

Run:

```sh
git diff --check
git status --short
```

Then commit:

```sh
git add .gitignore README.md tests/test_workflow_commands.py
git commit -m "docs: explain installed sample-optional workflow"
```

- [ ] **Step 9: Run final verification from a clean worktree**

Run:

```sh
git status --short
uv run pytest -q
uv build
```

Expected: clean status, full test success, and successful wheel/sdist build.
Do not claim completion without observing all three results.
