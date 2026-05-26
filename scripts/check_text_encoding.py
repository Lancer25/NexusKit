import argparse
from dataclasses import dataclass
from pathlib import Path


TEXT_EXTENSIONS = {
    ".bat",
    ".c",
    ".cc",
    ".cmake",
    ".cpp",
    ".cxx",
    ".h",
    ".hh",
    ".hpp",
    ".in",
    ".json",
    ".md",
    ".ps1",
    ".py",
    ".sh",
    ".txt",
    ".yml",
    ".yaml",
}
TEXT_FILENAMES = {
    "CHANGELOG.md",
    "CLAUDE.md",
    "CMakeLists.txt",
    "CONTRIBUTING.md",
    "LICENSE",
    "README.md",
}
SKIP_DIRS = {
    ".git",
    ".idea",
    ".vs",
    ".vscode",
    "__pycache__",
    "_deps",
    "build",
    "dist",
    "out",
    "release",
    "third_party",
}


@dataclass(frozen=True)
class EncodingFailure:
    path: Path
    reason: str


def is_text_candidate(path: Path) -> bool:
    return path.name in TEXT_FILENAMES or path.suffix.lower() in TEXT_EXTENSIONS


def should_skip(path: Path) -> bool:
    return any(part in SKIP_DIRS for part in path.parts)


def scan_tree(root: Path) -> list[EncodingFailure]:
    root = root.resolve()
    failures: list[EncodingFailure] = []
    for path in sorted(root.rglob("*")):
        if not path.is_file():
            continue
        relative = path.relative_to(root)
        if should_skip(relative) or not is_text_candidate(path):
            continue

        data = path.read_bytes()
        try:
            text = data.decode("utf-8")
        except UnicodeDecodeError as exc:
            reason = f"invalid utf-8 at byte {exc.start}: {exc.reason}"
            failures.append(EncodingFailure(relative, reason))
            continue

        if "\ufffd" in text:
            failures.append(
                EncodingFailure(relative, "contains unicode replacement character")
            )

    return failures


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Check repository text files are valid UTF-8."
    )
    parser.add_argument(
        "--dir",
        default=".",
        type=Path,
        help="Repository root to scan.",
    )
    args = parser.parse_args()

    failures = scan_tree(args.dir)
    if failures:
        for failure in failures:
            print(f"{failure.path}: {failure.reason}")
        return 1

    print("Text encoding checks passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
