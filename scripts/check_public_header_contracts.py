"""Validate focused public header documentation contracts."""

import argparse
import re
from pathlib import Path


PUBLIC_HEADER_ROOT = Path("include") / "nexus"
NET_HEADERS = [
    "include/nexus/net/tcp.h",
    "include/nexus/net/tcp_listener.h",
    "include/nexus/net/udp.h",
    "include/nexus/net/http.h",
    "include/nexus/net/websocket_client.h",
    "include/nexus/net/websocket_server.h",
]
ENUMS = {
    "include/nexus/core/status.h": {"StatusCode"},
    "include/nexus/log/logger.h": {"Level"},
    "include/nexus/media/media.h": {
        "AudioSampleFormat",
        "MediaStreamType",
        "VideoPixelFormat",
    },
    "include/nexus/screen/screen.h": {"ScreenPixelFormat"},
    "include/nexus/usb/usb.h": {"UsbTransport"},
    "include/nexus/camera/types.h": {"CameraControlKind", "PixelFormat"},
}
LIFECYCLE_HEADERS = [
    "include/nexus/core/result.h",
    "include/nexus/core/status.h",
    "include/nexus/log/logger.h",
    "include/nexus/common/json.h",
    "include/nexus/common/thread.h",
    "include/nexus/common/xml.h",
    "include/nexus/media/media.h",
    "include/nexus/screen/screen.h",
    "include/nexus/usb/usb.h",
    "include/nexus/audio/capturer.h",
    "include/nexus/audio/player.h",
    "include/nexus/camera/capturer.h",
    "include/nexus/hid/hid.h",
    "include/nexus/net/tcp.h",
    "include/nexus/net/tcp_listener.h",
    "include/nexus/net/udp.h",
    "include/nexus/net/http.h",
    "include/nexus/net/websocket_client.h",
    "include/nexus/net/websocket_server.h",
]

HANDLER_RE = re.compile(r"^\s*using\s+\w*Handler\s*=")
ENUM_RE = re.compile(r"^\s*enum\s+class\s+(?:NEXUS_\w+_API\s+)?(\w+)\s*\{")
ENUM_VALUE_RE = re.compile(r"^\s*(\w+)\s*(?:=\s*[^,]+)?\s*,?\s*$")
CLASS_RE = re.compile(r"^\s*(class|struct)\s+(?:NEXUS_\w+_API\s+)?(\w+)\b")
DASH_PUNCTUATION = ("—", "–")


def has_doxygen(lines: list[str], index: int) -> bool:
    for probe in range(index - 1, max(index - 8, -1), -1):
        stripped = lines[probe].strip()
        if stripped.startswith("///"):
            return True
        if not stripped:
            continue
        break
    return False


def check_net_callback_typedefs(root: Path) -> list[str]:
    messages = []
    for relative in NET_HEADERS:
        path = root / relative
        if not path.exists():
            continue
        lines = path.read_text(encoding="utf-8").splitlines()
        for index, line in enumerate(lines):
            if HANDLER_RE.match(line) and not has_doxygen(lines, index):
                messages.append(f"{relative}:{index + 1}: {line.strip()}")
    return messages


def check_tracked_enum_values(root: Path) -> list[str]:
    messages = []
    for relative, enum_names in ENUMS.items():
        path = root / relative
        if not path.exists():
            continue
        lines = path.read_text(encoding="utf-8").splitlines()
        active_enum = None
        for index, line in enumerate(lines):
            if active_enum is None:
                match = ENUM_RE.match(line)
                if match and match.group(1) in enum_names:
                    active_enum = match.group(1)
                continue

            if line.strip().startswith("};"):
                active_enum = None
                continue

            stripped = line.strip()
            if not stripped or stripped.startswith("//"):
                continue

            match = ENUM_VALUE_RE.match(line)
            if match and not has_doxygen(lines, index):
                messages.append(f"{relative}:{index + 1}: {active_enum}::{match.group(1)}")
    return messages


def check_tracked_lifecycle_methods(root: Path) -> list[str]:
    messages = []
    for relative in LIFECYCLE_HEADERS:
        path = root / relative
        if not path.exists():
            continue
        lines = path.read_text(encoding="utf-8").splitlines()
        class_name = None
        in_public = False
        brace_depth = 0
        for index, line in enumerate(lines):
            stripped = line.strip()
            if class_name is None:
                match = CLASS_RE.match(stripped)
                if match and "{" in stripped:
                    class_name = match.group(2)
                    in_public = match.group(1) == "struct"
                    brace_depth = stripped.count("{") - stripped.count("}")
                continue

            brace_depth += stripped.count("{") - stripped.count("}")
            if brace_depth <= 0:
                class_name = None
                in_public = False
                continue

            if stripped == "public:":
                in_public = True
                continue
            if stripped in ("private:", "protected:"):
                in_public = False
                continue
            if not in_public:
                continue

            is_lifecycle = (
                re.match(rf"^(?:explicit\s+)?{class_name}\s*\(", stripped)
                or re.match(rf"^{class_name}\s*\(", stripped)
                or re.match(rf"^~{class_name}\s*\(", stripped)
                or re.match(rf"^{class_name}&\s+operator=\s*\(", stripped)
            )
            if is_lifecycle and not has_doxygen(lines, index):
                messages.append(f"{relative}:{index + 1}: {class_name}: {stripped}")
    return messages


def check_public_header_dash_punctuation(root: Path) -> list[str]:
    messages = []
    header_root = root / PUBLIC_HEADER_ROOT
    if not header_root.exists():
        return messages
    for path in sorted(header_root.rglob("*.h")):
        relative = path.relative_to(root).as_posix()
        lines = path.read_text(encoding="utf-8").splitlines()
        for index, line in enumerate(lines):
            if any(mark in line for mark in DASH_PUNCTUATION):
                messages.append(
                    f"{relative}:{index + 1}: public header uses non-ASCII dash punctuation"
                )
    return messages


def check_repo(root: Path) -> list[str]:
    root = root.resolve()
    messages = []
    messages.extend(check_net_callback_typedefs(root))
    messages.extend(check_tracked_enum_values(root))
    messages.extend(check_tracked_lifecycle_methods(root))
    messages.extend(check_public_header_dash_punctuation(root))
    return messages


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Check focused NexusKit public header contracts."
    )
    parser.add_argument("--dir", default=".", type=Path)
    args = parser.parse_args()

    messages = check_repo(args.dir)
    if messages:
        for message in messages:
            print(message)
        return 1

    print("Public header contract checks passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
