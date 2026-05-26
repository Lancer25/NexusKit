import tempfile
import unittest
from pathlib import Path

from scripts.check_text_encoding import scan_tree


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


if __name__ == "__main__":
    unittest.main()
