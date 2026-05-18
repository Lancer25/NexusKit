// PulseAudio audio capture backend (Linux).

#ifdef NEXUS_AUDIO_HAS_PULSE

#include "capturer_internal.h"

#include <atomic>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <pulse/simple.h>
#include <pulse/error.h>
#include <pulse/context.h>
#include <pulse/introspect.h>
#include <pulse/mainloop.h>

#include <nexus/common/logging.h>
#include <nexus/common/thread.h>
#include <nexus/log/logger.h>

namespace nexus::audio {
namespace detail {

namespace {

std::string pa_strerror_wrap(int err) {
    return pa_strerror(err);
}

// Run a blocking PA mainloop operation.  Waits for the callback, then quits.
template <typename F>
void pulse_blocking_op(F&& setup) {
    pa_mainloop* ml = pa_mainloop_new();
    if (!ml) return;
    pa_mainloop_api* api = pa_mainloop_get_api(ml);

    pa_context* ctx = pa_context_new(api, "nexus_audio");
    if (!ctx) {
        pa_mainloop_free(ml);
        return;
    }

    int conn_err = 0;
    pa_context_set_state_callback(ctx, [](pa_context* c, void* userdata) {
        auto* err = static_cast<int*>(userdata);
        switch (pa_context_get_state(c)) {
        case PA_CONTEXT_READY:
            *err = 0;
            break;
        case PA_CONTEXT_FAILED:
        case PA_CONTEXT_TERMINATED:
            *err = 1;
            break;
        default:
            break;
        }
    }, &conn_err);

    if (pa_context_connect(ctx, nullptr, PA_CONTEXT_NOFLAGS, nullptr) < 0) {
        pa_context_unref(ctx);
        pa_mainloop_free(ml);
        return;
    }

    // Wait for context ready
    while (pa_mainloop_iterate(ml, 1, nullptr) >= 0) {
        if (pa_context_get_state(ctx) == PA_CONTEXT_READY) break;
        if (pa_context_get_state(ctx) == PA_CONTEXT_FAILED ||
            pa_context_get_state(ctx) == PA_CONTEXT_TERMINATED) break;
    }

    if (pa_context_get_state(ctx) != PA_CONTEXT_READY) {
        pa_context_unref(ctx);
        pa_mainloop_free(ml);
        return;
    }

    setup(ml, api, ctx);
    pa_context_unref(ctx);
    pa_mainloop_free(ml);
}

} // namespace

class PulseAudioCapturerStorage : public AudioCapturerStorage {
public:
    nexus::common::Thread thread;
    pa_simple* capture_handle = nullptr;
    AudioCapturer::DataHandler data_handler;
    unsigned int stream_channels = 2;
    unsigned int stream_rate = 44100;

    explicit PulseAudioCapturerStorage(AudioCaptureOptions opts)
        : AudioCapturerStorage(std::move(opts)) {}

    ~PulseAudioCapturerStorage() override { stop_capture(); }

    Status start_capture(AudioCapturer::DataHandler handler) override {
        if (capturing.load()) {
            return Status(StatusCode::kFailedPrecondition, "Already capturing");
        }

        unsigned int rate = options.sample_rate > 0 ? options.sample_rate : 44100;
        unsigned int ch = options.channels > 0 ? options.channels : 2;
        if (ch < 1) ch = 1;
        if (ch > 8) ch = 2;

        stream_rate = rate;
        stream_channels = ch;

        pa_sample_spec ss;
        ss.format = PA_SAMPLE_S16LE;
        ss.rate = rate;
        ss.channels = static_cast<uint8_t>(ch);

        const char* dev_name = options.device_id.empty() ? nullptr : options.device_id.c_str();

        int pa_err = 0;
        capture_handle = pa_simple_new(
            nullptr,                // Use default server
            "nexus_audio",          // Application name
            PA_STREAM_RECORD,       // Recording direction
            dev_name,               // Device (null = default)
            "Audio Capture",        // Stream description
            &ss,                    // Sample format
            nullptr,                // Channel map (null = default)
            nullptr,                // Buffering attributes (null = default)
            &pa_err);               // Error code

        if (!capture_handle) {
            return Status(StatusCode::kUnavailable,
                std::string("pa_simple_new failed: ") + pa_strerror_wrap(pa_err));
        }

        data_handler = std::move(handler);
        capturing.store(true);

        auto* self = this;
        thread.start([self] { self->capture_loop(); });

        nexus::common::diagnostic_log(log::Level::debug,
            "PulseAudio capture started: " + std::to_string(rate) + " Hz, "
            + std::to_string(ch) + " ch");
        return Status::ok_status();
    }

