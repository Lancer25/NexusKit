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
HANDLER_NAME_RE = re.compile(r"^\s*using\s+(\w*Handler)\s*=")
ENUM_RE = re.compile(r"^\s*enum\s+class\s+(?:NEXUS_\w+_API\s+)?(\w+)\s*\{")
ENUM_VALUE_RE = re.compile(r"^\s*(\w+)\s*(?:=\s*[^,]+)?\s*,?\s*$")
CLASS_RE = re.compile(r"^\s*(class|struct)\s+(?:NEXUS_\w+_API\s+)?(\w+)\b")
RESULT_DECL_RE = re.compile(
    r"^\s*(?:static\s+)?(?:NEXUS_\w+_API\s+)?Result<.+>\s+(\w+)\s*\("
)
STATUS_DECL_RE = re.compile(
    r"^\s*(?:static\s+)?(?:NEXUS_\w+_API\s+)?Status\s+(\w+)\s*\("
)
OPAQUE_IDENTIFIER_FIELD_RE = re.compile(
    r"^\s*std::string\s+(\w*(?:id|path)\w*)\s*;"
)
ZERO_TIMEOUT_FIELD_RE = re.compile(
    r"^\s*std::chrono::milliseconds\s+(\w*timeout)\s*\{\s*0\s*\}\s*;"
)
DASH_PUNCTUATION = ("\u2014", "\u2013")
STATUS_ERROR_CONTRACT_EXCLUDES = {
    "close",
    "stop",
    "ok_status",
    "invalid_argument",
    "not_found",
    "internal",
    "failed_precondition",
    "unavailable",
    "internal_error",
}
ERROR_STATUS_TOKENS = (
    "@retval",
    "StatusCode::kInvalidArgument",
    "StatusCode::kNotFound",
    "StatusCode::kAlreadyExists",
    "StatusCode::kPermissionDenied",
    "StatusCode::kResourceExhausted",
    "StatusCode::kFailedPrecondition",
    "StatusCode::kUnavailable",
    "StatusCode::kCancelled",
    "StatusCode::kInternal",
    "StatusCode::kUnknown",
    "`kInvalidArgument`",
    "`kNotFound`",
    "`kAlreadyExists`",
    "`kPermissionDenied`",
    "`kResourceExhausted`",
    "`kFailedPrecondition`",
    "`kUnavailable`",
    "`kCancelled`",
    "`kInternal`",
    "`kUnknown`",
)


def has_doxygen(lines: list[str], index: int) -> bool:
    for probe in range(index - 1, max(index - 8, -1), -1):
        stripped = lines[probe].strip()
        if stripped.startswith("///"):
            return True
        if not stripped:
            continue
        break
    return False


def doxygen_block(lines: list[str], index: int) -> list[str]:
    block = []
    for probe in range(index - 1, max(index - 12, -1), -1):
        stripped = lines[probe].strip()
        if stripped.startswith("///"):
            block.append(stripped[3:].strip())
            continue
        if not stripped:
            if block:
                continue
            continue
        break
    return list(reversed(block))


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


def check_net_callback_threading_contracts(root: Path) -> list[str]:
    messages = []
    contract_tokens = ("thread", "synchronous", "synchronously")
    for relative in NET_HEADERS:
        path = root / relative
        if not path.exists():
            continue
        lines = path.read_text(encoding="utf-8").splitlines()
        for index, line in enumerate(lines):
            match = HANDLER_NAME_RE.match(line)
            if not match:
                continue
            comment = " ".join(doxygen_block(lines, index)).lower()
            if not any(token in comment for token in contract_tokens):
                messages.append(
                    f"{relative}:{index + 1}: {match.group(1)} "
                    "missing threading or synchronous invocation contract"
                )
    return messages


def check_net_zero_timeout_field_contracts(root: Path) -> list[str]:
    messages = []
    required = "0 means no explicit timeout"
    for relative in NET_HEADERS:
        path = root / relative
        if not path.exists():
            continue
        lines = path.read_text(encoding="utf-8").splitlines()
        for index, line in enumerate(lines):
            match = ZERO_TIMEOUT_FIELD_RE.match(line)
            if not match:
                continue
            comment = " ".join(doxygen_block(lines, index)).lower()
            if required not in comment:
                messages.append(
                    f"{relative}:{index + 1}: {match.group(1)} "
                    "must document that 0 means no explicit timeout"
                )
    return messages


def check_move_only_ownership_contracts(root: Path) -> list[str]:
    messages = []
    header_root = root / PUBLIC_HEADER_ROOT
    if not header_root.exists():
        return messages
    for path in sorted(header_root.rglob("*.h")):
        relative = path.relative_to(root).as_posix()
        lines = path.read_text(encoding="utf-8").splitlines()
        for index, line in enumerate(lines):
            match = CLASS_RE.match(line.strip())
            if not match:
                continue
            comment = " ".join(doxygen_block(lines, index)).lower()
            if "move-only" not in comment:
                continue
            if "copy is deleted" not in comment or "move transfers ownership" not in comment:
                messages.append(
                    f"{relative}:{index + 1}: {match.group(2)} "
                    "must document that copy is deleted and move transfers ownership"
                )
    return messages


