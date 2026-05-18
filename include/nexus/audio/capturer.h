#pragma once

#include <functional>
#include <memory>

#include <nexus/audio/export.h>
#include <nexus/audio/types.h>
#include <nexus/core/result.h>
#include <nexus/core/status.h>

/// Audio capture: enumerate input devices and capture PCM frames.
///
/// On Windows the backend uses WASAPI (Windows Core Audio API).
/// On Linux the backend uses PulseAudio.
/// Backend headers are private; public headers only expose NexusKit types.
namespace nexus::audio {

namespace detail {
class AudioCapturerStorage;
}

/// Move-only RAII audio capturer.
///
/// Created via `create(options)`.  Start capturing with `start(handler)`
/// which delivers `Result<AudioFrame>` on an internal capture thread.
class NEXUS_AUDIO_API AudioCapturer {
public:
    /// Callback type for captured audio frames.
    using DataHandler = std::function<void(Result<AudioFrame>)>;

    AudioCapturer(AudioCapturer&& other) noexcept;
    AudioCapturer& operator=(AudioCapturer&& other) noexcept;
    ~AudioCapturer();

    /// Creates a capturer with the given options.
    ///
    /// @param options Capture configuration (device, sample rate, channels).
    /// @return An audio capturer on success.
    /// @retval kFailedPrecondition when no backend is available.
    /// @retval kInvalidArgument when options are invalid.
    static Result<AudioCapturer> create(AudioCaptureOptions options = {});

    /// Enumerates available audio input devices.
    ///
    /// @return A list of input device descriptors.
    /// @retval kFailedPrecondition when no backend is available.
    static Result<std::vector<AudioDevice>> enumerate_input_devices();

    /// Returns the system default audio input device.
    static Result<AudioDevice> default_input_device();

    /// Starts audio capture.  Frames are delivered via `handler` on an
    /// internal capture thread.  Must not already be capturing.
    ///
    /// @param handler Callback receiving each captured audio frame.
    /// @retval kFailedPrecondition when already capturing.
    /// @retval kUnavailable on backend failure.
    Status start(DataHandler handler);

    /// Stops capturing.  Idempotent — safe to call multiple times.
    Status stop();

    /// True while actively capturing frames.
    bool is_capturing() const;

    /// Gets the master volume level of a device (0–100).
    ///
    /// @param device_id Device id to query. Empty = system default input.
    /// @retval kNotFound when the device is not found.
    static Result<unsigned int> volume(const std::string& device_id = {});

    /// Sets the master volume level of a device (0–100).
    ///
    /// @param device_id Device id. Empty = system default input.
    /// @param level Volume level 0–100.
    /// @retval kNotFound when the device is not found.
    static Status set_volume(const std::string& device_id, unsigned int level);

    /// Mutes or unmutes a device.
    ///
    /// @param device_id Device id. Empty = system default input.
    /// @param mute True to mute, false to unmute.
    /// @retval kNotFound when the device is not found.
    static Status set_mute(const std::string& device_id, bool mute);

private:
    explicit AudioCapturer(std::unique_ptr<detail::AudioCapturerStorage> storage);

    std::unique_ptr<detail::AudioCapturerStorage> storage_;
};

} // namespace nexus::audio
