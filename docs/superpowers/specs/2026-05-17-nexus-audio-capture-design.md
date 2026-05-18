# Design Spec: nexus_audio — Audio Capture

## Motivation

NexusKit lacks audio capture capability. Users need to enumerate audio input devices and capture raw PCM audio frames on Windows (WASAPI) and Linux (PulseAudio), following the same cross-platform, backend-private patterns as `nexus_screen` and `nexus_net`.

## Design Goals

1. Cross-platform audio input device enumeration (Windows WASAPI, Linux PulseAudio)
2. Audio capture from a selected device delivering PCM frames via callback
3. Backend libraries (WASAPI COM, PulseAudio C API) kept entirely private
4. Consistent with existing NexusKit patterns: `nexus::Result<T>`, `nexus::Status`, RAII, move-only handles

## Out of Scope (for initial phase)

- Audio playback (speaker output)
- Volume/mute control
- Default device switching
- Device hotplug event callbacks
- Audio loopback/monitor capture

## Public API

### `include/nexus/audio/types.h`

```cpp
namespace nexus::audio {

/// Audio device descriptor.
struct AudioDevice {
    /// Opaque backend id. Do not persist across runs.
    std::string id;
    /// Human-readable device name.
    std::string name;
    /// Maximum supported channel count.
    unsigned int channels = 0;
    /// Supported sample rates in Hz (may be empty = any rate).
    std::vector<unsigned int> sample_rates;
};

/// Captured audio frame.
struct AudioFrame {
    /// Monotonic capture timestamp in milliseconds.
    std::int64_t timestamp_ms = 0;
    /// PCM sample data (int16 interleaved).
    std::vector<std::uint8_t> data;
    /// Sample rate this frame was captured at.
    unsigned int sample_rate = 0;
    /// Channel count.
    unsigned int channels = 0;
};

/// Capture configuration.
struct AudioCaptureOptions {
    /// Device id to capture from. Empty = system default.
    std::string device_id;
    /// Desired sample rate (0 = device default).
    unsigned int sample_rate = 0;
    /// Desired channel count (0 = device default).
    unsigned int channels = 0;
};

} // namespace nexus::audio
```

### `include/nexus/audio/capturer.h`

```cpp
namespace nexus::audio {

class NEXUS_AUDIO_API AudioCapturer {
public:
    using DataHandler = std::function<void(Result<AudioFrame>)>;

    AudioCapturer(AudioCapturer&&) noexcept;
    AudioCapturer& operator=(AudioCapturer&&) noexcept;
    ~AudioCapturer();

    /// Creates a capturer with the given options.
    static Result<AudioCapturer> create(AudioCaptureOptions options = {});

    /// Enumerate available audio input devices.
    static Result<std::vector<AudioDevice>> enumerate_input_devices();

    /// Get the system default input device.
    static Result<AudioDevice> default_input_device();

    /// Start capturing. Handler fires from an internal capture thread.
    Status start(DataHandler handler);

    /// Stop capturing. Idempotent.
    Status stop();

    /// True while actively capturing.
    bool is_capturing() const;

private:
    explicit AudioCapturer(std::unique_ptr<detail::AudioCapturerStorage> storage);
    std::unique_ptr<detail::AudioCapturerStorage> storage_;
};

} // namespace nexus::audio
```

## Backend Strategy

### Windows: WASAPI

- Use Windows Core Audio API (`mmdeviceapi.h`, `audioclient.h`)
- COM-based: `IMMDeviceEnumerator` → `IMMDevice` → `IAudioClient` → `IAudioCaptureClient`
- Capture thread loops on `IAudioCaptureClient::GetBuffer` / `ReleaseBuffer`
- PCM int16 format, shared-mode stream

### Linux: PulseAudio

- Use PulseAudio simple API (`pulse/simple.h`) for capture
- `pa_simple_new()` with device name, sample spec, channel map
- Capture thread loops on `pa_simple_read()`
- PCM int16 format

### Build Integration

- New CMake option: `NEXUS_ENABLE_AUDIO` (default OFF, like nexus_usb/nexus_media)
- Windows: link against `ole32.lib` (COM), no additional SDK requirements
- Linux: link against `libpulse-simple`, found via `find_package(PulseAudio)` or pkg-config

## Module Layout

```
include/nexus/audio/
    export.h
    types.h          — AudioDevice, AudioFrame, AudioCaptureOptions
    capturer.h       — AudioCapturer class

src/audio/
    CMakeLists.txt
    capturer.cpp     — public methods, platform dispatch
    wasapi_capturer.cpp  — Windows WASAPI backend (detail::AudioCapturerStorage)
    pulse_capturer.cpp   — Linux PulseAudio backend (detail::AudioCapturerStorage)

tests/audio/
    audio_tests.cpp
```

## Error Model

- No supported backend → `StatusCode::kFailedPrecondition`
- Device not found → `StatusCode::kNotFound`
- Invalid options → `StatusCode::kInvalidArgument`
- Capture already running → `StatusCode::kFailedPrecondition`
- Platform API failure → `StatusCode::kUnavailable`
