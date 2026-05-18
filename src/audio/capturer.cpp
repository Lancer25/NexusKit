#include "capturer_internal.h"

#include <utility>

#include <nexus/common/logging.h>
#include <nexus/log/logger.h>

namespace nexus::audio {

// --- AudioCapturer ---

AudioCapturer::AudioCapturer(AudioCapturer&& other) noexcept = default;
AudioCapturer& AudioCapturer::operator=(AudioCapturer&& other) noexcept = default;

AudioCapturer::~AudioCapturer() {
    stop();
}

Result<AudioCapturer> AudioCapturer::create(AudioCaptureOptions options) {
    nexus::common::diagnostic_log(log::Level::debug, "AudioCapturer::create");

#if defined(NEXUS_AUDIO_HAS_WASAPI)
    auto storage = detail::wasapi_create_storage(std::move(options));
#elif defined(NEXUS_AUDIO_HAS_PULSE)
    auto storage = detail::pulse_create_storage(std::move(options));
#else
    auto storage = Result<std::unique_ptr<detail::AudioCapturerStorage>>(
        Status(StatusCode::kFailedPrecondition, "No audio backend available"));
#endif
    if (!storage.ok()) {
        return storage.status();
    }
    return AudioCapturer(std::move(storage).value());
}

Result<std::vector<AudioDevice>> AudioCapturer::enumerate_input_devices() {
#if defined(NEXUS_AUDIO_HAS_WASAPI)
    return detail::wasapi_enumerate_input_devices();
#elif defined(NEXUS_AUDIO_HAS_PULSE)
    return detail::pulse_enumerate_input_devices();
#else
    return Status(StatusCode::kFailedPrecondition, "No audio backend available");
#endif
}

Result<AudioDevice> AudioCapturer::default_input_device() {
#if defined(NEXUS_AUDIO_HAS_WASAPI)
    return detail::wasapi_default_input_device();
#elif defined(NEXUS_AUDIO_HAS_PULSE)
    return detail::pulse_default_input_device();
#else
    return Status(StatusCode::kNotFound, "No audio backend available");
#endif
}

Status AudioCapturer::start(DataHandler handler) {
    if (!storage_) {
        return Status(StatusCode::kFailedPrecondition, "Capturer is closed");
    }
    return storage_->start_capture(std::move(handler));
}

Status AudioCapturer::stop() {
    if (storage_ && storage_->capturing.load()) {
        return storage_->stop_capture();
    }
    return Status::ok_status();
}

bool AudioCapturer::is_capturing() const {
    return storage_ && storage_->capturing.load();
}

Result<unsigned int> AudioCapturer::volume(const std::string& device_id) {
#if defined(NEXUS_AUDIO_HAS_WASAPI)
    return detail::wasapi_get_volume(device_id);
#elif defined(NEXUS_AUDIO_HAS_PULSE)
    return detail::pulse_get_volume(device_id);
#else
    return Status(StatusCode::kFailedPrecondition, "No audio backend available");
#endif
}

Status AudioCapturer::set_volume(const std::string& device_id, unsigned int level) {
#if defined(NEXUS_AUDIO_HAS_WASAPI)
    return detail::wasapi_set_volume(device_id, level);
#elif defined(NEXUS_AUDIO_HAS_PULSE)
    return detail::pulse_set_volume(device_id, level);
#else
    return Status(StatusCode::kFailedPrecondition, "No audio backend available");
#endif
}

Status AudioCapturer::set_mute(const std::string& device_id, bool mute) {
#if defined(NEXUS_AUDIO_HAS_WASAPI)
    return detail::wasapi_set_mute(device_id, mute);
#elif defined(NEXUS_AUDIO_HAS_PULSE)
    return detail::pulse_set_mute(device_id, mute);
#else
    return Status(StatusCode::kFailedPrecondition, "No audio backend available");
#endif
}

AudioCapturer::AudioCapturer(std::unique_ptr<detail::AudioCapturerStorage> storage)
    : storage_(std::move(storage)) {}

} // namespace nexus::audio
