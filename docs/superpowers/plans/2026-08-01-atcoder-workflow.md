# AtCoder Local Workflow Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a safe, reproducible C++ workflow behind the existing `acc` wrapper for build, run, sample test, bundled submission, and environment diagnosis.

**Architecture:** Keep atcoder-cli as the external contest/submission adapter and route only five local commands into a focused Python package. Resolve task metadata once, pass all processes through an injectable runner, and atomically write generated files below `.atcoder-local/`. Vendor ACL v1.6 and bundle repository headers before submission.

**Tech Stack:** Python 3.10+, unittest/pytest, uv, atcoder-cli 2.2.0, online-judge-tools, online-judge-verify-helper 5.6.0, C++23, ACL v1.6, bash, zsh

## Global Constraints

- Preserve existing `acc new` memo creation and push-lock registration.
- Preserve stdio and exit codes for commands delegated to atcoder-cli.
- Support macOS/Linux with bash and zsh; Windows is out of scope.
- Implement only C++, while keeping task resolution independent of the language backend.
- Root invocation requires both `-c/--contest` and `-t/--task`.
- Compiler order is `CXX`, verified GCC 15, available `g++`, then available `clang++`.
- Treat `CXX` as one executable path or command name; do not split it into shell words.
- Release flags are `-std=gnu++23 -O2 -Wall -Wextra -DONLINE_JUDGE -DATCODER`.
- Debug flags are `-std=gnu++23 -O0 -g -Wall -Wextra -DATCODER -DLOCAL -fsanitize=address,undefined -fno-omit-frame-pointer`.
- Vendor ACL tag `v1.6`, commit `864245a00b00dd008d1abfdc239618fdb7d139da`, with its license.
- Stop submission on bundle, compile, sample-test, TTY, or confirmation failure.
- Do not add confirmation bypass, browser fallback, URL fallback, or clipboard operations.
- Never execute stale build or bundle output after a failed replacement.
- Invoke external commands with argument arrays, never interpolated shell commands.

---

## File Map

- Create `tools/atcoder_workflow/__init__.py`: shared `WorkflowError`.
- Create `tools/atcoder_workflow/runner.py`: injectable subprocess boundary.
- Create `tools/atcoder_workflow/context.py`: repository/task/source resolution.
- Create `tools/atcoder_workflow/compiler.py`: compiler inspection, probe, selection, and flags.
- Create `tools/atcoder_workflow/cpp.py`: atomic compile/bundle and execution helpers.
- Create `tools/atcoder_workflow/commands.py`: command orchestration and doctor checks.
- Create `tools/atcoder_workflow/cli.py`: argparse and error-to-exit-code conversion.
- Modify `tools/acc_wrapper.py`: route the five custom commands.
- Create `tools/acc-wrapper.bash`; modify `tools/acc-wrapper.zsh`: shell entrypoints.
- Create `library/atcoder/**`: ACL headers, license, and version record.
- Create `library/atcoder_local/{core,io,debug}.hpp`: minimal local helpers.
- Modify `template/cpp/main.cpp`, `pyproject.toml`, `uv.lock`, `.gitignore`, and `README.md`.
- Create `tests/test_cpp_library.py`, `tests/test_workflow_context.py`, `tests/test_workflow_compiler.py`, `tests/test_workflow_cpp.py`, `tests/test_workflow_commands.py`, and `tests/test_workflow_doctor.py`.

---

### Task 1: Reproducible dependencies and C++ headers

**Files:**
- Modify: `pyproject.toml`, `uv.lock`, `.gitignore`, `template/cpp/main.cpp`
- Create: `library/atcoder/**`, `library/atcoder_local/core.hpp`, `library/atcoder_local/io.hpp`, `library/atcoder_local/debug.hpp`
- Test: `tests/test_cpp_library.py`

**Interfaces:**
- Consumes: current C++ template and uv project.
- Produces: include root `<repo>/library`; `oj-bundle`; minimal local headers; vendored ACL.

- [ ] **Step 1: Write the failing C++ smoke tests**

