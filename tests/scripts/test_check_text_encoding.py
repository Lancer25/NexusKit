import tempfile
import unittest
from pathlib import Path

from scripts.check_text_encoding import check_text_policy, scan_tree


class TextEncodingChecksTest(unittest.TestCase):
    def test_accepts_utf8_text(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            path = root / "docs" / "guide.md"
            path.parent.mkdir()
            path.write_bytes(b"# Guide\nUTF-8 text: \xe4\xb8\xad\xe6\x96\x87\n")

            self.assertEqual([], scan_tree(root))

    def test_rejects_invalid_utf8_text(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            path = root / "include" / "broken.h"
            path.parent.mkdir()
            path.write_bytes(b"#pragma once\n\xff\n")

            failures = scan_tree(root)

        self.assertEqual(1, len(failures))
        self.assertEqual(Path("include") / "broken.h", failures[0].path)
        self.assertIn("invalid utf-8", failures[0].reason)

    def test_ignores_binary_extensions(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            path = root / "image.png"
            path.write_bytes(b"\x89PNG\r\n\x1a\n\xff")

            self.assertEqual([], scan_tree(root))

    def test_requires_editorconfig_text_policy(self):
        with tempfile.TemporaryDirectory() as tmp:
            failures = check_text_policy(Path(tmp))

        self.assertEqual(1, len(failures))
        self.assertEqual(Path(".editorconfig"), failures[0].path)
        self.assertIn("missing", failures[0].reason)

    def test_accepts_editorconfig_text_policy(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / ".editorconfig").write_text(
                "root = true\n"
                "\n"
                "[*]\n"
                "charset = utf-8\n"
                "end_of_line = lf\n"
                "insert_final_newline = true\n"
                "trim_trailing_whitespace = true\n"
                "\n"
                "[*.{bat,cmd,ps1}]\n"
                "end_of_line = crlf\n",
                encoding="utf-8",
            )

            self.assertEqual([], check_text_policy(root))

    def test_rejects_incomplete_editorconfig_text_policy(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / ".editorconfig").write_text(
                "root = true\n"
                "\n"
                "[*]\n"
                "charset = utf-8\n"
                "end_of_line = lf\n"
                "insert_final_newline = true\n"
                "\n"
                "[*.{bat,cmd,ps1}]\n"
                "end_of_line = crlf\n",
                encoding="utf-8",
            )

            failures = check_text_policy(root)

        self.assertEqual(1, len(failures))
        self.assertEqual(Path(".editorconfig"), failures[0].path)
        self.assertIn("[*].trim_trailing_whitespace", failures[0].reason)


if __name__ == "__main__":
    unittest.main()
