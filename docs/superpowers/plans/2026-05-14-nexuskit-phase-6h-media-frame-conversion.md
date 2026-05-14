# NexusKit Phase 6H Media Frame Conversion Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add first-frame conversion utilities for decoded media frames: audio resampling/sample-format conversion and video RGB/RGBA pixel conversion.

**Architecture:** Keep conversion APIs in `nexus_media` public headers without exposing FFmpeg types. Implement conversion privately with `libswresample` and `libswscale` when available, and return clear failed-precondition statuses when the FFmpeg conversion backend is unavailable.

**Tech Stack:** C++17, Catch2, CMake, FFmpeg `swresample`/`swscale`, NexusKit `Result<MediaFrame>`.

---

### Task 1: Audio Conversion API

**Files:**
- Modify: `include/nexus/media/media.h`
- Modify: `src/media/media.cpp`
- Modify: `src/media/CMakeLists.txt`
- Test: `tests/media/media_tests.cpp`

- [ ] **Step 1: Write the failing audio conversion test**

Add a test that decodes the generated WAV fixture, requests 16 kHz mono s16 output, and expects a converted frame:

```cpp
TEST_CASE("Media audio converter resamples decoded audio frames when FFmpeg backend is available") {
    const auto backend = nexus::media::ffmpeg_backend_info();
    if (!backend.available) {
        return;
    }

    const auto path = write_pcm_wav_fixture("convert-audio.wav");
    auto decoder = nexus::media::MediaDecoder::open(path);
    REQUIRE(decoder.ok());

    const auto frame = decoder.value().read_frame();
    REQUIRE(frame.ok());

    nexus::media::AudioConvertOptions options;
    options.sample_rate = 16000;
    options.channels = 1;
    options.sample_format = nexus::media::AudioSampleFormat::s16;

    const auto converted = nexus::media::convert_audio_frame(frame.value(), options);
    REQUIRE(converted.ok());
    CHECK(converted.value().type == nexus::media::MediaStreamType::audio);
    CHECK(converted.value().sample_rate == 16000);
    CHECK(converted.value().channels == 1);
    CHECK(converted.value().format_name == "s16");
    CHECK(converted.value().bytes_per_sample == 2);
    CHECK_FALSE(converted.value().planar);
    CHECK_FALSE(converted.value().data.empty());
}
```

- [ ] **Step 2: Verify the test fails**

Run:

```powershell
cmake --build build/phase6g-ffmpeg --target nexus_media_tests --config Debug
```

Expected: compile failure because `AudioConvertOptions`, `AudioSampleFormat`, and `convert_audio_frame` do not exist.

- [ ] **Step 3: Add public audio conversion API**

Add `AudioSampleFormat`, `AudioConvertOptions`, and `convert_audio_frame` declarations to `include/nexus/media/media.h`.

- [ ] **Step 4: Link FFmpeg conversion libraries**

Update `src/media/CMakeLists.txt` so `nexus_media` enables `NEXUS_MEDIA_WITH_FFMPEG` only when `FFmpeg::avutil`, `FFmpeg::avcodec`, `FFmpeg::avformat`, `FFmpeg::swresample`, and `FFmpeg::swscale` are available, and links all five privately.

- [ ] **Step 5: Implement minimal audio conversion**

Use `swr_alloc_set_opts2` on modern FFmpeg, default channel layouts from channel counts, packed output formats only, and compute input samples from `MediaFrame::data`, `channels`, and `bytes_per_sample`.

- [ ] **Step 6: Run media tests**

Run:

```powershell
build\phase6g-ffmpeg\bin\Debug\nexus_media_tests.exe
```

Expected: all media tests pass.

### Task 2: Video Conversion API

**Files:**
- Modify: `include/nexus/media/media.h`
- Modify: `src/media/media.cpp`
- Test: `tests/media/media_tests.cpp`

- [ ] **Step 1: Write the failing video conversion test**

Add a test that decodes the generated PPM fixture as video and converts it to RGBA:

```cpp
TEST_CASE("Media video converter converts decoded frames to RGBA when FFmpeg backend is available") {
    const auto backend = nexus::media::ffmpeg_backend_info();
    if (!backend.available) {
        return;
    }

    const auto path = write_ppm_fixture("convert-video.ppm");
    nexus::media::MediaDecodeOptions decode_options;
    decode_options.stream_type = nexus::media::MediaStreamType::video;
    auto decoder = nexus::media::MediaDecoder::open(path, decode_options);
    REQUIRE(decoder.ok());

    const auto frame = decoder.value().read_frame();
    REQUIRE(frame.ok());

    nexus::media::VideoConvertOptions options;
    options.pixel_format = nexus::media::VideoPixelFormat::rgba;

    const auto converted = nexus::media::convert_video_frame(frame.value(), options);
    REQUIRE(converted.ok());
    CHECK(converted.value().type == nexus::media::MediaStreamType::video);
    CHECK(converted.value().width == 2);
    CHECK(converted.value().height == 2);
    CHECK(converted.value().format_name == "rgba");
    CHECK(converted.value().data.size() == 2 * 2 * 4);
}
```

- [ ] **Step 2: Verify the test fails**

Run:

```powershell
cmake --build build/phase6g-ffmpeg --target nexus_media_tests --config Debug
```

Expected: compile failure because `VideoConvertOptions`, `VideoPixelFormat`, and `convert_video_frame` do not exist.

- [ ] **Step 3: Add public video conversion API**

Add `VideoPixelFormat`, `VideoConvertOptions`, and `convert_video_frame` declarations to `include/nexus/media/media.h`.

- [ ] **Step 4: Implement minimal video conversion**

Use `sws_getContext` and `sws_scale`, infer input format with `av_get_pix_fmt(frame.format_name.c_str())`, and support RGB24/RGBA/BGR24/BGRA outputs.

- [ ] **Step 5: Run media tests**

Run:

```powershell
build\phase6g-ffmpeg\bin\Debug\nexus_media_tests.exe
```

Expected: all media tests pass.

### Task 3: Documentation and Verification

**Files:**
- Modify: `README.md`
- Modify: `docs/modules/media.md`
- Modify: `CHANGELOG.md`

- [ ] **Step 1: Update docs**

Document audio conversion, video conversion, supported target formats, and backend availability behavior.

- [ ] **Step 2: Run final verification**

Run:

```powershell
$ff='E:/Cloudview/HikCommonDll/build/phase2b-ffmpeg-ucrt/deps/ffmpeg'
cmake -S . -B build/phase6h-ffmpeg -G "Visual Studio 17 2022" -A x64 -DNEXUS_BUILD_TESTS=ON -DNEXUS_BUILD_EXAMPLES=OFF -DNEXUS_ENABLE_MEDIA=ON -DNEXUS_ENABLE_LOG=ON -DNEXUS_ENABLE_COMMON=OFF -DNEXUS_ENABLE_NET=OFF -DNEXUS_ENABLE_HID=OFF -DNEXUS_ENABLE_USB=OFF "-DNEXUS_FFMPEG_INSTALL_DIR=$ff" -DCMAKE_INSTALL_PREFIX=E:/Cloudview/HikCommonDll/build/install/phase6h-ffmpeg
cmake --build build/phase6h-ffmpeg --target nexus_media_tests --config Debug
build\phase6h-ffmpeg\bin\Debug\nexus_media_tests.exe
cmake --install build/phase6h-ffmpeg --config Debug
```

Expected: configure, build, tests, and install pass.