Create a real-compiler helper and these tests:

```python
class TestCppLibrary(unittest.TestCase):
    def compiler(self) -> str:
        compiler = shutil.which("g++") or shutil.which("clang++")
        if compiler is None:
            self.skipTest("no C++ compiler is installed")
        return compiler

    def test_local_headers_and_acl_compile_together(self) -> None:
        source = """
#include <atcoder/dsu>
#include <atcoder_local/core.hpp>
#include <atcoder_local/io.hpp>
int main() {
    atcoder_local::setup_io();
    atcoder::dsu graph(2);
    graph.merge(0, 1);
    ll value = 3;
    chmax(value, 5LL);
    atcoder_local::print(graph.same(0, 1), value);
}
"""
        self.assertEqual(self.compile_and_run(source), "1 5\n")

    def test_debug_macro_is_compiled_out_without_local(self) -> None:
        source = """
#include <atcoder_local/debug.hpp>
int main() { DBG(42); }
"""
        self.assertEqual(self.compile_and_run(source), "")
```

`compile_and_run` must write a temporary source, compile with `-std=gnu++23 -I <repo>/library`, execute it, and return stdout.

- [ ] **Step 2: Verify the red state**

Run: `uv run python -m unittest tests.test_cpp_library -v`

Expected: FAIL because the headers do not exist.

- [ ] **Step 3: Add Python dependencies and vendor ACL**

Set:

```toml
dependencies = [
    "aclogin>=0.2.1",
    "online-judge-tools>=11.5.1",
    "online-judge-verify-helper==5.6.0",
]

[dependency-groups]
dev = ["pytest>=9.1.1"]
```

Vendor the immutable ACL commit:

```bash
acl_tmp=$(mktemp -d)
curl -L https://api.github.com/repos/atcoder/ac-library/tarball/864245a00b00dd008d1abfdc239618fdb7d139da -o "$acl_tmp/acl.tar.gz"
mkdir "$acl_tmp/src"
tar -xzf "$acl_tmp/acl.tar.gz" -C "$acl_tmp/src" --strip-components=1
mkdir -p library/atcoder
cp -R "$acl_tmp/src/atcoder/." library/atcoder/
cp "$acl_tmp/src/LICENSE" library/atcoder/LICENSE
```

Create `library/atcoder/VERSION` containing `v1.6` and the full commit SHA on separate lines. Run `uv lock`.

- [ ] **Step 4: Implement local headers and template**

Provide these exact public symbols:

```cpp
using ll = long long;
using ull = unsigned long long;
#define rep(i, n) for (int i = 0; i < static_cast<int>(n); ++i)
template <class T, class U> bool chmin(T& current, const U& candidate);
template <class T, class U> bool chmax(T& current, const U& candidate);

namespace atcoder_local {
inline void setup_io();
template <class... Ts> void read(Ts&... values);
inline void print();
template <class T, class... Ts> void print(const T& first, const Ts&... rest);
}
```

`debug.hpp` defines `DBG` to call `atcoder_local::debug_log` only under `LOCAL`, otherwise `((void)0)`. Use include guards. Update the template to include all three local headers and call `setup_io()`. Ignore `.atcoder-local/`.

- [ ] **Step 5: Verify and commit**

Run: `uv sync --group dev`

Run: `uv run pytest tests/test_cpp_library.py -v`

Expected: all tests PASS.

```bash
git add pyproject.toml uv.lock .gitignore template/cpp/main.cpp library tests/test_cpp_library.py
git commit -m "feat: add reproducible C++ library toolchain"
```

---

### Task 2: Task context resolution

**Files:**
- Create: `tools/atcoder_workflow/__init__.py`, `tools/atcoder_workflow/context.py`
- Test: `tests/test_workflow_context.py`

**Interfaces:**
- Produces `WorkflowError`.
- Produces immutable `TaskContext(repository_root, contest_id, task_id, task_label, contest_dir, task_dir, source_path, test_dir, build_dir)`.
- Produces `resolve_task_context(cwd, contest_id=None, task_label=None) -> TaskContext`.