def check_result_error_contracts(root: Path) -> list[str]:
    messages = []
    header_root = root / PUBLIC_HEADER_ROOT
    if not header_root.exists():
        return messages
    for path in sorted(header_root.rglob("*.h")):
        relative = path.relative_to(root).as_posix()
        lines = path.read_text(encoding="utf-8").splitlines()
        class_name = None
        in_public = True
        brace_depth = 0
        for index, line in enumerate(lines):
            stripped = line.strip()
            if class_name is None:
                class_match = CLASS_RE.match(stripped)
                if class_match and "{" in stripped:
                    class_name = class_match.group(2)
                    in_public = class_match.group(1) == "struct"
                    brace_depth = stripped.count("{") - stripped.count("}")
                    continue
                in_public = True
            else:
                brace_depth += stripped.count("{") - stripped.count("}")
                if brace_depth <= 0:
                    class_name = None
                    in_public = True
                    continue
                if stripped == "public:":
                    in_public = True
                    continue
                if stripped in ("private:", "protected:"):
                    in_public = False
                    continue

            if not in_public:
                continue
            match = RESULT_DECL_RE.match(stripped)
            if not match:
                continue
            comment = " ".join(doxygen_block(lines, index))
            if not any(token in comment for token in ERROR_STATUS_TOKENS):
                messages.append(
                    f"{relative}:{index + 1}: {match.group(1)} "
                    "must document expected Result error status codes"
                )
    return messages


def check_status_error_contracts(root: Path) -> list[str]:
    messages = []
    header_root = root / PUBLIC_HEADER_ROOT
    if not header_root.exists():
        return messages
    for path in sorted(header_root.rglob("*.h")):
        relative = path.relative_to(root).as_posix()
        lines = path.read_text(encoding="utf-8").splitlines()
        class_name = None
        in_public = True
        brace_depth = 0
        for index, line in enumerate(lines):
            stripped = line.strip()
            if class_name is None:
                class_match = CLASS_RE.match(stripped)
                if class_match and "{" in stripped:
                    class_name = class_match.group(2)
                    in_public = class_match.group(1) == "struct"
                    brace_depth = stripped.count("{") - stripped.count("}")
                    continue
                in_public = True
            else:
                brace_depth += stripped.count("{") - stripped.count("}")
                if brace_depth <= 0:
                    class_name = None
                    in_public = True
                    continue
                if stripped == "public:":
                    in_public = True
                    continue
                if stripped in ("private:", "protected:"):
                    in_public = False
                    continue

            if not in_public:
                continue
            match = STATUS_DECL_RE.match(stripped)
            if not match:
                continue
            name = match.group(1)
            if name in STATUS_ERROR_CONTRACT_EXCLUDES:
                continue
            comment = " ".join(doxygen_block(lines, index))
            if not any(token in comment for token in ERROR_STATUS_TOKENS):
                messages.append(
                    f"{relative}:{index + 1}: {name} "
                    "must document expected Status error status codes"
                )
    return messages


def check_opaque_identifier_contracts(root: Path) -> list[str]:
    messages = []
    header_root = root / PUBLIC_HEADER_ROOT
    if not header_root.exists():
        return messages
    for path in sorted(header_root.rglob("*.h")):
        relative = path.relative_to(root).as_posix()
        lines = path.read_text(encoding="utf-8").splitlines()
        for index, line in enumerate(lines):
            match = OPAQUE_IDENTIFIER_FIELD_RE.match(line)
            if not match:
                continue
            comment = " ".join(doxygen_block(lines, index)).lower()
            if "opaque" not in comment:
                continue
            if "do not persist" not in comment and "use this to open" not in comment:
                messages.append(
                    f"{relative}:{index + 1}: {match.group(1)} "
                    "must document whether callers should persist or only use the opaque value to open"
                )
    return messages


def check_screen_visible_window_contracts(root: Path) -> list[str]:
    messages = []
    relative = "include/nexus/screen/screen.h"
    path = root / relative
    if not path.exists():
        return messages
    lines = path.read_text(encoding="utf-8").splitlines()
    for index, line in enumerate(lines):
        stripped = line.strip()
        if stripped.startswith("struct ScreenWindow"):
            comment = " ".join(doxygen_block(lines, index)).lower()
            required = ("visible", "minimized", "cross-display")
            if not all(token in comment for token in required):
                messages.append(
                    f"{relative}:{index + 1}: ScreenWindow "
                    "must document visible, minimized, and cross-display enumeration semantics"
                )
        if "capture_window(" in stripped:
            comment = " ".join(doxygen_block(lines, index)).lower()
            required = ("visible", "desktop", "occluding")
            if not all(token in comment for token in required):
                messages.append(
                    f"{relative}:{index + 1}: capture_window "
                    "must document visible desktop pixels and occlusion semantics"
                )
    return messages


def check_screen_frame_layout_contracts(root: Path) -> list[str]:
    messages = []
    relative = "include/nexus/screen/screen.h"
    path = root / relative
    if not path.exists():
        return messages
    lines = path.read_text(encoding="utf-8").splitlines()
    for index, line in enumerate(lines):
        if line.strip().startswith("struct ScreenFrame"):
            comment = " ".join(doxygen_block(lines, index)).lower()
            required = ("contiguous", "data.size()", "width * height * 4")
            if not all(token in comment for token in required):
                messages.append(
                    f"{relative}:{index + 1}: ScreenFrame "
                    "must document contiguous pixel data and width * height * 4 byte sizing"
                )
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
    checks = [
        ("callback-typedefs", check_net_callback_typedefs),
        ("callback-threading", check_net_callback_threading_contracts),
        ("zero-timeout-fields", check_net_zero_timeout_field_contracts),
        ("move-only-ownership", check_move_only_ownership_contracts),
        ("result-error-contracts", check_result_error_contracts),
        ("status-error-contracts", check_status_error_contracts),
        ("opaque-identifiers", check_opaque_identifier_contracts),
        ("screen-visible-window", check_screen_visible_window_contracts),
        ("screen-frame-layout", check_screen_frame_layout_contracts),
        ("enum-values", check_tracked_enum_values),
        ("lifecycle", check_tracked_lifecycle_methods),
        ("dash-punctuation", check_public_header_dash_punctuation),
    ]
    for category, check in checks:
        messages.extend(f"{category}: {message}" for message in check(root))
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
