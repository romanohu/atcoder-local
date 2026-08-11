# Installed `acc` Discovery Fix Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the installed `acc` wrapper discover npm `atcoder-cli` and its own uv tool dependencies without shell sourcing, while documenting a source-independent installation.

**Architecture:** Extend the existing native-`acc` locator with a bounded npm global-prefix fallback after the environment override and `PATH` scan. At the installed console boundary, prepend the active Python interpreter's directory to `PATH` so sibling `oj` entry points are inherited by local and delegated subprocesses. Keep packaging unchanged and switch the documented normal installation from editable to non-editable.

**Tech Stack:** Python 3.10+, `pathlib`, `shutil`, `subprocess`, setuptools console scripts, uv, pytest 9

## Global Constraints

- Keep the public command names, including `acc test`, unchanged.
- Require no shell `source` command for the normal installation.
- Continue supporting only the pinned `atcoder-cli@2.2.0` adapter documented by the repository.
- Do not edit shell startup files or install system dependencies automatically.
- Do not change build, bundle, sample-test, or submission behavior.
- Do not write credentials or AtCoder runtime state during tests.
- Run the complete host-independent pytest suite before completion.

---

### Task 1: Discover npm-global `atcoder-cli` outside `PATH`

**Files:**
- Modify: `tools/atcoder_workflow/acc_locator.py`
- Test: `tests/test_acc_locator.py`

**Interfaces:**
- Consumes: `find_upstream_acc(environ: Mapping[str, str], wrapper_path: Path | str | None = None) -> str | None`
- Produces: the same public signature, with a final `npm prefix --global` fallback that returns `<prefix>/bin/acc` only when it is executable and is not an atcoder-local wrapper

- [ ] **Step 1: Write the failing npm-prefix discovery test**

Add this test to `tests/test_acc_locator.py`:

```python
def test_finds_npm_global_acc_when_global_bin_is_not_on_path(
    tmp_path: Path,
) -> None:
    active = installed_wrapper(tmp_path / "uv-tool/bin/acc")
    prefix = tmp_path / "npm-global"
    upstream = executable(prefix / "bin/acc")
    search_bin = tmp_path / "search-bin"
    search_bin.mkdir()
    npm = search_bin / "npm"
    npm.write_text(
        "#!/bin/sh\n"
        "test \"$1 $2\" = \"prefix --global\" || exit 9\n"
        f"printf '%s\\n' '{prefix}'\n",
        encoding="utf-8",
    )
    npm.chmod(0o755)

    assert find_upstream_acc(
        {"PATH": str(search_bin)}, active
    ) == str(upstream.resolve())
```

- [ ] **Step 2: Run the focused test and verify RED**

Run:

```sh
uv run pytest tests/test_acc_locator.py::test_finds_npm_global_acc_when_global_bin_is_not_on_path -v
```

Expected: FAIL because `find_upstream_acc` currently returns `None` after scanning only `PATH`.

- [ ] **Step 3: Implement the bounded npm-prefix fallback**

In `tools/atcoder_workflow/acc_locator.py`, import `shutil` and `subprocess`, call a new helper after the existing `PATH` loop, and implement it as follows:

```python
def _find_npm_global_acc(
    environ: Mapping[str, str], active: Path
) -> str | None:
    npm = shutil.which("npm", path=environ.get("PATH", ""))
    if npm is None:
        return None
    try:
        completed = subprocess.run(
            [npm, "prefix", "--global"],
            check=False,
            capture_output=True,
            text=True,
            env=dict(environ),
            timeout=5,
        )
    except (OSError, subprocess.TimeoutExpired):
        return None
    prefix = completed.stdout.strip()
    if completed.returncode != 0 or not prefix or "\n" in prefix:
        return None
    prefix_path = Path(prefix).expanduser()
    if not prefix_path.is_absolute():
        return None
    candidate = (prefix_path / "bin" / "acc").resolve()
    if _is_upstream_acc(candidate, active):
        return str(candidate)
    return None
```

Replace the final `return None` in `find_upstream_acc` with:

```python
    return _find_npm_global_acc(environ, active)
```

- [ ] **Step 4: Add discovery-failure coverage**

Add a parameterized test proving non-zero, empty, and relative npm-prefix output all remain `None`:

```python
@pytest.mark.parametrize(
    ("output", "exit_code"),
    [("", 0), ("relative-prefix", 0), ("/unused", 9)],
)
def test_ignores_invalid_npm_global_prefix(
    tmp_path: Path, output: str, exit_code: int
) -> None:
    active = installed_wrapper(tmp_path / "uv-tool/bin/acc")
    search_bin = tmp_path / "search-bin"
    search_bin.mkdir()
    npm = search_bin / "npm"
    npm.write_text(
        "#!/bin/sh\n"
        f"printf '%s\\n' '{output}'\n"
        f"exit {exit_code}\n",
        encoding="utf-8",
    )
    npm.chmod(0o755)

    assert find_upstream_acc({"PATH": str(search_bin)}, active) is None
```