- [ ] **Step 1: Write failing resolver tests**

Use this metadata shape:

```python
config = {
    "contest": {"id": "abc999", "title": "Contest", "url": "https://atcoder.jp/contests/abc999"},
    "tasks": [{
        "id": "abc999_a",
        "label": "A",
        "title": "Task A",
        "url": "https://atcoder.jp/contests/abc999/tasks/abc999_a",
        "directory": {"path": "a", "submit": "main.cpp", "testdir": "test"},
    }],
}
```

Add tests for task-directory discovery, root `-c abc999 -t a`, one missing flag, duplicate contest configs, a submit path escaping the task directory, and a current directory that is not a configured task.

- [ ] **Step 2: Verify the red state**

Run: `uv run pytest tests/test_workflow_context.py -v`

Expected: collection FAILS with `ModuleNotFoundError`.

- [ ] **Step 3: Implement the model and resolver**

```python
@dataclass(frozen=True)
class TaskContext:
    repository_root: Path
    contest_id: str
    task_id: str
    task_label: str
    contest_dir: Path
    task_dir: Path
    source_path: Path
    test_dir: Path
    build_dir: Path
```

Implementation rules:

1. Find the nearest parent containing `.git`.
2. Reject exactly one of contest/task flags.
3. Skip `.git`, `.venv`, `.worktrees`, and `.atcoder-local` when searching configs.
4. With flags, require exactly one JSON `contest.id`, then one case-insensitive task label.
5. Without flags, use the nearest parent config and require the current path to match exactly one task.
6. Resolve `directory.path`, `directory.submit`, and `directory.testdir`; reject paths outside their owners.
7. Require `.cpp`, an existing source, and an existing test directory.
8. Build under `<root>/.atcoder-local/build/<contest>/<full-task-id>`.

- [ ] **Step 4: Verify and commit**

Run: `uv run pytest tests/test_workflow_context.py tests/test_acc_wrapper.py -q`

Expected: new context tests and existing wrapper tests PASS.

```bash
git add tools/atcoder_workflow tests/test_workflow_context.py
git commit -m "feat: resolve AtCoder task context"
```

---

### Task 3: Process runner and compiler selection

**Files:**
- Create: `tools/atcoder_workflow/runner.py`, `tools/atcoder_workflow/compiler.py`
- Test: `tests/test_workflow_compiler.py`

**Interfaces:**
- Produces callable protocol `ProcessRunner` and implementation `run_process`.
- Produces `CompilerFamily`, `BuildMode`, and immutable `CompilerInfo(executable, family, major, version_text)`.
- Produces `detect_compiler(environ, runner, which) -> CompilerInfo` and `compiler_flags(mode, library_dir) -> list[str]`.

- [ ] **Step 1: Write failing compiler tests**

```python
def test_explicit_cxx_wins_over_gcc15() -> None:
    info = detect_compiler(
        environ={"CXX": "/custom/clang++"},
        runner=fake_compiler_runner({"/custom/clang++": "Apple clang version 17.0.0"}),
        which=fake_which({"g++-15": "/opt/g++-15"}),
    )
    assert info.executable == "/custom/clang++"
    assert info.family is CompilerFamily.CLANG

def test_verified_gcc15_wins_over_apple_gpp() -> None:
    versions = {
        "/opt/g++-15": "g++ (GCC) 15.2.0",
        "/usr/bin/g++": "Apple clang version 17.0.0",
    }
    info = detect_compiler(
        environ={},
        runner=fake_compiler_runner(versions),
        which=fake_which({"g++-15": "/opt/g++-15", "g++": "/usr/bin/g++"}),
    )
    assert (info.executable, info.major) == ("/opt/g++-15", 15)
```

Also test broken explicit `CXX`, a failed C++23 probe, missing compiler, exact release flags, and exact debug flags without `ONLINE_JUDGE`.

- [ ] **Step 2: Verify the red state**

Run: `uv run pytest tests/test_workflow_compiler.py -v`

Expected: collection FAILS because compiler modules do not exist.

