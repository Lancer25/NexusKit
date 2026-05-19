// PulseAudio playback backend (Linux).

#ifdef NEXUS_AUDIO_HAS_PULSE

#include "player_internal.h"

#include <atomic>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <pulse/simple.h>
#include <pulse/error.h>

#include <nexus/common/logging.h>
#include <nexus/common/thread.h>
#include <nexus/log/logger.h>

namespace nexus::audio {
namespace detail {

namespace {

constexpr unsigned int kDefaultSampleRate = 44100;
constexpr unsigned int kDefaultChannels = 2;

} // namespace

class PulseAudioPlayerStorage : public AudioPlayerStorage {
public:
    nexus::common::Thread thread;
    pa_simple* pa_stream = nullptr;
    unsigned int stream_rate = kDefaultSampleRate;
    unsigned int stream_channels = kDefaultChannels;

    AudioPlayer::DataHandler data_handler;

    explicit PulseAudioPlayerStorage(AudioPlayerOptions opts)
        : AudioPlayerStorage(std::move(opts)) {}

    ~PulseAudioPlayerStorage() override {
        stop_playback();
    }

    Status start_playback(AudioPlayer::DataHandler handler) override {
        if (playing.load()) {
            return Status(StatusCode::kFailedPrecondition, "Already playing");
        }

        unsigned int rate = options.sample_rate > 0
                                ? options.sample_rate
                                : kDefaultSampleRate;
        unsigned int ch = options.channels > 0
                              ? options.channels
                              : kDefaultChannels;

        stream_rate = rate;
        stream_channels = ch;

        pa_sample_spec ss;
        ss.format = PA_SAMPLE_S16LE;
        ss.rate = rate;
        ss.channels = static_cast<uint8_t>(ch);

        int error = 0;
        pa_stream = pa_simple_new(
            nullptr,                     // default server
            "NexusKit",                  // application name
            PA_STREAM_PLAYBACK,          // direction
            options.device_id.empty() ? nullptr : options.device_id.c_str(),
            "AudioPlayer",               // stream description
            &ss,                         // sample format
            nullptr,                     // channel map (default)
            nullptr,                     // buffering attributes (default)
            &error);

        if (!pa_stream) {
            return Status(StatusCode::kUnavailable,
                std::string("pa_simple_new failed: ") + pa_strerror(error));
        }

        data_handler = std::move(handler);
        playing.store(true);

        auto* self = this;
        thread.start([self] { self->render_loop(); });

        return Status::ok_status();
    }

    Status stop_playback() override {
        if (!playing.load()) return Status::ok_status();
        playing.store(false);

        if (thread.is_running()) {
            thread.stop();
        }

        if (pa_stream) {
            pa_simple_free(pa_stream);
            pa_stream = nullptr;
        }
        data_handler = nullptr;
        return Status::ok_status();
    }

private:
    void render_loop() {
        while (playing.load()) {
            auto frame_result = data_handler();
            if (!frame_result.ok() || frame_result.value().data.empty()) {
                playing.store(false);
                break;
            }

            auto& frame = frame_result.value();
            int error = 0;
            if (pa_simple_write(pa_stream, frame.data.data(),
                                frame.data.size(), &error) < 0) {
                break;
            }
        }
    }
};

// --- Platform-backed static helpers ---

Result<std::vector<AudioDevice>> pulse_enumerate_output_devices() {
    // PulseAudio simple API does not expose device enumeration.
    // Return a single default device.
    std::vector<AudioDevice> devices;
    AudioDevice dev;
    dev.id = "default";
    dev.name = "Default PulseAudio Output";
    dev.channels = 2;
    dev.sample_rates = {44100, 48000};
    devices.push_back(std::move(dev));
    return devices;
}

Result<AudioDevice> pulse_default_output_device() {
    AudioDevice dev;
    dev.id = "default";
    dev.name = "Default PulseAudio Output";
    dev.channels = 2;
    dev.sample_rates = {44100, 48000};
    return dev;
}

Result<std::unique_ptr<AudioPlayerStorage>>
pulse_create_player_storage(AudioPlayerOptions options) {
    return std::unique_ptr<AudioPlayerStorage>(
        std::make_unique<PulseAudioPlayerStorage>(std::move(options)));
}

} // namespace detail
} // namespace nexus::audio

#endif // NEXUS_AUDIO_HAS_PULSE
