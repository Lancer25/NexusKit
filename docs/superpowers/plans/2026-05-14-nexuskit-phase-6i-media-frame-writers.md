# NexusKit Phase 6I Media Frame Writers Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add lightweight media frame writing helpers so decoded/converted NexusKit frames can be written to inspectable WAV and PPM files.

**Architecture:** Keep this slice independent of FFmpeg muxing and encoding. Implement small file-format writers with standard C++ streams: PCM WAV for packed audio frames and binary PPM (`P6`) for RGB-family video frames.

**Tech Stack:** C++17, `<filesystem>`, `<fstream>`, Catch2, NexusKit `Status`.

---

### Task 1: PCM WAV Writer

**Files:**
- Modify: `include/nexus/media/media.h`
- Modify: `src/media/media.cpp`
- Test: `tests/media/media_tests.cpp`

- [ ] **Step 1: Write the failing WAV writer test**

Add a test that decodes the generated WAV fixture and writes the decoded frame to another WAV file:

```cpp
TEST_CASE("Media WAV writer writes decoded PCM audio frames") {
    const auto backend = nexus::media::ffmpeg_backend_info();
    if (!backend.available) {
        return;
    }

    const auto input_path = write_pcm_wav_fixture("writer-input.wav");
    auto decoder = nexus::media::MediaDecoder::open(input_path);
    REQUIRE(decoder.ok());
    const auto frame = decoder.value().read_frame();
    REQUIRE(frame.ok());

    const auto output_path = test_media_path("writer-output.wav");
    const auto status = nexus::media::write_wav_file(output_path, {frame.value()});
    REQUIRE(status.ok());

    const auto probe = nexus::media::probe_media(output_path);
    REQUIRE(probe.ok());
    REQUIRE(probe.value().streams.size() == 1);
    CHECK(probe.value().streams[0].type == nexus::media::MediaStreamType::audio);
    CHECK(probe.value().streams[0].sample_rate == frame.value().sample_rate);
    CHECK(probe.value().streams[0].channels == frame.value().channels);
}
```

- [ ] **Step 2: Verify the test fails**

Run:

```powershell
cmake --build build/phase6h-ffmpeg --target nexus_media_tests --config Debug
```

Expected: compile failure because `write_wav_file` does not exist.

- [ ] **Step 3: Add public WAV writer declaration**

Add:

```cpp
NEXUS_MEDIA_API Status write_wav_file(
    const std::filesystem::path& path,
    const std::vector<MediaFrame>& frames);
```

- [ ] **Step 4: Implement minimal PCM WAV writing**

Validate non-empty path, non-empty frames, all frames are packed audio with matching `sample_rate`, `channels`, `bytes_per_sample`, and `format_name == "s16"`. Write RIFF/WAVE headers and concatenate frame bytes.

- [ ] **Step 5: Run media tests**

Run:

```powershell
build\phase6h-ffmpeg\bin\Debug\nexus_media_tests.exe
```

Expected: all media tests pass.

### Task 2: PPM Writer

**Files:**
- Modify: `include/nexus/media/media.h`
- Modify: `src/media/media.cpp`
- Test: `tests/media/media_tests.cpp`

- [ ] **Step 1: Write the failing PPM writer test**

Add a test that decodes the PPM fixture, converts it to RGB24, writes it, and probes it again:

```cpp
TEST_CASE("Media PPM writer writes RGB video frames") {
    const auto backend = nexus::media::ffmpeg_backend_info();
    if (!backend.available) {
        return;
    }

    const auto input_path = write_ppm_fixture("writer-input.ppm");
    nexus::media::MediaDecodeOptions decode_options;
    decode_options.stream_type = nexus::media::MediaStreamType::video;
    auto decoder = nexus::media::MediaDecoder::open(input_path, decode_options);
    REQUIRE(decoder.ok());

    const auto frame = decoder.value().read_frame();
    REQUIRE(frame.ok());

    nexus::media::VideoConvertOptions convert_options;
    convert_options.pixel_format = nexus::media::VideoPixelFormat::rgb24;
    const auto rgb = nexus::media::convert_video_frame(frame.value(), convert_options);
    REQUIRE(rgb.ok());

    const auto output_path = test_media_path("writer-output.ppm");
    const auto status = nexus::media::write_ppm_file(output_path, rgb.value());
    REQUIRE(status.ok());

    const auto probe = nexus::media::probe_media(output_path);
    REQUIRE(probe.ok());
    REQUIRE(probe.value().streams.size() == 1);
    CHECK(probe.value().streams[0].type == nexus::media::MediaStreamType::video);
    CHECK(probe.value().streams[0].width == 2);
    CHECK(probe.value().streams[0].height == 2);
}
```

- [ ] **Step 2: Verify the test fails**

Run:

```powershell
cmake --build build/phase6h-ffmpeg --target nexus_media_tests --config Debug
```

Expected: compile failure because `write_ppm_file` does not exist.

- [ ] **Step 3: Add public PPM writer declaration**

Add:

```cpp
NEXUS_MEDIA_API Status write_ppm_file(
    const std::filesystem::path& path,
    const MediaFrame& frame);
```

- [ ] **Step 4: Implement minimal PPM writing**

Validate non-empty path, video frame type, positive width/height, and format `rgb24` or `rgba`. Write binary `P6` header. For `rgb24`, write data directly. For `rgba`, drop alpha while writing RGB bytes.

- [ ] **Step 5: Run media tests**

Run:

```powershell
build\phase6h-ffmpeg\bin\Debug\nexus_media_tests.exe
```

Expected: all media tests pass.

### Task 3: Documentation and Verification

**Files:**
- Modify: `README.md`
- Modify: `docs/modules/media.md`
- Modify: `CHANGELOG.md`

- [ ] **Step 1: Update docs**

Document `write_wav_file` and `write_ppm_file`, including the supported formats and the deliberate non-goal of full encoding/muxing.

- [ ] **Step 2: Run final verification**

Run:

```powershell
$ff='E:/Cloudview/HikCommonDll/build/phase2b-ffmpeg-ucrt/deps/ffmpeg'
cmake -S . -B build/phase6i-ffmpeg -G "Visual Studio 17 2022" -A x64 -DNEXUS_BUILD_TESTS=ON -DNEXUS_BUILD_EXAMPLES=OFF -DNEXUS_ENABLE_MEDIA=ON -DNEXUS_ENABLE_LOG=ON -DNEXUS_ENABLE_COMMON=OFF -DNEXUS_ENABLE_NET=OFF -DNEXUS_ENABLE_HID=OFF -DNEXUS_ENABLE_USB=OFF "-DNEXUS_FFMPEG_INSTALL_DIR=$ff" -DCMAKE_INSTALL_PREFIX=E:/Cloudview/HikCommonDll/build/install/phase6i-ffmpeg
cmake --build build/phase6i-ffmpeg --target nexus_media_tests --config Debug
build\phase6i-ffmpeg\bin\Debug\nexus_media_tests.exe
cmake --install build/phase6i-ffmpeg --config Debug
```

Expected: configure, build, tests, and install pass.