- [ ] **Step 3: Implement runner and compiler**

`run_process` wraps `subprocess.run(list(argv), cwd=cwd, env=env, capture_output=capture_output, text=True, check=False)`. Its Protocol exposes the same keyword parameters.

Define:

```python
class CompilerFamily(Enum):
    GCC = "gcc"
    CLANG = "clang"

class BuildMode(Enum):
    RELEASE = "release"
    DEBUG = "debug"

@dataclass(frozen=True)
class CompilerInfo:
    executable: str
    family: CompilerFamily
    major: int
    version_text: str
```

Inspect `--version`, parse GCC and Clang forms, and probe `int main() { return 0; }` with `-std=gnu++23` in a temporary directory. Probe the auto-candidate names `g++-15`, `g++15`, `g++`, and `clang++`, removing duplicate resolved paths. A bad explicit `CXX` is an error; a bad auto-candidate is skipped. Return flags exactly as stated in Global Constraints plus `-I <library_dir>`.

- [ ] **Step 4: Verify and commit**

Run: `uv run pytest tests/test_workflow_compiler.py -v`

Expected: all compiler tests PASS.

```bash
git add tools/atcoder_workflow/runner.py tools/atcoder_workflow/compiler.py tests/test_workflow_compiler.py
git commit -m "feat: detect and configure C++ compilers"
```

---

### Task 4: Atomic C++ primitives

**Files:**
- Create: `tools/atcoder_workflow/cpp.py`
- Test: `tests/test_workflow_cpp.py`

**Interfaces:**
- Produces `compile_cpp(source_path, output_path, working_dir, compiler, mode, library_dir, runner) -> Path`.
- Produces `bundle_cpp(source_path, output_path, working_dir, library_dir, runner) -> Path`.
- Produces `run_binary(binary_path, working_dir, runner) -> int`.
- Produces `run_samples(binary_path, test_dir, working_dir, runner) -> int`.

- [ ] **Step 1: Write failing atomicity tests**

```python
def test_failed_compile_does_not_return_stale_output(tmp_path: Path) -> None:
    output = tmp_path / "main"
    output.write_text("old", encoding="utf-8")
    with pytest.raises(WorkflowError, match="compile failed"):
        compile_cpp(
            source_path=tmp_path / "main.cpp",
            output_path=output,
            working_dir=tmp_path,
            compiler=GCC15,
            mode=BuildMode.RELEASE,
            library_dir=tmp_path / "library",
            runner=compile_runner(returncode=1),
        )
    assert output.read_text(encoding="utf-8") == "old"
```

Also test successful atomic replacement, failed/successful bundle replacement, inherited-stdio binary execution, and exact `oj test` arguments.

- [ ] **Step 2: Verify the red state**

Run: `uv run pytest tests/test_workflow_cpp.py -v`

Expected: collection FAILS because `cpp.py` does not exist.

- [ ] **Step 3: Implement atomic operations**

```python
def compile_cpp(
    *,
    source_path: Path,
    output_path: Path,
    working_dir: Path,
    compiler: CompilerInfo,
    mode: BuildMode,
    library_dir: Path,
    runner: ProcessRunner,
) -> Path:
    output_path.parent.mkdir(parents=True, exist_ok=True)
    temporary = output_path.with_name(f".{output_path.name}.tmp")
    temporary.unlink(missing_ok=True)
    result = runner(
        [compiler.executable, *compiler_flags(mode, library_dir), str(source_path), "-o", str(temporary)],
        cwd=working_dir,
    )
    if result.returncode != 0:
        temporary.unlink(missing_ok=True)
        raise WorkflowError(f"compile failed for {source_path}")
    temporary.replace(output_path)
    return output_path
```

`bundle_cpp` runs `oj-bundle -I <library> <source>` with captured stdout, writes a UTF-8 temporary file, and replaces only on success. `run_binary` invokes the binary as one argument with inherited stdio. `run_samples` invokes `oj test -c <shlex-quoted-binary> -d <test-dir>`.

- [ ] **Step 4: Verify and commit**

