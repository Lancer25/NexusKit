"""Run NexusKit documentation and public API hygiene checks."""

import argparse
import sys
from dataclasses import dataclass
from pathlib import Path

if __package__ in (None, ""):
    sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from scripts import check_public_comments  # noqa: E402
from scripts import check_public_header_contracts  # noqa: E402
from scripts import check_release_docs  # noqa: E402
from scripts import check_text_encoding  # noqa: E402


@dataclass(frozen=True)
class CheckResult:
    name: str
    messages: list[str]

    @property
    def ok(self) -> bool:
        return not self.messages


def _check_public_comments(root: Path) -> list[str]:
    messages = []
    for header in check_public_comments.TARGET_HEADERS:
        path = root / header
        if not path.exists():
            continue
        missing = check_public_comments.check_header(path)
        for name in missing:
            messages.append(f"{header}: {name}")
    return messages


def run_checks(root: Path) -> list[CheckResult]:
    root = root.resolve()
    encoding_messages = [
        f"{failure.path}: {failure.reason}"
        for failure in check_text_encoding.scan_tree(root)
    ]
    return [
        CheckResult("text-encoding", encoding_messages),
        CheckResult("release-docs", check_release_docs.check_repo(root)),
        CheckResult("public-comments", _check_public_comments(root)),
        CheckResult("public-header-contracts", check_public_header_contracts.check_repo(root)),
    ]


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Run NexusKit documentation quality checks."
    )
    parser.add_argument(
        "--dir",
        default=".",
        type=Path,
        help="Repository root to scan.",
    )
    args = parser.parse_args()

    results = run_checks(args.dir)
    failed = [result for result in results if not result.ok]
    if failed:
        for result in failed:
            print(f"{result.name} failed:")
            for message in result.messages:
                print(f"  {message}")
        return 1

    print("Documentation checks passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