Also add `import pytest` to `tests/test_acc_locator.py`.

- [ ] **Step 5: Run all locator tests and verify GREEN**

Run:

```sh
uv run pytest tests/test_acc_locator.py -v
```

Expected: all locator tests PASS, including the existing override and wrapper-recursion cases.

- [ ] **Step 6: Commit the locator fix**

```sh
git add tools/atcoder_workflow/acc_locator.py tests/test_acc_locator.py
git commit -m "fix: discover npm global atcoder cli"
```

---

### Task 2: Expose sibling uv tool executables during dispatch

**Files:**
- Modify: `tools/acc_wrapper.py`
- Test: `tests/test_acc_wrapper.py`

**Interfaces:**
- Consumes: `console_main() -> NoReturn`, `os.environ`, and `sys.executable`
- Produces: `_prepend_runtime_bin_to_path(environ: MutableMapping[str, str], executable: str) -> None`; `console_main` invokes it before `main()`

- [ ] **Step 1: Write the failing console PATH test**

Add `MutableMapping` to the `collections.abc` imports in the implementation plan's production file only. Add this test to `tests/test_acc_wrapper.py`:

```python
def test_console_main_prepends_runtime_bin_to_path(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    runtime_python = Path("/isolated/tool/bin/python")
    observed_path: str | None = None

    def wrapped_main() -> int:
        nonlocal observed_path
        observed_path = os.environ.get("PATH")
        return 0

    monkeypatch.setenv("PATH", os.pathsep.join(["/usr/bin", "/bin"]))
    monkeypatch.setattr(sys, "executable", str(runtime_python))
    with (
        patch("tools.acc_wrapper.main", side_effect=wrapped_main),
        pytest.raises(SystemExit) as raised,
    ):
        console_main()

    assert raised.value.code == 0
    assert observed_path == os.pathsep.join(
        [str(runtime_python.parent), "/usr/bin", "/bin"]
    )
```

- [ ] **Step 2: Run the focused test and verify RED**

Run:

```sh
uv run pytest tests/test_acc_wrapper.py::test_console_main_prepends_runtime_bin_to_path -v
```

Expected: FAIL because `console_main` currently sets only `ATCODER_LOCAL_CONSOLE`.

- [ ] **Step 3: Implement PATH preparation at the console boundary**

In `tools/acc_wrapper.py`, import `MutableMapping` beside `Callable` and add:

```python
def _prepend_runtime_bin_to_path(
    environ: MutableMapping[str, str], executable: str
) -> None:
    runtime_bin = str(Path(executable).parent)
    existing = [
        entry
        for entry in environ.get("PATH", "").split(os.pathsep)
        if entry and entry != runtime_bin
    ]
    environ["PATH"] = os.pathsep.join([runtime_bin, *existing])
```

Update `console_main`:

```python
def console_main() -> NoReturn:
    os.environ["ATCODER_LOCAL_CONSOLE"] = "1"
    _prepend_runtime_bin_to_path(os.environ, sys.executable)
    raise SystemExit(main())
```

- [ ] **Step 4: Add duplicate-entry coverage**

Add this test to `tests/test_acc_wrapper.py`:

```python
def test_console_main_moves_existing_runtime_bin_to_front_once(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    runtime_python = Path("/isolated/tool/bin/python")
    observed_path: str | None = None

    def wrapped_main() -> int:
        nonlocal observed_path
        observed_path = os.environ.get("PATH")
        return 0

    monkeypatch.setenv(
        "PATH",
        os.pathsep.join(["/usr/bin", str(runtime_python.parent), "/bin"]),
    )
    monkeypatch.setattr(sys, "executable", str(runtime_python))
    with (
        patch("tools.acc_wrapper.main", side_effect=wrapped_main),
        pytest.raises(SystemExit),
    ):
        console_main()

    assert observed_path == os.pathsep.join(
        [str(runtime_python.parent), "/usr/bin", "/bin"]
    )
```

- [ ] **Step 5: Run wrapper and workflow command tests and verify GREEN**

Run:

```sh
uv run pytest tests/test_acc_wrapper.py tests/test_workflow_commands.py -v
```

Expected: all tests PASS. Existing native delegation remains unchanged, while subprocesses inherit the updated `PATH`.

- [ ] **Step 6: Commit the console environment fix**

