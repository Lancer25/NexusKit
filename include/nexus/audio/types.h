#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <nexus/audio/export.h>

/// Audio capture types: device descriptors, frames, and capture options.
namespace nexus::audio {

/// An audio input device descriptor.
struct NEXUS_AUDIO_API AudioDevice {
    /// Opaque backend identifier.  Do not persist across runs.
    std::string id;
    /// Human-readable device name.
    std::string name;
    /// Maximum channel count supported by this device.
    unsigned int channels = 0;
    /// Supported sample rates in Hz.  Empty = any rate accepted.
    std::vector<unsigned int> sample_rates;
};

/// A captured audio frame containing PCM int16 interleaved samples.
struct NEXUS_AUDIO_API AudioFrame {
    /// Monotonic capture timestamp in milliseconds.
    std::int64_t timestamp_ms = 0;
    /// PCM int16 interleaved sample data.
    std::vector<std::uint8_t> data;
    /// Sample rate in Hz this frame was captured at.
    unsigned int sample_rate = 0;
    /// Channel count.
    unsigned int channels = 0;
};

/// Audio capture configuration.
struct NEXUS_AUDIO_API AudioCaptureOptions {
    /// Device id to capture from.  Empty string = system default device.
    std::string device_id;
    /// Desired sample rate in Hz.  0 = use device default.
    unsigned int sample_rate = 0;
    /// Desired channel count.  0 = use device default (typically 1 for mono).
    unsigned int channels = 0;
};

} // namespace nexus::audio
