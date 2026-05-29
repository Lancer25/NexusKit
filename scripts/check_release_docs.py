"""Validate release-facing NexusKit documentation invariants."""

import argparse
import re
import sys
from pathlib import Path


RELEASE_FILES = [
    "README.md",
    "CHANGELOG.md",
    "docs/build.md",
    "docs/architecture.md",
    "docs/iteration.md",
    "docs/modules/hid.md",
    "docs/modules/usb.md",
]

FULL_BUILD_FLAGS = [
    "-DNEXUS_ENABLE_SCREEN=ON",
    "-DNEXUS_ENABLE_USB=ON",
    "-DNEXUS_ENABLE_HID=ON",
    "-DNEXUS_ENABLE_AUDIO=ON",
    "-DNEXUS_ENABLE_MEDIA=ON",
    "-DNEXUS_ENABLE_CAMERA=ON",
]


def _read_lines(root, relative):
    return (root / relative).read_text(encoding="utf-8").splitlines()


def _message(relative, line, text):
    return f"{relative}:{line}: {text}"


def _powershell_blocks(lines):
    in_block = False
    start = 0
    block = []
    for idx, line in enumerate(lines, start=1):
        stripped = line.strip().lower()
        if not in_block and stripped.startswith("```powershell"):
            in_block = True
            start = idx + 1
            block = []
            continue
        if in_block and stripped.startswith("```"):
            yield start, block
            in_block = False
            continue
        if in_block:
            block.append((idx, line))


def _check_changelog(root):
    relative = "CHANGELOG.md"
    lines = _read_lines(root, relative)
    for idx, line in enumerate(lines, start=1):
        if line.startswith("## "):
            if not re.match(r"^## 0\.1\.0 - \d{4}-\d{2}-\d{2}$", line):
                return [_message(relative, idx, "0.1.0 release heading must use a concrete date")]
            return []
    return [_message(relative, 1, "missing 0.1.0 release heading")]


def _check_powershell_blocks(root, relative):
    messages = []
    lines = _read_lines(root, relative)
    for _start, block in _powershell_blocks(lines):
        for pos, (line_no, line) in enumerate(block):
            if not line.rstrip().endswith("`"):
                continue
            if line != line.rstrip():
                messages.append(_message(relative, line_no, "PowerShell continuation must be the final character on the line"))
            if pos == len(block) - 1:
                messages.append(_message(relative, line_no, "PowerShell continuation cannot be the last line of a fenced block"))
                continue
            next_line = block[pos + 1][1]
            if not next_line.strip():
                messages.append(_message(relative, line_no, "PowerShell continuation cannot be followed by a blank line"))
    return messages


def _full_build_blocks(root, relative):
    lines = _read_lines(root, relative)
    for _start, block in _powershell_blocks(lines):
        text = "\n".join(line for _, line in block)
        if "cmake --preset windows-msvc-release" in text and "-DNEXUS_ENABLE_" in text:
            yield block, text


def _check_full_build_flags(root, relative):
    messages = []
    blocks = list(_full_build_blocks(root, relative))
    if not blocks:
        return [_message(relative, 1, "missing Windows release full-build PowerShell block")]
    for block, text in blocks:
        line_no = block[0][0] if block else 1
        for flag in FULL_BUILD_FLAGS:
            if flag not in text:
                messages.append(_message(relative, line_no, f"full-build block missing {flag}"))
    return messages


def _check_screen_default(root):
    messages = []
    for relative in ("README.md", "docs/build.md", "docs/architecture.md"):
        for idx, line in enumerate(_read_lines(root, relative), start=1):
            lowered = line.lower()
            if "nexus_screen" in lowered and "enabled by default" in lowered:
                messages.append(_message(relative, idx, "nexus_screen defaults to OFF; do not describe it as enabled by default"))
            if "enabled by default through `nexus_enable_screen=on`" in lowered:
                messages.append(_message(relative, idx, "NEXUS_ENABLE_SCREEN=ON enables an optional module; it is not the default"))
    return messages


def _check_hid_hotplug(root):
    messages = []
    allowed = ("not planned", "do not add", "unless the product requirement changes")
    for relative in ("docs/modules/hid.md", "docs/modules/usb.md", "docs/iteration.md"):
        for idx, line in enumerate(_read_lines(root, relative), start=1):
            lowered = line.lower()
            if "hid-level hotplug" in lowered and not any(token in lowered for token in allowed):
                messages.append(_message(relative, idx, "HID-level hotplug must remain explicitly not planned"))
    return messages


def _check_iteration_hygiene_summary(root):
    relative = "docs/iteration.md"
    text = "\n".join(_read_lines(root, relative))
    required = ("Phase 13E-13K", "Repository text and documentation hygiene")
    if all(token in text for token in required):
        return []
    return [
        _message(
            relative,
            1,
            "docs/iteration.md must summarize Phase 13E-13K Repository text and documentation hygiene",
        )
    ]


def _check_iteration_branch_guidance(root):
    relative = "docs/iteration.md"
    messages = []
    for idx, line in enumerate(_read_lines(root, relative), start=1):
        lowered = line.lower()
        if "commit and push to `main`" in lowered or "continue directly on `main`" in lowered:
            messages.append(
                _message(relative, idx, "default direct-work branch must be `master`")
            )
    return messages


def _check_iteration_stale_roadmap(root):
    relative = "docs/iteration.md"
    messages = []
    in_roadmap = False
    for idx, line in enumerate(_read_lines(root, relative), start=1):
        if line.startswith("## Near-Term Roadmap"):
            in_roadmap = True
            continue
        if in_roadmap and line.startswith("## "):
            in_roadmap = False
        if in_roadmap and line.lstrip().startswith("- Phase 13C:"):
            messages.append(
                _message(relative, idx, "Phase 13C is completed; do not list it as future roadmap work")
            )
        if in_roadmap and line.lstrip().startswith("- Phase 14A:"):
            messages.append(
                _message(relative, idx, "Phase 14A is completed; do not list it as future roadmap work")
            )
        if in_roadmap and line.lstrip().startswith("- Phase 14B:"):
            messages.append(
                _message(relative, idx, "Phase 14B is completed; do not list it as future roadmap work")
            )
        if in_roadmap and line.lstrip().startswith("- Phase 14C:"):
            messages.append(
                _message(relative, idx, "Phase 14C is completed; do not list it as future roadmap work")
            )
    return messages


def check_repo(root):
    root = Path(root)
    messages = []
    missing = [relative for relative in RELEASE_FILES if not (root / relative).exists()]
    for relative in missing:
        messages.append(_message(relative, 1, "required release-facing doc is missing"))
    if missing:
        return messages

    messages.extend(_check_changelog(root))
    for relative in ("README.md", "docs/build.md"):
        messages.extend(_check_powershell_blocks(root, relative))
        messages.extend(_check_full_build_flags(root, relative))
    messages.extend(_check_screen_default(root))
    messages.extend(_check_hid_hotplug(root))
    messages.extend(_check_iteration_hygiene_summary(root))
    messages.extend(_check_iteration_branch_guidance(root))
    messages.extend(_check_iteration_stale_roadmap(root))
    return messages


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--dir", default=".")
    args = parser.parse_args()

    messages = check_repo(Path(args.dir))
    if messages:
        for message in messages:
            print(message)
        return 1

    print("Release docs checks passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
