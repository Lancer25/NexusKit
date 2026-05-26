import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
CI_WORKFLOW = ROOT / ".github" / "workflows" / "ci.yml"


class CiWorkflowTest(unittest.TestCase):
    def test_documentation_checker_tests_use_unittest_discovery(self):
        workflow = CI_WORKFLOW.read_text(encoding="utf-8")

        self.assertIn(
            "python -m unittest discover -s tests/scripts -p \"test_*.py\" -v",
            workflow,
        )
        self.assertNotIn("tests.scripts.test_check_docs", workflow)


if __name__ == "__main__":
    unittest.main()
