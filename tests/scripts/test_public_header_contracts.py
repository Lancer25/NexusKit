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

HANDLER_RE = re.compile(r"^\s*using\s+\w*Handler\s*=")


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
    def test_net_callback_typedefs_have_doxygen_comments(self):
        missing = []
        for relative in NET_HEADERS:
            path = ROOT / relative
            lines = path.read_text(encoding="utf-8").splitlines()
            for index, line in enumerate(lines):
                if HANDLER_RE.match(line) and not has_doxygen(lines, index):
                    missing.append(f"{relative}:{index + 1}: {line.strip()}")

        self.assertEqual([], missing)


if __name__ == "__main__":
    unittest.main()