    Status stop_capture() override {
        if (!capturing.load()) return Status::ok_status();
        capturing.store(false);

        if (thread.is_running()) {
            thread.stop();
        }

        if (capture_handle) {
            pa_simple_free(capture_handle);
            capture_handle = nullptr;
        }

        data_handler = nullptr;
        return Status::ok_status();
    }

private:
    void capture_loop() {
        // Read in fixed-size chunks (~100 ms at 44.1 kHz stereo = 17640 bytes)
        const size_t bytes_per_frame = stream_channels * 2; // 16-bit
        const size_t frames_per_chunk = stream_rate / 10;    // ~100ms
        const size_t chunk_size = frames_per_chunk * bytes_per_frame;

        std::vector<uint8_t> buffer(chunk_size);
        int64_t timestamp_ms = 0;

        while (capturing.load()) {
            int pa_err = 0;
            if (pa_simple_read(capture_handle, buffer.data(), chunk_size, &pa_err) < 0) {
                std::string msg = "pa_simple_read failed: ";
                msg += pa_strerror_wrap(pa_err);
                data_handler(Status(StatusCode::kUnavailable, std::move(msg)));
                break;
            }

            AudioFrame frame;
            frame.timestamp_ms = timestamp_ms;
            timestamp_ms += 100;
            frame.sample_rate = stream_rate;
            frame.channels = stream_channels;
            frame.data = buffer; // Copy the chunk
            data_handler(std::move(frame));
        }
    }
};

// --- Platform-backed static helpers ---

Result<std::vector<AudioDevice>> pulse_enumerate_input_devices() {
    std::vector<AudioDevice> devices;

    pulse_blocking_op([&](pa_mainloop* ml, pa_mainloop_api* /*api*/, pa_context* ctx) {
        bool done = false;

        pa_context_get_source_info_list(ctx, [](pa_context*, const pa_source_info* info,
                                                  int eol, void* userdata) {
            auto* data = static_cast<std::pair<std::vector<AudioDevice>*, bool>*>(userdata);
            if (eol) {
                data->second = true;
                return;
            }
            if (!info) return;

            AudioDevice dev;
            dev.id = info->name ? info->name : "";
            dev.name = info->description ? info->description : dev.id;
            dev.channels = info->sample_spec.channels;
            dev.sample_rates = {info->sample_spec.rate};
            data->first->push_back(std::move(dev));
        }, std::pair<std::vector<AudioDevice>*, bool>{&devices, &done});

        // Run mainloop until the callback signals completion
        while (!done) {
            pa_mainloop_iterate(ml, 1, nullptr);
        }
    });

    return devices;
}

Result<AudioDevice> pulse_default_input_device() {
    AudioDevice result;

    pulse_blocking_op([&](pa_mainloop* ml, pa_mainloop_api* /*api*/, pa_context* ctx) {
        bool done = false;

        pa_context_get_server_info(ctx, [](pa_context*, const pa_server_info* info, void* userdata) {
            auto* pair_data = static_cast<std::pair<AudioDevice*, bool*>*>(userdata);
            if (!info) {
                *pair_data->second = true;
                return;
            }
            pair_data->first->id = info->default_source_name ? info->default_source_name : "";
            pair_data->first->name = pair_data->first->id;
            *pair_data->second = true;
        }, std::pair<AudioDevice*, bool*>{&result, &done});

        while (!done) {
            pa_mainloop_iterate(ml, 1, nullptr);
        }
    });

    if (result.id.empty()) {
        return Status(StatusCode::kNotFound, "No default PulseAudio source");
    }
    return result;
}

Result<std::unique_ptr<AudioCapturerStorage>>
pulse_create_storage(AudioCaptureOptions options) {
    return std::unique_ptr<AudioCapturerStorage>(
        std::make_unique<PulseAudioCapturerStorage>(std::move(options)));
}

Result<unsigned int> pulse_get_volume(const std::string& /*device_id*/) {
    return Status(StatusCode::kUnavailable, "PulseAudio volume control not yet implemented");
}

Status pulse_set_volume(const std::string& /*device_id*/, unsigned int /*level*/) {
    return Status(StatusCode::kUnavailable, "PulseAudio volume control not yet implemented");
}

Status pulse_set_mute(const std::string& /*device_id*/, bool /*mute*/) {
    return Status(StatusCode::kUnavailable, "PulseAudio volume control not yet implemented");
}

} // namespace detail
} // namespace nexus::audio

#endif // NEXUS_AUDIO_HAS_PULSE
