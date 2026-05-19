#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include <nexus/audio/player.h>
#include <nexus/core/result.h>

namespace nexus::audio {
namespace detail {

/// Base class for platform-specific player storage.
class AudioPlayerStorage {
public:
    AudioPlayerOptions options;
    std::atomic<bool> playing{false};

    explicit AudioPlayerStorage(AudioPlayerOptions opts)
        : options(std::move(opts)) {}

    virtual ~AudioPlayerStorage() = default;

    virtual Status start_playback(AudioPlayer::DataHandler handler) = 0;
    virtual Status stop_playback() = 0;
};

// Platform backend factories — defined in per-platform TU.

#if defined(NEXUS_AUDIO_HAS_WASAPI)
Result<std::vector<AudioDevice>> wasapi_enumerate_output_devices();
Result<AudioDevice> wasapi_default_output_device();
Result<std::unique_ptr<AudioPlayerStorage>>
wasapi_create_player_storage(AudioPlayerOptions options);
#endif

#if defined(NEXUS_AUDIO_HAS_PULSE)
Result<std::vector<AudioDevice>> pulse_enumerate_output_devices();
Result<AudioDevice> pulse_default_output_device();
Result<std::unique_ptr<AudioPlayerStorage>>
pulse_create_player_storage(AudioPlayerOptions options);
#endif

} // namespace detail
} // namespace nexus::audio