```sh
git add tools/acc_wrapper.py tests/test_acc_wrapper.py
git commit -m "fix: expose uv tool executables to acc"
```

---

### Task 3: Document a source-independent installation

**Files:**
- Modify: `README.md`
- Test: `tests/test_workflow_commands.py`

**Interfaces:**
- Consumes: uv's local-project tool installation syntax
- Produces: setup command `uv tool install .` and refresh command `uv tool install --force .`; no normal-workflow recommendation containing `--editable`

- [ ] **Step 1: Change the README assertion first**

Replace the editable-install assertions in `test_readme_documents_installed_acc_and_task_local_submission` with:

```python
    assert "uv tool install ." in readme
    assert "uv tool install --force ." in readme
    assert "uv tool install --editable ." not in readme
    assert "uv tool install --force --editable ." not in readme
```

- [ ] **Step 2: Run the focused README test and verify RED**

Run:

```sh
uv run pytest tests/test_workflow_commands.py::test_readme_documents_installed_acc_and_task_local_submission -v
```

Expected: FAIL because README still recommends editable installation.

- [ ] **Step 3: Update setup, refresh, and compatibility wording**

In `README.md`:

- replace `uv tool install --editable .` with `uv tool install .` in setup;
- replace `uv tool install --force --editable .` with `uv tool install --force .` in refresh instructions;
- change the explanation to say the installed command does not depend on keeping the checkout at the installation-time absolute path;
- replace the compatibility section's normal-install recommendation with `uv tool install .`;
- add a troubleshooting bullet instructing users who previously installed editable to run `uv tool install --force .` if the old checkout moved or disappeared.

- [ ] **Step 4: Run README tests and verify GREEN**

Run:

```sh
uv run pytest tests/test_workflow_commands.py -v
```

Expected: all workflow command and README contract tests PASS.

- [ ] **Step 5: Commit the installation documentation fix**

```sh
git add README.md tests/test_workflow_commands.py
git commit -m "docs: use source-independent acc install"
```

---

### Task 4: Verify the complete fix and publish it

**Files:**
- Verify only: all tracked files

**Interfaces:**
- Consumes: the three completed tasks and the repository's pinned dependencies
- Produces: passing host-independent tests, a clean package install outside the checkout, and pushed `main`

- [ ] **Step 1: Run formatting and whitespace checks**

Run:

```sh
git diff --check HEAD~3
```

Expected: no output and exit code 0.

- [ ] **Step 2: Run the complete host-independent suite**

Run:

```sh
uv run pytest -q
```

Expected: all tests PASS; environment-specific C++ integration tests may report their existing explicit skips.

- [ ] **Step 3: Build a non-editable wheel**

Run with temporary uv cache/output directories if the managed sandbox requires them:

```sh
uv build --wheel
```

Expected: `dist/atcoder_local-0.1.0-py3-none-any.whl` is created successfully and contains the `tools` package and `acc` console entry point.

- [ ] **Step 4: Verify the wheel outside the source checkout**

Create a temporary virtual environment, install the wheel without resolving dependencies, add a sibling `oj` executable, then run the installed console command from `/tmp` with npm's global bin absent from `PATH`:

```sh
wheel_check_dir=$(mktemp -d /tmp/atcoder-local-wheel-check.XXXXXX)
python3 -m venv "$wheel_check_dir/venv"
"$wheel_check_dir/venv/bin/pip" install --no-deps --force-reinstall dist/atcoder_local-0.1.0-py3-none-any.whl
ln -s /usr/bin/printf "$wheel_check_dir/venv/bin/oj"
cd /tmp
PATH="$wheel_check_dir/venv/bin:/opt/homebrew/bin:/usr/bin:/bin" "$wheel_check_dir/venv/bin/acc" check-oj
```

The diagnosed machine has native `atcoder-cli` under npm's global prefix, while the command above deliberately omits that prefix's `bin` directory. Expected results:

- the installed console imports without access to the checkout;
- npm-prefix fallback delegates to the real pinned native `acc`;
- native `atcoder-cli` reports online-judge-tools available at the temporary virtual environment's sibling `oj`, proving that the installed console prepended its own `bin` directory.

Record the generated temporary path in the verification notes. Do not use a broad or unresolved destructive cleanup command.

- [ ] **Step 5: Check repository state and recent commits**

Run:

```sh
git status -sb
git log -4 --oneline
```

Expected: `main` is ahead of `origin/main` only by the reviewed design and implementation commits; no untracked build artifacts remain. If `dist/` is untracked, remove only the wheel generated in Step 3 after verifying its exact path.

- [ ] **Step 6: Push the reviewed commits**

Run:

```sh
git push origin main
```

Expected: `origin/main` advances to the final implementation commit.
