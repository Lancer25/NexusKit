import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

from scripts import check_docs


ROOT = Path(__file__).resolve().parents[2]


class UnifiedDocsChecksTest(unittest.TestCase):
    def test_run_checks_returns_clean_results_when_all_checks_pass(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            with mock.patch.object(check_docs.check_text_encoding, "scan_tree", return_value=[]), \
                 mock.patch.object(check_docs.check_text_encoding, "check_text_policy", return_value=[]), \
                 mock.patch.object(check_docs.check_release_docs, "check_repo", return_value=[]), \
                 mock.patch.object(check_docs, "_check_public_comments", return_value=[]), \
                 mock.patch.object(check_docs.check_public_header_contracts, "check_repo", return_value=[]):
                results = check_docs.run_checks(root)

        self.assertTrue(all(result.ok for result in results))
        self.assertEqual(
            ["text-encoding", "release-docs", "public-comments", "public-header-contracts"],
            [result.name for result in results],
        )

    def test_run_checks_aggregates_failures(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            encoding_failure = check_docs.check_text_encoding.EncodingFailure(
                Path("bad.txt"), "invalid utf-8 at byte 1"
            )
            with mock.patch.object(check_docs.check_text_encoding, "scan_tree",
                                   return_value=[encoding_failure]), \
                 mock.patch.object(check_docs.check_text_encoding, "check_text_policy",
                                   return_value=[]), \
                 mock.patch.object(check_docs.check_release_docs, "check_repo",
                                   return_value=["README.md:1: missing heading"]), \
                 mock.patch.object(check_docs, "_check_public_comments",
                                   return_value=["include/nexus/example.h: Widget"]), \
                 mock.patch.object(check_docs.check_public_header_contracts, "check_repo",
                                   return_value=["include/nexus/example.h:1: bad contract"]):
                results = check_docs.run_checks(root)

        failures = {result.name: result.messages for result in results}
        self.assertEqual(["bad.txt: invalid utf-8 at byte 1"],
                         failures["text-encoding"])
        self.assertEqual(["README.md:1: missing heading"],
                         failures["release-docs"])
        self.assertEqual(["include/nexus/example.h: Widget"],
                         failures["public-comments"])
        self.assertEqual(["include/nexus/example.h:1: bad contract"],
                         failures["public-header-contracts"])

    def test_script_runs_as_direct_file(self):
        completed = subprocess.run(
            [sys.executable, "scripts/check_docs.py", "--dir", "."],
            cwd=ROOT,
            capture_output=True,
            text=True,
            check=False,
        )

        self.assertEqual("", completed.stderr)
        self.assertEqual(0, completed.returncode)
        self.assertIn("Documentation checks passed.", completed.stdout)


if __name__ == "__main__":
    unittest.main()
