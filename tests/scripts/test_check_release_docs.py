import tempfile
import unittest
from pathlib import Path

from scripts import check_release_docs


def write_file(root, relative, text):
    path = Path(root) / relative
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")


def minimal_repo(root):
    write_file(
        root,
        "CHANGELOG.md",
        "# Changelog\n\n## 0.1.0 - 2026-05-20\n",
    )
    write_file(
        root,
        "README.md",
        """# NexusKit

### All modules

```powershell
cmake --preset windows-msvc-release -B build/windows-msvc-release `
  -DNEXUS_ENABLE_SCREEN=ON `
  -DNEXUS_ENABLE_USB=ON `
  -DNEXUS_ENABLE_HID=ON -DNEXUS_BUILD_HIDAPI=ON `
  -DNEXUS_ENABLE_AUDIO=ON `
  -DNEXUS_ENABLE_MEDIA=ON -DNEXUS_BUILD_FFMPEG=ON `
  -DNEXUS_ENABLE_CAMERA=ON -DNEXUS_BUILD_LIBUVC=ON
```
""",
    )
    write_file(
        root,
        "docs/build.md",
        """# Build

```powershell
cmake --preset windows-msvc-release -B build/windows-msvc-release `
  -DNEXUS_ENABLE_SCREEN=ON `
  -DNEXUS_ENABLE_USB=ON `
  -DNEXUS_ENABLE_HID=ON -DNEXUS_BUILD_HIDAPI=ON `
  -DNEXUS_ENABLE_AUDIO=ON `
  -DNEXUS_ENABLE_MEDIA=ON -DNEXUS_BUILD_FFMPEG=ON `
  -DNEXUS_ENABLE_CAMERA=ON -DNEXUS_BUILD_LIBUVC=ON
```

`nexus_screen` is enabled with `NEXUS_ENABLE_SCREEN=ON` and defaults to `OFF`.
""",
    )
    write_file(root, "docs/architecture.md", "# Architecture\n")
    write_file(
        root,
        "docs/iteration.md",
        "Phase 13E-13K: Repository text and documentation hygiene checks.\n"
        "Commit and push to `master` unless the active task says otherwise.\n"
        "Continue directly on `master` unless the user requests a branch.\n"
        "Do not add HID-level hotplug support unless the product requirement changes.\n",
    )
    write_file(
        root,
        "docs/modules/hid.md",
        "HID-level hotplug is not planned; use `nexus::usb::UsbHotplugMonitor`.\n",
    )
    write_file(
        root,
        "docs/modules/usb.md",
        "HID-level hotplug is not planned; use `UsbHotplugMonitor`.\n",
    )


class ReleaseDocsChecksTest(unittest.TestCase):
    def test_accepts_minimal_valid_docs(self):
        with tempfile.TemporaryDirectory() as tmp:
            minimal_repo(tmp)

            self.assertEqual([], check_release_docs.check_repo(Path(tmp)))

    def test_rejects_unreleased_changelog_heading(self):
        with tempfile.TemporaryDirectory() as tmp:
            minimal_repo(tmp)
            write_file(tmp, "CHANGELOG.md", "# Changelog\n\n## 0.1.0 - Unreleased\n")

            messages = check_release_docs.check_repo(Path(tmp))

        self.assertTrue(any("0.1.0 release heading" in message for message in messages))

    def test_rejects_powershell_continuation_before_fence_close(self):
        with tempfile.TemporaryDirectory() as tmp:
            minimal_repo(tmp)
            write_file(
                tmp,
                "README.md",
                """# NexusKit

```powershell
cmake --preset windows-msvc-release `
```
""",
            )

            messages = check_release_docs.check_repo(Path(tmp))

        self.assertTrue(any("PowerShell continuation" in message for message in messages))

    def test_rejects_full_build_missing_screen(self):
        with tempfile.TemporaryDirectory() as tmp:
            minimal_repo(tmp)
            readme = Path(tmp) / "README.md"
            readme.write_text(
                readme.read_text(encoding="utf-8").replace(
                    "  -DNEXUS_ENABLE_SCREEN=ON `\n", ""
                ),
                encoding="utf-8",
            )

            messages = check_release_docs.check_repo(Path(tmp))

        self.assertTrue(any("missing -DNEXUS_ENABLE_SCREEN=ON" in message for message in messages))

    def test_rejects_hid_hotplug_without_not_planned_wording(self):
        with tempfile.TemporaryDirectory() as tmp:
            minimal_repo(tmp)
            write_file(tmp, "docs/modules/hid.md", "HID-level hotplug is planned follow-up work.\n")

            messages = check_release_docs.check_repo(Path(tmp))

        self.assertTrue(any("HID-level hotplug" in message for message in messages))

    def test_rejects_iteration_without_phase13_hygiene_summary(self):
        with tempfile.TemporaryDirectory() as tmp:
            minimal_repo(tmp)
            write_file(
                tmp,
                "docs/iteration.md",
                "Commit and push to `master` unless the active task says otherwise.\n"
                "Do not add HID-level hotplug support unless the product requirement changes.\n",
            )

            messages = check_release_docs.check_repo(Path(tmp))

        self.assertTrue(any("Phase 13E-13K" in message for message in messages))

    def test_rejects_iteration_defaulting_to_main_branch(self):
        with tempfile.TemporaryDirectory() as tmp:
            minimal_repo(tmp)
            write_file(
                tmp,
                "docs/iteration.md",
                "Phase 13E-13K: Repository text and documentation hygiene checks.\n"
                "Commit and push to `main` unless the active task says otherwise.\n"
                "Continue directly on `main` unless the user requests a branch.\n"
                "Do not add HID-level hotplug support unless the product requirement changes.\n",
            )

            messages = check_release_docs.check_repo(Path(tmp))

        self.assertTrue(any("master" in message for message in messages))


if __name__ == "__main__":
    unittest.main()