Run: `uv run pytest tests/test_workflow_cpp.py -v`

Expected: all primitive tests PASS.

```bash
git add tools/atcoder_workflow/cpp.py tests/test_workflow_cpp.py
git commit -m "feat: add atomic C++ workflow primitives"
```

---

### Task 5: Build, run, test, and CLI

**Files:**
- Create: `tools/atcoder_workflow/commands.py`, `tools/atcoder_workflow/cli.py`
- Test: `tests/test_workflow_commands.py`

**Interfaces:**
- Produces `WorkflowDependencies(runner, environ, which, input_fn, stdin_isatty)`.
- Produces `run_build(context, dependencies, mode=RELEASE)`, `run_program(context, dependencies)`, `run_tests(context, dependencies, debug=False)`, and `main(argv, cwd, dependencies)`.

- [ ] **Step 1: Write failing orchestration tests**

```python
def test_run_tests_builds_before_samples() -> None:
    events: list[str] = []
    with (
        patch("tools.atcoder_workflow.commands.detect_compiler", return_value=GCC15),
        patch("tools.atcoder_workflow.commands.compile_cpp", side_effect=lambda **kw: events.append("compile") or Path("/tmp/main")),
        patch("tools.atcoder_workflow.commands.run_samples", side_effect=lambda **kw: events.append("samples") or 0),
    ):
        assert run_tests(CONTEXT, DEPENDENCIES) == 0
    assert events == ["compile", "samples"]
```

Also test release output, run-after-build, debug mode selection, sample failure propagation, root flag parsing, rejection of `--debug` outside `test`, and stderr/exit 1 for `WorkflowError`.

- [ ] **Step 2: Verify the red state**

Run: `uv run pytest tests/test_workflow_commands.py -v`

Expected: collection FAILS because command modules do not exist.

- [ ] **Step 3: Implement command dependencies and orchestration**

```python
@dataclass(frozen=True)
class WorkflowDependencies:
    runner: ProcessRunner
    environ: Mapping[str, str]
    which: Callable[[str], str | None]
    input_fn: Callable[[str], str]
    stdin_isatty: Callable[[], bool]
```

`run_build` detects the compiler and writes `main` or `main-debug`. `run_program` release-builds then runs. `run_tests` chooses the build mode, compiles, then calls sample tests. Later stages are never called after an exception.

- [ ] **Step 4: Implement argparse**

Create subparsers `build`, `run`, `test`, `submit`, and `doctor`. Task commands accept `-c/-t`; only `test` accepts `--debug`. For task commands, resolve context once and dispatch. For `doctor`, find only the repository root and do not require a contest or task. Catch `WorkflowError`, print `[acc-wrapper] <message>` to stderr, and return 1.

- [ ] **Step 5: Verify and commit**

Run: `uv run pytest tests/test_workflow_commands.py tests/test_workflow_context.py tests/test_workflow_compiler.py tests/test_workflow_cpp.py -q`

Expected: all workflow tests PASS.

```bash
git add tools/atcoder_workflow/commands.py tools/atcoder_workflow/cli.py tests/test_workflow_commands.py
git commit -m "feat: add local build run and test commands"
```

---

### Task 6: Safe bundled submission

**Files:**
- Modify: `tools/atcoder_workflow/commands.py`, `tools/atcoder_workflow/cli.py`, `tests/test_workflow_commands.py`, `tests/test_cpp_library.py`

**Interfaces:**
- Produces `run_submit(context, dependencies) -> int`.
- Produces `submission.cpp` and `submission-main` under the task build directory.

- [ ] **Step 1: Write failing gate-order tests**

```python
def test_submit_runs_all_gates_in_order() -> None:
    events: list[str] = []
    dependencies = submit_dependencies(events, answer="yes", tty=True)
    with patched_submit_stages(events, sample_returncode=0):
        assert run_submit(CONTEXT, dependencies) == 0
    assert events == ["bundle", "compile", "samples", "prompt", "submit"]
```

