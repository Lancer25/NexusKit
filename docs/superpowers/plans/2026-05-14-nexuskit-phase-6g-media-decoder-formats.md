# NexusKit Phase 6G Media Decoder Formats Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extend `nexus_media` decoding so callers can choose audio or video decoding and inspect basic frame format metadata without exposing FFmpeg types.

**Architecture:** Keep `MediaDecoder::open(path)` backward compatible by defaulting to audio. Add `MediaDecodeOptions` for explicit stream selection, extend `MediaFrame` with format metadata, and keep all FFmpeg ownership inside `src/media/media.cpp`.

**Tech Stack:** C++17, CMake, Catch2, FFmpeg `avformat`/`avcodec`/`avutil`, NexusKit `Status`/`Result`.

---

### Task 1: Decoder Options and Audio Format Metadata

**Files:**
- Modify: `include/nexus/media/media.h`
- Modify: `src/media/media.cpp`
- Test: `tests/media/media_tests.cpp`
- Docs: `docs/modules/media.md`

- [ ] **Step 1: Write the failing tests**

Add tests that expect a `MediaDecodeOptions` type, an `open(path, options)` overload, and audio format metadata on decoded WAV frames:

```cpp
TEST_CASE("Media decoder accepts explicit audio options") {
    const auto backend = nexus::media::ffmpeg_backend_info();
    if (!backend.available) {
        return;
    }

    const auto path = write_pcm_wav_fixture("decoder-options-audio.wav");
    nexus::media::MediaDecodeOptions options;
    options.stream_type = nexus::media::MediaStreamType::audio;

    auto decoder = nexus::media::MediaDecoder::open(path, options);
    REQUIRE(decoder.ok());

    const auto frame = decoder.value().read_frame();
    REQUIRE(frame.ok());
    CHECK(frame.value().type == nexus::media::MediaStreamType::audio);
    CHECK(frame.value().sample_rate == 8000);
    CHECK(frame.value().channels == 1);
    CHECK(frame.value().format_name == "s16");
    CHECK(frame.value().bytes_per_sample == 2);
    CHECK_FALSE(frame.value().planar);
    CHECK_FALSE(frame.value().data.empty());
}
```

- [ ] **Step 2: Run the media test build to verify it fails**

Run:

```powershell
cmake --build --preset windows-msvc-debug --target nexus_media_tests
```

Expected: compile failure because `MediaDecodeOptions`, `MediaFrame::format_name`, `MediaFrame::bytes_per_sample`, and `MediaFrame::planar` do not exist.

- [ ] **Step 3: Add public API fields**

In `include/nexus/media/media.h`, add:

```cpp
struct MediaDecodeOptions {
    MediaStreamType stream_type = MediaStreamType::audio;
};
```

Extend `MediaFrame`:

```cpp
std::string format_name;
int bytes_per_sample = 0;
bool planar = false;
```

Add an overload:

```cpp
static Result<MediaDecoder> open(const std::filesystem::path& path, const MediaDecodeOptions& options);
```

- [ ] **Step 4: Implement minimal audio option and metadata support**

In `src/media/media.cpp`, keep `MediaDecoder::open(path)` delegating to `open(path, MediaDecodeOptions{})`. Store the selected `MediaStreamType` in `MediaDecoderStorage`, use it to pick `AVMEDIA_TYPE_AUDIO`, and populate `MediaFrame::format_name`, `bytes_per_sample`, and `planar` from `AVSampleFormat`.

- [ ] **Step 5: Run the media tests**

Run:

```powershell
ctest --preset windows-msvc-debug -R nexus_media_tests --output-on-failure
```

Expected: media tests pass in the configured build.

### Task 2: Planar Audio Copy Safety

**Files:**
- Modify: `src/media/media.cpp`
- Test: `tests/media/media_tests.cpp`

- [ ] **Step 1: Write the failing unit-level expectation**

Extend the explicit audio options test to assert that frame data size is compatible with `channels * bytes_per_sample`:

```cpp
REQUIRE(frame.value().bytes_per_sample > 0);
REQUIRE(frame.value().channels > 0);
CHECK(frame.value().data.size() % static_cast<std::size_t>(
    frame.value().channels * frame.value().bytes_per_sample) == 0);
```

