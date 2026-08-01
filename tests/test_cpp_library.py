from __future__ import annotations

from pathlib import Path
import shutil
import subprocess
import tempfile
import textwrap
import unittest


REPOSITORY_ROOT = Path(__file__).resolve().parents[1]


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