Add parameterized bundle/compile/sample failures and assert raw `acc` is never called. Also test non-TTY, empty answer, accepted `y/yes`, raw failure propagation, and exact full task ID arguments.

- [ ] **Step 2: Verify the red state**

Run: `uv run pytest tests/test_workflow_commands.py -k submit -v`

Expected: FAIL because `run_submit` is absent.

- [ ] **Step 3: Implement the submission sequence**

Execute exactly:

1. Detect compiler.
2. Bundle source to `submission.cpp`.
3. Release-compile it to `submission-main`.
4. Run all samples with that binary.
5. Reject nonzero samples.
6. Reject non-TTY.
7. Print contest/task/path and prompt `Submit <task-id> from <path>? [y/N] `.
8. Accept only case-insensitive `y` or `yes`.
9. Resolve raw `acc` with `which`.
10. Run `[raw_acc, "submit", submission, "-c", contest_id, "-t", full_task_id]`.
11. Return the raw exit code without fallback.

- [ ] **Step 4: Add a real self-contained bundle smoke test**

Run `oj-bundle -I <library> <temporary-source>`, write stdout to `submission.cpp`, compile it without `-I library`, run it, and assert expected output. The temporary source must use both `<atcoder/dsu>` and `<atcoder_local/core.hpp>`.

- [ ] **Step 5: Verify and commit**

Run: `uv run pytest tests/test_workflow_commands.py -k submit -v`

Run: `uv run pytest tests/test_cpp_library.py -v`

Expected: all submission and bundle tests PASS.

```bash
git add tools/atcoder_workflow/commands.py tools/atcoder_workflow/cli.py tests/test_workflow_commands.py tests/test_cpp_library.py
git commit -m "feat: add verified bundled submission"
```

---

### Task 7: Doctor, shell wrappers, and dispatcher

**Files:**
- Modify: `tools/atcoder_workflow/commands.py`, `tools/atcoder_workflow/cli.py`, `tools/acc_wrapper.py`, `tools/acc-wrapper.zsh`, `tests/test_acc_wrapper.py`
- Create: `tools/acc-wrapper.bash`, `tests/test_workflow_doctor.py`

**Interfaces:**
- Produces `CheckStatus`, `DoctorCheck`, `collect_doctor_checks(root, dependencies)`, and `run_doctor(root, dependencies)`.
- Extends `acc_wrapper.main` with injectable `workflow_runner(args, cwd)`.

- [ ] **Step 1: Write failing doctor and routing tests**

```python
def test_doctor_warns_for_non_gcc15_compiler() -> None:
    checks = collect_doctor_checks(ROOT, healthy_dependencies(compiler=APPLE_CLANG17))
    check = next(item for item in checks if item.name == "compiler")
    assert check.status is CheckStatus.WARNING

def test_custom_command_bypasses_native_acc(self) -> None:
    native_calls: list[list[str]] = []
    workflow_calls: list[list[str]] = []
    result = acc_wrapper.main(
        ["test", "--debug"],
        cwd=self.root,
        acc_runner=lambda args: native_calls.append(args) or 0,
        workflow_runner=lambda args, cwd: workflow_calls.append(args) or 0,
    )
    self.assertEqual((result, native_calls, workflow_calls), (0, [], [["test", "--debug"]]))
```

Doctor tests cover missing `uv`, `acc`, `oj`, `oj-bundle`, bad ACL metadata, missing wrapper marker, uninstalled push guard, non-GCC warning, and fully healthy status. Routing tests also prove `config` delegates and `new` retains existing pre/post operations.

- [ ] **Step 2: Verify the red state**

Run: `uv run pytest tests/test_workflow_doctor.py tests/test_acc_wrapper.py -v`

Expected: FAIL because doctor and custom routing do not exist.

- [ ] **Step 3: Implement doctor**

```python
class CheckStatus(Enum):
    OK = "ok"
    WARNING = "warning"
    ERROR = "error"

@dataclass(frozen=True)
class DoctorCheck:
    name: str
    status: CheckStatus
    message: str
```

