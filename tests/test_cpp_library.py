from __future__ import annotations

import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import textwrap
import unittest


REPOSITORY_ROOT = Path(__file__).resolve().parents[1]


def test_cpp_template_quotes_repository_local_headers() -> None:
    template = (REPOSITORY_ROOT / "template/cpp/main.cpp").read_text(
        encoding="utf-8"
    )

    assert '#include "atcoder_local/core.hpp"' in template
    assert '#include "atcoder_local/debug.hpp"' in template
    assert '#include "atcoder_local/io.hpp"' in template
    assert "#include <atcoder_local/" not in template


class TestCppLibrary(unittest.TestCase):
    def compiler(self) -> str:
        compiler = shutil.which("g++") or shutil.which("clang++")
        if compiler is None:
            self.skipTest("no C++ compiler is installed")
        return compiler

    def compile_and_run(self, source: str) -> str:
        with tempfile.TemporaryDirectory() as temporary_directory:
            directory = Path(temporary_directory)
            source_path = directory / "main.cpp"
            executable_path = directory / "main"
            source_path.write_text(textwrap.dedent(source), encoding="utf-8")
            subprocess.run(
                [
                    self.compiler(),
                    "-std=gnu++23",
                    "-I",
                    str(REPOSITORY_ROOT / "library"),
                    str(source_path),
                    "-o",
                    str(executable_path),
                ],
                check=True,
                capture_output=True,
                text=True,
            )
            completed = subprocess.run(
                [str(executable_path)],
                check=True,
                capture_output=True,
                text=True,
            )
            return completed.stdout

    def gnu_compiler(self) -> str:
        for name in ("g++-15", "g++15", "g++"):
            compiler = shutil.which(name)
            if compiler is None:
                continue
            version = subprocess.run(
                [compiler, "--version"],
                check=True,
                capture_output=True,
                text=True,
            ).stdout
            if "clang" not in version.casefold():
                return compiler
        self.skipTest("oj-bundle requires a GNU C++ compiler")

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

    def test_bundled_submission_inlines_repository_local_headers(self) -> None:
        bundler = shutil.which("oj-bundle")
        if bundler is None:
            self.skipTest("oj-bundle is not installed")
        compiler = self.gnu_compiler()

        with tempfile.TemporaryDirectory() as temporary_directory:
            directory = Path(temporary_directory)
            source_path = directory / "main.cpp"
            submission_path = directory / "submission.cpp"
            executable_path = directory / "submission-main"
            source_path.write_text(
                textwrap.dedent(
                    """
                    #include <atcoder/dsu>
                    #include "atcoder_local/core.hpp"
                    #include <iostream>
                    int main() {
                        atcoder::dsu graph(2);
                        graph.merge(0, 1);
                        ll value = 3;
                        chmax(value, 5LL);
                        std::cout << graph.same(0, 1) << ' ' << value << '\\n';
                    }
                    """
                ),
                encoding="utf-8",
            )
            bundled = subprocess.run(
                [
                    bundler,
                    "-I",
                    str(REPOSITORY_ROOT / "library"),
                    str(source_path),
                ],
                check=False,
                capture_output=True,
                text=True,
                env={**os.environ, "CXX": compiler},
            )
            self.assertEqual(bundled.returncode, 0, bundled.stderr)
            self.assertNotIn("#include <atcoder_local/", bundled.stdout)
            self.assertNotIn('#include "atcoder_local/', bundled.stdout)
            submission_path.write_text(bundled.stdout, encoding="utf-8")
            subprocess.run(
                [
                    compiler,
                    "-std=gnu++23",
                    "-I",
                    str(REPOSITORY_ROOT / "library"),
                    str(submission_path),
                    "-o",
                    str(executable_path),
                ],
                check=True,
                capture_output=True,
                text=True,
            )
            completed = subprocess.run(
                [str(executable_path)],
                check=True,
                capture_output=True,
                text=True,
            )

        self.assertEqual(completed.stdout, "1 5\n")
