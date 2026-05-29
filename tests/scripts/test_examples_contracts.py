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

    def test_media_remux_uses_public_nexus_media_api(self):
        example_dir = ROOT / "examples" / "media_remux"
        source = example_dir / "main.cpp"
        cmake = example_dir / "CMakeLists.txt"
        examples_cmake = ROOT / "examples" / "CMakeLists.txt"

        self.assertTrue(source.exists(), "examples/media_remux/main.cpp must exist")
        self.assertTrue(cmake.exists(), "examples/media_remux/CMakeLists.txt must exist")

        source_text = source.read_text(encoding="utf-8")
        cmake_text = cmake.read_text(encoding="utf-8")
        examples_text = examples_cmake.read_text(encoding="utf-8")

        self.assertIn("#include <nexus/media/media.h>", source_text)
        self.assertIn("nexus::media::remux_file", source_text)
        self.assertNotIn("libav", source_text)
        self.assertIn("nexus::media", cmake_text)
        self.assertNotIn("FFmpeg::", cmake_text)
        self.assertIn("if(TARGET nexus::media)", examples_text)
        self.assertIn("add_subdirectory(media_remux)", examples_text)

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

    def test_audio_probe_uses_public_nexus_audio_api(self):
        example_dir = ROOT / "examples" / "audio_probe"
        source = example_dir / "main.cpp"
        cmake = example_dir / "CMakeLists.txt"
        examples_cmake = ROOT / "examples" / "CMakeLists.txt"

        self.assertTrue(source.exists(), "examples/audio_probe/main.cpp must exist")
        self.assertTrue(cmake.exists(), "examples/audio_probe/CMakeLists.txt must exist")

        source_text = source.read_text(encoding="utf-8").lower()
        cmake_text = cmake.read_text(encoding="utf-8")
        examples_text = examples_cmake.read_text(encoding="utf-8")

        self.assertIn("#include <nexus/audio/capturer.h>", source_text)
        self.assertIn("#include <nexus/audio/player.h>", source_text)
        for private_name in ("wasapi", "pulse", "mmdeviceapi", "audioclient"):
            self.assertNotIn(private_name, source_text)
        self.assertIn("nexus::audio", cmake_text)
        self.assertNotIn("pulse", cmake_text.lower())
        self.assertIn("if(TARGET nexus::audio)", examples_text)
        self.assertIn("add_subdirectory(audio_probe)", examples_text)

    def test_camera_probe_uses_public_nexus_camera_api(self):
        example_dir = ROOT / "examples" / "camera_probe"
        source = example_dir / "main.cpp"
        cmake = example_dir / "CMakeLists.txt"
        examples_cmake = ROOT / "examples" / "CMakeLists.txt"

        self.assertTrue(source.exists(), "examples/camera_probe/main.cpp must exist")
        self.assertTrue(cmake.exists(), "examples/camera_probe/CMakeLists.txt must exist")

        source_text = source.read_text(encoding="utf-8").lower()
        cmake_text = cmake.read_text(encoding="utf-8")
        examples_text = examples_cmake.read_text(encoding="utf-8")

        self.assertIn("#include <nexus/camera/capturer.h>", source_text)
        for private_name in ("libuvc", "v4l2", "linux/videodev2", "uvc/"):
            self.assertNotIn(private_name, source_text)
        self.assertIn("nexus::camera", cmake_text)
        self.assertNotIn("uvc", cmake_text.lower())
        self.assertIn("if(TARGET nexus::camera)", examples_text)
        self.assertIn("add_subdirectory(camera_probe)", examples_text)


if __name__ == "__main__":
    unittest.main()
