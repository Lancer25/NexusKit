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
REQUIRED_EDITORCONFIG_SETTINGS = {
    "": {"root": "true"},
    "*": {
        "charset": "utf-8",
        "end_of_line": "lf",
        "insert_final_newline": "true",
        "trim_trailing_whitespace": "true",
    },
    "*.{bat,cmd,ps1}": {"end_of_line": "crlf"},
}


@dataclass(frozen=True)
class EncodingFailure:
    path: Path
    reason: str


def is_text_candidate(path: Path) -> bool:
    return path.name in TEXT_FILENAMES or path.suffix.lower() in TEXT_EXTENSIONS


def should_skip(path: Path) -> bool:
    return any(part in SKIP_DIRS for part in path.parts)


def _check_text_hygiene(relative: Path, text: str) -> list[EncodingFailure]:
    failures: list[EncodingFailure] = []
    if text and not text.endswith("\n"):
        failures.append(EncodingFailure(relative, "missing final newline"))

    for line_number, line in enumerate(text.splitlines(), start=1):
        if line.endswith((" ", "\t")):
            failures.append(
                EncodingFailure(
                    relative,
                    f"line {line_number}: trailing whitespace",
                )
            )

    return failures


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
            continue

        failures.extend(_check_text_hygiene(relative, text))

    return failures


def _parse_editorconfig(text: str) -> dict[str, dict[str, str]]:
    sections: dict[str, dict[str, str]] = {"": {}}
    current = ""
    for raw_line in text.splitlines():
        line = raw_line.strip()
        if not line or line.startswith(("#", ";")):
            continue
        if line.startswith("[") and line.endswith("]"):
            current = line[1:-1].strip()
            sections.setdefault(current, {})
            continue
        if "=" not in line:
            continue

        key, value = line.split("=", 1)
        sections.setdefault(current, {})[key.strip().lower()] = value.strip().lower()
    return sections


def check_text_policy(root: Path) -> list[EncodingFailure]:
    root = root.resolve()
    editorconfig = root / ".editorconfig"
    if not editorconfig.exists():
        return [EncodingFailure(Path(".editorconfig"), "missing .editorconfig")]

    try:
        text = editorconfig.read_text(encoding="utf-8")
    except UnicodeDecodeError as exc:
        reason = f"invalid utf-8 at byte {exc.start}: {exc.reason}"
        return [EncodingFailure(Path(".editorconfig"), reason)]

    sections = _parse_editorconfig(text)
    failures: list[EncodingFailure] = []
    for section, required_settings in REQUIRED_EDITORCONFIG_SETTINGS.items():
        values = sections.get(section)
        section_name = "top-level" if section == "" else f"[{section}]"
        if values is None:
            failures.append(
                EncodingFailure(Path(".editorconfig"), f"missing {section_name} section")
            )
            continue
        for key, expected in required_settings.items():
            actual = values.get(key)
            if actual != expected:
                display_key = key if section == "" else f"[{section}].{key}"
                failures.append(
                    EncodingFailure(
                        Path(".editorconfig"),
                        f"{display_key} must be {expected}",
                    )
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

    failures = scan_tree(args.dir) + check_text_policy(args.dir)
    if failures:
        for failure in failures:
            print(f"{failure.path}: {failure.reason}")
        return 1

    print("Text encoding checks passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
