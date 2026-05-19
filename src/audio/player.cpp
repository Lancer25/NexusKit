#include "player_internal.h"

#include <utility>

#include <nexus/common/logging.h>
#include <nexus/log/logger.h>

namespace nexus::audio {

// --- AudioPlayer ---

AudioPlayer::AudioPlayer(AudioPlayer&& other) noexcept = default;
AudioPlayer& AudioPlayer::operator=(AudioPlayer&& other) noexcept = default;

AudioPlayer::~AudioPlayer() {
    stop();
}

Result<AudioPlayer> AudioPlayer::create(AudioPlayerOptions options) {
    nexus::common::diagnostic_log(log::Level::debug, "AudioPlayer::create");

#if defined(NEXUS_AUDIO_HAS_WASAPI)
    auto storage = detail::wasapi_create_player_storage(std::move(options));
#elif defined(NEXUS_AUDIO_HAS_PULSE)
    auto storage = detail::pulse_create_player_storage(std::move(options));
#else
    auto storage = Result<std::unique_ptr<detail::AudioPlayerStorage>>(
        Status(StatusCode::kFailedPrecondition, "No audio backend available"));
#endif
    if (!storage.ok()) {
        return storage.status();
    }
    return AudioPlayer(std::move(storage).value());
}

Result<std::vector<AudioDevice>> AudioPlayer::enumerate_output_devices() {
#if defined(NEXUS_AUDIO_HAS_WASAPI)
    return detail::wasapi_enumerate_output_devices();
#elif defined(NEXUS_AUDIO_HAS_PULSE)
    return detail::pulse_enumerate_output_devices();
#else
    return Status(StatusCode::kFailedPrecondition, "No audio backend available");
#endif
}

Result<AudioDevice> AudioPlayer::default_output_device() {
#if defined(NEXUS_AUDIO_HAS_WASAPI)
    return detail::wasapi_default_output_device();
#elif defined(NEXUS_AUDIO_HAS_PULSE)
    return detail::pulse_default_output_device();
#else
    return Status(StatusCode::kNotFound, "No audio backend available");
#endif
}

Status AudioPlayer::start(DataHandler handler) {
    if (!storage_) {
        return Status(StatusCode::kFailedPrecondition, "Player is closed");
    }
    return storage_->start_playback(std::move(handler));
}

Status AudioPlayer::stop() {
    if (storage_ && storage_->playing.load()) {
        return storage_->stop_playback();
    }
    return Status::ok_status();
}

bool AudioPlayer::is_playing() const {
    return storage_ && storage_->playing.load();
}

AudioPlayer::AudioPlayer(std::unique_ptr<detail::AudioPlayerStorage> storage)
    : storage_(std::move(storage)) {}

} // namespace nexus::audio
