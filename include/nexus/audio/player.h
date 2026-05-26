#pragma once

#include <functional>
#include <memory>

#include <nexus/audio/export.h>
#include <nexus/audio/types.h>
#include <nexus/core/result.h>
#include <nexus/core/status.h>

/// Audio playback: render PCM frames to output devices.
///
/// On Windows the backend uses WASAPI (Windows Core Audio API).
/// On Linux the backend uses PulseAudio.
/// Backend headers are private; public headers only expose NexusKit types.
namespace nexus::audio {

namespace detail {
class AudioPlayerStorage;
}

/// Options for `AudioPlayer::create`.
struct NEXUS_AUDIO_API AudioPlayerOptions {
    /// Device id to play to.  Empty string = system default device.
    std::string device_id;
    /// Desired sample rate in Hz.  0 = use device default.
    unsigned int sample_rate = 0;
    /// Desired channel count.  0 = use device default.
    unsigned int channels = 0;
};

/// Move-only RAII audio player.
///
/// Created via `create(options)`.  Start playback with `start(handler)`
/// which pulls `AudioFrame` data from the callback on an internal render
/// thread.  Return an empty `AudioFrame` (data empty) from the handler to
/// signal end-of-stream.
///
/// Copy is deleted; move transfers ownership.  A moved-from player is closed
/// and `is_playing()` returns false.
class NEXUS_AUDIO_API AudioPlayer {
public:
    /// Callback that returns the next audio frame to play.
    ///
    /// Called from an internal render thread.  Must return quickly to avoid
    /// glitches.  Return a default-constructed `AudioFrame` (empty data) to
    /// stop playback gracefully.
    using DataHandler = std::function<Result<AudioFrame>()>;

    /// Moves an audio player handle.
    AudioPlayer(AudioPlayer&& other) noexcept;
    /// Moves an audio player handle.
    AudioPlayer& operator=(AudioPlayer&& other) noexcept;
    /// Stops playback and releases the player if needed.
    ~AudioPlayer();

    /// Creates a player with the given options.
    ///
    /// @param options Playback configuration (device, sample rate, channels).
    /// @return An audio player on success.
    /// @retval kFailedPrecondition when no backend is available.
    /// @retval kNotFound when no output devices are found.
    /// @retval kInvalidArgument when options are invalid.
    static Result<AudioPlayer> create(AudioPlayerOptions options = {});

    /// Enumerates available audio output devices.
    ///
    /// @return A list of output device descriptors.
    /// @retval kFailedPrecondition when no backend is available.
    static Result<std::vector<AudioDevice>> enumerate_output_devices();

    /// Returns the system default audio output device.
    ///
    /// @retval kNotFound when no output devices are found.
    static Result<AudioDevice> default_output_device();

    /// Starts audio playback.  Frames are pulled via `handler` on an
    /// internal render thread.  Must not already be playing.
    ///
    /// @param handler Callback providing PCM int16 interleaved audio frames.
    /// @retval kFailedPrecondition when already playing.
    /// @retval kUnavailable on backend failure.
    Status start(DataHandler handler);

    /// Stops playback.  Idempotent; safe to call multiple times.
    Status stop();

    /// True while actively playing audio.
    bool is_playing() const;

private:
    explicit AudioPlayer(std::unique_ptr<detail::AudioPlayerStorage> storage);

    std::unique_ptr<detail::AudioPlayerStorage> storage_;
};

} // namespace nexus::audio