Collect named checks `uv`, `atcoder-cli`, `oj`, `oj-bundle`, `compiler`, `acl`, `push-guard`, and `shell-wrapper`. Missing requirements are errors; a working non-GCC-15 compiler is a warning. Print one line per check and return 1 when any ERROR exists.

- [ ] **Step 4: Route custom commands**

Define `CUSTOM_COMMANDS = {"build", "run", "test", "submit", "doctor"}`. Before existing contest extraction, send matching commands to the injected runner or `atcoder_workflow.cli.main(args, cwd=working_directory)`. Leave legacy code unchanged.

- [ ] **Step 5: Add bash and zsh markers**

```bash
_atcoder_local_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

acc() {
  ATCODER_LOCAL_WRAPPER=1 uv run python "${_atcoder_local_root}/tools/acc_wrapper.py" "$@"
}
```

Keep the existing zsh root expression and set the same marker on its Python command.

- [ ] **Step 6: Verify and commit**

Run: `uv run pytest tests/test_workflow_doctor.py tests/test_acc_wrapper.py -q`

Run: `bash -n tools/acc-wrapper.bash`

Run: `zsh -n tools/acc-wrapper.zsh`

Expected: all tests PASS and both syntax checks exit 0.

```bash
git add tools/atcoder_workflow tools/acc_wrapper.py tools/acc-wrapper.zsh tools/acc-wrapper.bash tests/test_workflow_doctor.py tests/test_acc_wrapper.py
git commit -m "feat: route workflow commands and diagnose setup"
```

---

### Task 8: README and full acceptance verification

**Files:**
- Modify: `README.md`, `tests/test_workflow_commands.py`

**Interfaces:**
- Produces a reproducible new-user setup and complete command reference.

- [ ] **Step 1: Write a failing README contract test**

```python
def test_readme_documents_complete_workflow() -> None:
    readme = (ROOT / "README.md").read_text(encoding="utf-8")
    required = [
        "uv sync --group dev",
        "tools/acc-wrapper.zsh",
        "tools/acc-wrapper.bash",
        "acc doctor",
        "acc build",
        "acc run",
        "acc test",
        "acc test --debug",
        "acc submit",
        "acc test -c abc123 -t a",
        "CAPTCHA",
        "Rated",
        "--no-verify",
    ]
    for fragment in required:
        assert fragment in readme
```

Define `ROOT = Path(__file__).parents[1]` in the test module so this assertion does not depend on the process working directory.

- [ ] **Step 2: Verify the red state**

Run: `uv run pytest tests/test_workflow_commands.py -k readme -v`

Expected: FAIL because README lacks the new commands.

- [ ] **Step 3: Rewrite README**

Use sections in this order: overview; requirements; `uv sync` and atcoder-cli installation; atcoder-cli config; bash/zsh wrapper activation; push-guard install; `acc doctor`; contest creation; build/run/test/debug/submit; root `-c/-t`; CAPTCHA limitations; push-guard recovery/bypass limits; troubleshooting.

State that submission is interactive-only, defaults to No, and performs no browser/URL/clipboard fallback.

- [ ] **Step 4: Run complete verification**

Run: `uv run pytest -q`

Expected: every existing push-guard test and every new workflow test PASS.

Run: `uv run pytest tests/test_cpp_library.py -v`

Expected: header, ACL, debug, bundle, and bundle-recompile smoke tests PASS.

Run: `bash -n tools/acc-wrapper.bash`

Run: `zsh -n tools/acc-wrapper.zsh`

Expected: both exit 0.

- [ ] **Step 5: Check state safety**

Run: `git status --short`

Expected: no `.atcoder-local/`, `a.out`, probe, or temporary bundle file is listed.

Run: `uv run python tools/push_guard.py status`

Expected: status completes without modifying registered locks.

- [ ] **Step 6: Commit docs and perform the final clean run**

```bash
git add README.md tests/test_workflow_commands.py
git commit -m "docs: explain local AtCoder workflow"
```

Run: `uv run pytest -q`

Expected: full suite PASS.

Run: `git status --short`

Expected: clean working tree.
