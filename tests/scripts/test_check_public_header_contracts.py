import tempfile
import unittest
from pathlib import Path

from scripts.check_public_header_contracts import check_repo


class PublicHeaderContractsCheckerTest(unittest.TestCase):
    def test_reports_public_header_dash_punctuation(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            header = root / "include" / "nexus" / "common" / "example.h"
            header.parent.mkdir(parents=True)
            header.write_text("/// bad dash — here\n", encoding="utf-8")

            messages = check_repo(root)

        self.assertEqual(
            ["include/nexus/common/example.h:1: public header uses non-ASCII dash punctuation"],
            messages,
        )

    def test_accepts_empty_repository_layout(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.assertEqual([], check_repo(Path(tmp)))


if __name__ == "__main__":
    unittest.main()
