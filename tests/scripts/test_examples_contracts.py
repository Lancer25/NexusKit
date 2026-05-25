import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


class ExamplesContractTest(unittest.TestCase):
    def test_media_probe_uses_public_nexus_media_api(self):
        example_dir = ROOT / "examples" / "media_probe"
        source = example_dir / "main.cpp"
        cmake = example_dir / "CMakeLists.txt"
        examples_cmake = ROOT / "examples" / "CMakeLists.txt"

        self.assertTrue(source.exists(), "examples/media_probe/main.cpp must exist")
        self.assertTrue(cmake.exists(), "examples/media_probe/CMakeLists.txt must exist")

        source_text = source.read_text(encoding="utf-8")
        cmake_text = cmake.read_text(encoding="utf-8")
        examples_text = examples_cmake.read_text(encoding="utf-8")

        self.assertIn("#include <nexus/media/media.h>", source_text)
        self.assertNotIn("libav", source_text)
        self.assertIn("nexus::media", cmake_text)
        self.assertNotIn("FFmpeg::", cmake_text)
        self.assertIn("if(TARGET nexus::media)", examples_text)
        self.assertIn("add_subdirectory(media_probe)", examples_text)

    def test_net_tcp_echo_uses_public_nexus_net_api(self):
        example_dir = ROOT / "examples" / "net_tcp_echo"
        source = example_dir / "main.cpp"
        cmake = example_dir / "CMakeLists.txt"
        examples_cmake = ROOT / "examples" / "CMakeLists.txt"

        self.assertTrue(source.exists(), "examples/net_tcp_echo/main.cpp must exist")
        self.assertTrue(cmake.exists(), "examples/net_tcp_echo/CMakeLists.txt must exist")

        source_text = source.read_text(encoding="utf-8").lower()
        cmake_text = cmake.read_text(encoding="utf-8")
        examples_text = examples_cmake.read_text(encoding="utf-8")

        self.assertIn("#include <nexus/net/tcp.h>", source_text)
        self.assertIn("#include <nexus/net/tcp_listener.h>", source_text)
        self.assertNotIn("asio", source_text)
        self.assertNotIn("httplib", source_text)
        self.assertNotIn("websocketpp", source_text)
        self.assertIn("nexus::net", cmake_text)
        self.assertNotIn("asio::", cmake_text)
        self.assertNotIn("httplib::", cmake_text)
        self.assertNotIn("websocketpp::", cmake_text)
        self.assertIn("if(TARGET nexus::net)", examples_text)
        self.assertIn("add_subdirectory(net_tcp_echo)", examples_text)


if __name__ == "__main__":
    unittest.main()
