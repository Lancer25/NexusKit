#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include <nexus/audio/capturer.h>
#include <nexus/core/result.h>

namespace nexus::audio {
namespace detail {

/// Base class for platform-specific capturer storage.
class AudioCapturerStorage {
public:
    AudioCaptureOptions options;
    std::atomic<bool> capturing{false};

    explicit AudioCapturerStorage(AudioCaptureOptions opts)
        : options(std::move(opts)) {}

    virtual ~AudioCapturerStorage() = default;

    virtual Status start_capture(AudioCapturer::DataHandler handler) = 0;
    virtual Status stop_capture() = 0;
};

// Platform backend factories — defined in per-platform TU.
// Returns nullptr-or-error when the backend is unavailable.

#if defined(NEXUS_AUDIO_HAS_WASAPI)
Result<std::vector<AudioDevice>> wasapi_enumerate_input_devices();
Result<AudioDevice> wasapi_default_input_device();
Result<std::unique_ptr<AudioCapturerStorage>>
wasapi_create_storage(AudioCaptureOptions options);
Result<unsigned int> wasapi_get_volume(const std::string& device_id);
Status wasapi_set_volume(const std::string& device_id, unsigned int level);
Status wasapi_set_mute(const std::string& device_id, bool mute);
#endif

#if defined(NEXUS_AUDIO_HAS_PULSE)
Result<std::vector<AudioDevice>> pulse_enumerate_input_devices();
Result<AudioDevice> pulse_default_input_device();
Result<std::unique_ptr<AudioCapturerStorage>>
pulse_create_storage(AudioCaptureOptions options);
Result<unsigned int> pulse_get_volume(const std::string& device_id);
Status pulse_set_volume(const std::string& device_id, unsigned int level);
Status pulse_set_mute(const std::string& device_id, bool mute);
#endif

} // namespace detail
} // namespace nexus::audio
