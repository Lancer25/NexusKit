import tempfile
import unittest
from pathlib import Path

from scripts.check_text_encoding import check_text_policy, scan_tree


def write_valid_editorconfig(root: Path) -> None:
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


def write_valid_gitattributes(root: Path) -> None:
    (root / ".gitattributes").write_text(
        "* text=auto eol=lf\n"
        "\n"
        "*.bat text eol=crlf\n"
        "*.cmd text eol=crlf\n"
        "*.ps1 text eol=crlf\n",
        encoding="utf-8",
    )


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

    def test_rejects_utf8_bom_text(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            path = root / "docs" / "bom.md"
            path.parent.mkdir()
            path.write_bytes(b"\xef\xbb\xbf# Guide\n")

            failures = scan_tree(root)

        self.assertEqual(1, len(failures))
        self.assertEqual(Path("docs") / "bom.md", failures[0].path)
        self.assertIn("utf-8 bom", failures[0].reason)

    def test_rejects_text_missing_final_newline(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            path = root / "docs" / "guide.md"
            path.parent.mkdir()
            path.write_text("# Guide\nNo final newline", encoding="utf-8")

            failures = scan_tree(root)

        self.assertEqual(1, len(failures))
        self.assertEqual(Path("docs") / "guide.md", failures[0].path)
        self.assertIn("missing final newline", failures[0].reason)

    def test_rejects_text_trailing_whitespace(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            path = root / "include" / "example.h"
            path.parent.mkdir()
            path.write_text("#pragma once\nint value; \n", encoding="utf-8")

            failures = scan_tree(root)

        self.assertEqual(1, len(failures))
        self.assertEqual(Path("include") / "example.h", failures[0].path)
        self.assertIn("line 2", failures[0].reason)
        self.assertIn("trailing whitespace", failures[0].reason)

    def test_rejects_editorconfig_trailing_whitespace(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / ".editorconfig").write_text("root = true \n", encoding="utf-8")

            failures = scan_tree(root)

        self.assertEqual(1, len(failures))
        self.assertEqual(Path(".editorconfig"), failures[0].path)
        self.assertIn("trailing whitespace", failures[0].reason)

    def test_rejects_gitattributes_missing_final_newline(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / ".gitattributes").write_text("* text=auto eol=lf", encoding="utf-8")

            failures = scan_tree(root)

        self.assertEqual(1, len(failures))
        self.assertEqual(Path(".gitattributes"), failures[0].path)
        self.assertIn("missing final newline", failures[0].reason)

    def test_rejects_gitignore_trailing_whitespace(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / ".gitignore").write_text("build/ \n", encoding="utf-8")

            failures = scan_tree(root)

        self.assertEqual(1, len(failures))
        self.assertEqual(Path(".gitignore"), failures[0].path)
        self.assertIn("trailing whitespace", failures[0].reason)

    def test_accepts_crlf_text_without_trailing_whitespace(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            path = root / "scripts" / "build.ps1"
            path.parent.mkdir()
            path.write_bytes(b"Write-Host 'ok'\r\n")

            self.assertEqual([], scan_tree(root))

    def test_ignores_binary_extensions(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            path = root / "image.png"
            path.write_bytes(b"\x89PNG\r\n\x1a\n\xff")

            self.assertEqual([], scan_tree(root))

    def test_requires_editorconfig_text_policy(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            write_valid_gitattributes(root)

            failures = check_text_policy(root)

        self.assertEqual(1, len(failures))
        self.assertEqual(Path(".editorconfig"), failures[0].path)
        self.assertIn("missing", failures[0].reason)

    def test_accepts_editorconfig_text_policy(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            write_valid_editorconfig(root)
            write_valid_gitattributes(root)

            self.assertEqual([], check_text_policy(root))

    def test_requires_gitattributes_text_policy(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            write_valid_editorconfig(root)

            failures = check_text_policy(root)

        self.assertEqual(1, len(failures))
        self.assertEqual(Path(".gitattributes"), failures[0].path)
        self.assertIn("missing", failures[0].reason)

    def test_rejects_incomplete_editorconfig_text_policy(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            write_valid_gitattributes(root)
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