- [ ] **Step 2: Run the test**

Run:

```powershell
ctest --preset windows-msvc-debug -R nexus_media_tests --output-on-failure
```

Expected: pass for packed WAV. This establishes the invariant before touching copy code.

- [ ] **Step 3: Make audio frame copying handle planar and packed layouts explicitly**

In `MediaDecoder::read_frame`, branch on `av_sample_fmt_is_planar`. For packed audio, copy `nb_samples * channels * bytes_per_sample` from `frame->data[0]`. For planar audio, append each channel plane in channel order using `nb_samples * bytes_per_sample` bytes per plane.

- [ ] **Step 4: Run the media tests again**

Run:

```powershell
ctest --preset windows-msvc-debug -R nexus_media_tests --output-on-failure
```

Expected: media tests still pass.

### Task 3: Video Decode Selection

**Files:**
- Modify: `tests/media/media_tests.cpp`
- Modify: `src/media/media.cpp`
- Docs: `docs/modules/media.md`

- [ ] **Step 1: Write the failing video decode test**

Add a helper that writes a minimal PPM image and invokes the media decoder in video mode:

```cpp
std::filesystem::path write_ppm_fixture(const std::string& name) {
    const auto path = test_media_path(name);
    std::ofstream file(path, std::ios::binary);
    file << "P6\n2 2\n255\n";
    const unsigned char pixels[] = {
        255, 0, 0,
        0, 255, 0,
        0, 0, 255,
        255, 255, 255,
    };
    file.write(reinterpret_cast<const char*>(pixels), sizeof(pixels));
    return path;
}

TEST_CASE("Media decoder reads video frames when FFmpeg backend is available") {
    const auto backend = nexus::media::ffmpeg_backend_info();
    if (!backend.available) {
        return;
    }

    const auto path = write_ppm_fixture("decoder-video.ppm");
    nexus::media::MediaDecodeOptions options;
    options.stream_type = nexus::media::MediaStreamType::video;

    auto decoder = nexus::media::MediaDecoder::open(path, options);
    REQUIRE(decoder.ok());

    const auto frame = decoder.value().read_frame();
    REQUIRE(frame.ok());
    CHECK(frame.value().type == nexus::media::MediaStreamType::video);
    CHECK(frame.value().width == 2);
    CHECK(frame.value().height == 2);
    CHECK_FALSE(frame.value().format_name.empty());
    CHECK_FALSE(frame.value().data.empty());
}
```

- [ ] **Step 2: Run the media test to verify it fails**

Run:

```powershell
ctest --preset windows-msvc-debug -R nexus_media_tests --output-on-failure
```

Expected: failure because decoder open still only selects audio streams.

- [ ] **Step 3: Implement video stream selection and frame copying**

Map `MediaStreamType::video` to `AVMEDIA_TYPE_VIDEO` in `MediaDecoder::open`. Store the requested stream type. In `read_frame`, set `MediaFrame::type`, `width`, `height`, and `format_name` for video frames. Copy video frame data plane-by-plane using `frame->linesize[plane]` and `av_image_fill_linesizes`-compatible plane counts for common FFmpeg decoded formats, without converting pixel formats.

- [ ] **Step 4: Run media tests**

Run:

```powershell
ctest --preset windows-msvc-debug -R nexus_media_tests --output-on-failure
```

Expected: media tests pass.

### Task 4: Documentation and Full Verification

**Files:**
- Modify: `README.md`
- Modify: `docs/modules/media.md`
- Modify: `CHANGELOG.md`

- [ ] **Step 1: Update docs**

Document `MediaDecodeOptions`, frame format metadata, audio/video decode scope, and the fact that video frames are returned in backend-native pixel format without conversion.

- [ ] **Step 2: Run build and tests**

Run:

```powershell
cmake --build --preset windows-msvc-debug
ctest --preset windows-msvc-debug --output-on-failure
cmake --install build/windows-msvc-debug
```

Expected: build, tests, and install complete successfully.

- [ ] **Step 3: Inspect git diff**

Run:

```powershell
git status --short
git diff --stat
```

Expected: only Phase 6G plan, media API/implementation/tests/docs, README, and changelog are changed.
