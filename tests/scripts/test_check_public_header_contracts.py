import tempfile
import unittest
from pathlib import Path

from scripts.check_public_header_contracts import check_repo


class PublicHeaderContractsCheckerTest(unittest.TestCase):
    def test_reports_public_header_dash_punctuation(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            header = root / "include" / "nexus" / "common" / "example.h"
            header.parent.mkdir(parents=True)
            header.write_bytes(b"/// bad dash \xe2\x80\x94 here\n")

            messages = check_repo(root)

        self.assertEqual(
            ["dash-punctuation: include/nexus/common/example.h:1: public header uses non-ASCII dash punctuation"],
            messages,
        )

    def test_reports_net_callback_missing_threading_contract(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            header = root / "include" / "nexus" / "net" / "tcp.h"
            header.parent.mkdir(parents=True)
            header.write_text(
                "namespace nexus::net {\n"
                "class TcpClient {\n"
                "public:\n"
                "    /// Completion callback for async connect.\n"
                "    /// Receives OK when connected.\n"
                "    using ConnectHandler = std::function<void(Status)>;\n"
                "};\n"
                "} // namespace nexus::net\n",
                encoding="utf-8",
            )

            messages = check_repo(root)

        self.assertEqual(
            [
                "callback-threading: include/nexus/net/tcp.h:6: "
                "ConnectHandler missing threading or synchronous invocation contract"
            ],
            messages,
        )

    def test_reports_zero_timeout_field_missing_contract(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            header = root / "include" / "nexus" / "net" / "udp.h"
            header.parent.mkdir(parents=True)
            header.write_text(
                "#include <chrono>\n"
                "namespace nexus::net {\n"
                "struct UdpReceiveOptions {\n"
                "    /// Receive timeout.\n"
                "    std::chrono::milliseconds timeout{0};\n"
                "};\n"
                "} // namespace nexus::net\n",
                encoding="utf-8",
            )

            messages = check_repo(root)

        self.assertEqual(
            [
                "zero-timeout-fields: include/nexus/net/udp.h:5: "
                "timeout must document that 0 means no explicit timeout"
            ],
            messages,
        )

    def test_accepts_empty_repository_layout(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.assertEqual([], check_repo(Path(tmp)))


if __name__ == "__main__":
    unittest.main()
