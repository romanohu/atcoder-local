import json
import tempfile
import unittest
from pathlib import Path

from tools.acc_wrapper import (
    build_memo_content,
    create_memo_if_missing,
    extract_contest_dir_format,
    extract_contest_id,
    extract_task_headings,
)


class TestAccWrapper(unittest.TestCase):
    def test_extract_contest_id_simple(self) -> None:
        self.assertEqual(extract_contest_id(["new", "abc454"]), "abc454")

    def test_extract_contest_id_with_option_values(self) -> None:
        args = ["new", "-c", "all", "--template", "cpp", "abc454"]
        self.assertEqual(extract_contest_id(args), "abc454")

    def test_extract_contest_dir_format_short_option(self) -> None:
        args = ["new", "-d", "archive/{ContestID}", "abc454"]
        self.assertEqual(extract_contest_dir_format(args), "archive/{ContestID}")

    def test_extract_contest_dir_format_long_option_with_equal(self) -> None:
        args = ["new", "--contest-dirname-format=archive/{ContestID}", "abc454"]
        self.assertEqual(extract_contest_dir_format(args), "archive/{ContestID}")

    def test_extract_task_headings_prefers_directory_path(self) -> None:
        config = {
            "tasks": [
                {"directory": {"path": "a"}, "label": "A"},
                {"directory": {"path": "b"}, "label": "B"},
            ]
        }
        self.assertEqual(extract_task_headings(config), ["a", "b"])

    def test_extract_task_headings_falls_back_to_label(self) -> None:
        config = {"tasks": [{"label": "PracticeA"}, {"label": "ABC086A"}]}
        self.assertEqual(extract_task_headings(config), ["practicea", "abc086a"])

    def test_build_memo_content(self) -> None:
        expected = "# abc454\n\n## a\n\n## b\n"
        self.assertEqual(build_memo_content("abc454", ["a", "b"]), expected)

    def test_create_memo_if_missing_writes_file(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            contest_dir = Path(tmpdir)
            created = create_memo_if_missing(contest_dir, "abc454", ["a", "b"])
            self.assertTrue(created)
            self.assertEqual(
                (contest_dir / "memo.md").read_text(encoding="utf-8"),
                "# abc454\n\n## a\n\n## b\n",
            )

    def test_create_memo_if_missing_does_not_overwrite(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            contest_dir = Path(tmpdir)
            memo_path = contest_dir / "memo.md"
            memo_path.write_text("# keep\n", encoding="utf-8")

            created = create_memo_if_missing(contest_dir, "abc454", ["a", "b"])
            self.assertFalse(created)
            self.assertEqual(memo_path.read_text(encoding="utf-8"), "# keep\n")

    def test_extract_task_headings_from_realistic_json(self) -> None:
        config_text = """
        {
          "contest": { "id": "abc454" },
          "tasks": [
            { "label": "A", "directory": { "path": "a" } },
            { "label": "B", "directory": { "path": "b" } }
          ]
        }
        """
        config = json.loads(config_text)
        self.assertEqual(extract_task_headings(config), ["a", "b"])


if __name__ == "__main__":
    unittest.main()
