import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
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
    "include/nexus/camera/types.h": {"CameraControlKind", "PixelFormat"},
}

HANDLER_RE = re.compile(r"^\s*using\s+\w*Handler\s*=")
ENUM_RE = re.compile(r"^\s*enum\s+class\s+(?:NEXUS_\w+_API\s+)?(\w+)\s*\{")
ENUM_VALUE_RE = re.compile(r"^\s*(\w+)\s*(?:=\s*[^,]+)?\s*,?\s*$")


def has_doxygen(lines, index):
    for probe in range(index - 1, max(index - 8, -1), -1):
        stripped = lines[probe].strip()
        if stripped.startswith("///"):
            return True
        if not stripped:
            continue
        break
    return False


class PublicHeaderContractsTest(unittest.TestCase):
    maxDiff = None

    def test_net_callback_typedefs_have_doxygen_comments(self):
        missing = []
        for relative in NET_HEADERS:
            path = ROOT / relative
            lines = path.read_text(encoding="utf-8").splitlines()
            for index, line in enumerate(lines):
                if HANDLER_RE.match(line) and not has_doxygen(lines, index):
                    missing.append(f"{relative}:{index + 1}: {line.strip()}")

        self.assertEqual([], missing)

    def test_tracked_enum_values_have_doxygen_comments(self):
        missing = []
        for relative, enum_names in ENUMS.items():
            path = ROOT / relative
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
                    missing.append(
                        f"{relative}:{index + 1}: {active_enum}::{match.group(1)}"
                    )

        self.assertEqual([], missing)


if __name__ == "__main__":
    unittest.main()
