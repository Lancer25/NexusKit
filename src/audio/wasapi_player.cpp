// WASAPI audio render backend (Windows).

#ifdef NEXUS_AUDIO_HAS_WASAPI

#include "player_internal.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <audioclient.h>
#include <endpointvolume.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys.h>

DEFINE_PROPERTYKEY(PKEY_Device_FriendlyName,
    0xa45c254e, 0xdf1c, 0x4efd,
    0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 14);

#include <nexus/common/logging.h>
#include <nexus/common/string.h>
#include <nexus/common/thread.h>
#include <nexus/log/logger.h>

namespace nexus::audio {
namespace detail {

namespace {

Status hresult_to_status(HRESULT hr, const char* operation) {
    std::string msg = std::string(operation) + " failed: 0x";
    char hex[16];
    snprintf(hex, sizeof(hex), "%08lX", static_cast<unsigned long>(hr));
    msg += hex;
    return Status(StatusCode::kUnavailable, msg);
}

class WasapiComGuard {
public:
    WasapiComGuard() { CoInitializeEx(nullptr, COINIT_MULTITHREADED); }
    ~WasapiComGuard() { CoUninitialize(); }
};

AudioDevice device_info(IMMDevice* dev) {
    AudioDevice info;

    LPWSTR wid = nullptr;
    if (SUCCEEDED(dev->GetId(&wid)) && wid) {
        info.id = nexus::common::wide_to_utf8(wid);
        CoTaskMemFree(wid);
    }

    IPropertyStore* props = nullptr;
    if (SUCCEEDED(dev->OpenPropertyStore(STGM_READ, &props))) {
        PROPVARIANT v;
        PropVariantInit(&v);
        if (SUCCEEDED(props->GetValue(PKEY_Device_FriendlyName, &v)) && v.pwszVal) {
            info.name = nexus::common::wide_to_utf8(v.pwszVal);
        }
        PropVariantClear(&v);
        props->Release();
    }

    IAudioClient* ac = nullptr;
    if (SUCCEEDED(dev->Activate(__uuidof(IAudioClient), CLSCTX_ALL,
                                nullptr, reinterpret_cast<void**>(&ac)))) {
        WAVEFORMATEX* fmt = nullptr;
        if (SUCCEEDED(ac->GetMixFormat(&fmt)) && fmt) {
            info.channels = fmt->nChannels;
            info.sample_rates = {fmt->nSamplesPerSec};
            CoTaskMemFree(fmt);
        }
        ac->Release();
    }

    return info;
}

IMMDevice* open_output_device(IMMDeviceEnumerator* enumerator,
                               const std::string& device_id) {
    if (device_id.empty()) {
        IMMDevice* dev = nullptr;
        HRESULT hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &dev);
        if (FAILED(hr)) return nullptr;
        return dev;
    }
    IMMDevice* dev = nullptr;
    HRESULT hr = enumerator->GetDevice(
        nexus::common::utf8_to_wide(device_id).c_str(), &dev);
    if (FAILED(hr)) return nullptr;
    return dev;
}

} // namespace

class WasapiAudioPlayerStorage : public AudioPlayerStorage {
public:
    nexus::common::Thread thread;

    IMMDeviceEnumerator* enumerator = nullptr;
    IAudioClient* audio_client = nullptr;
    IAudioRenderClient* render_client = nullptr;

    unsigned int stream_channels = 0;
    unsigned int stream_rate = 0;
    UINT32 buffer_frames = 0;

    AudioPlayer::DataHandler data_handler;

    explicit WasapiAudioPlayerStorage(AudioPlayerOptions opts)
        : AudioPlayerStorage(std::move(opts)) {}

    ~WasapiAudioPlayerStorage() override {
        stop_playback();
    }

    Status start_playback(AudioPlayer::DataHandler handler) override {
        if (playing.load()) {
            return Status(StatusCode::kFailedPrecondition, "Already playing");
        }

        WasapiComGuard com;

        HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                                       CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                                       reinterpret_cast<void**>(&enumerator));
        if (FAILED(hr)) {
            return hresult_to_status(hr, "CoCreateInstance MMDeviceEnumerator");
        }

        IMMDevice* device = open_output_device(enumerator, options.device_id);
        if (!device) {
            enumerator->Release();
            enumerator = nullptr;
            return Status(StatusCode::kNotFound, "Output device not found");
        }

        hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                              reinterpret_cast<void**>(&audio_client));
        device->Release();
        if (FAILED(hr)) {
            enumerator->Release();
            enumerator = nullptr;
            return hresult_to_status(hr, "Activate IAudioClient");
        }

        WAVEFORMATEX* mix_fmt = nullptr;
        hr = audio_client->GetMixFormat(&mix_fmt);
        if (FAILED(hr) || !mix_fmt) {
            audio_client->Release();
            audio_client = nullptr;
            enumerator->Release();
            enumerator = nullptr;
            return hresult_to_status(hr, "GetMixFormat");
        }

        unsigned int rate = options.sample_rate > 0
                                ? options.sample_rate
                                : mix_fmt->nSamplesPerSec;
        unsigned int ch = options.channels > 0
                              ? options.channels
                              : (mix_fmt->nChannels > 0 ? mix_fmt->nChannels : 2);

        stream_rate = rate;
        stream_channels = ch;

        if (ch < 1) ch = 1;
        if (ch > 2) ch = 2;
        if (rate < 8000) rate = 8000;

        WAVEFORMATEXTENSIBLE wfx = {};
        wfx.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        wfx.Format.nChannels = static_cast<WORD>(ch);
        wfx.Format.nSamplesPerSec = rate;
        wfx.Format.wBitsPerSample = 16;
        wfx.Format.nBlockAlign = static_cast<WORD>(ch * 2);
        wfx.Format.nAvgBytesPerSec = rate * ch * 2;
        wfx.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
        wfx.Samples.wValidBitsPerSample = 16;
        wfx.dwChannelMask = (ch == 1) ? SPEAKER_FRONT_CENTER
                                      : (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT);
        wfx.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;

        CoTaskMemFree(mix_fmt);

        REFERENCE_TIME buf_duration = 20000000; // 2 second buffer
        hr = audio_client->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                       0, buf_duration, 0,
                                       &wfx.Format, nullptr);
        if (FAILED(hr)) {
            audio_client->Release();
            audio_client = nullptr;
            enumerator->Release();
            enumerator = nullptr;
            return hresult_to_status(hr, "IAudioClient::Initialize");
        }

        hr = audio_client->GetBufferSize(&buffer_frames);
        if (FAILED(hr)) {
            audio_client->Release();
            audio_client = nullptr;
            enumerator->Release();
            enumerator = nullptr;
            return hresult_to_status(hr, "GetBufferSize");
        }

        hr = audio_client->GetService(__uuidof(IAudioRenderClient),
                                       reinterpret_cast<void**>(&render_client));
        if (FAILED(hr)) {
            audio_client->Release();
            audio_client = nullptr;
            enumerator->Release();
            enumerator = nullptr;
            return hresult_to_status(hr, "GetService IAudioRenderClient");
        }

        hr = audio_client->Start();
        if (FAILED(hr)) {
            render_client->Release();
            render_client = nullptr;
            audio_client->Release();
            audio_client = nullptr;
            enumerator->Release();
            enumerator = nullptr;
            return hresult_to_status(hr, "IAudioClient::Start");
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

        if (audio_client) {
            audio_client->Stop();
        }

        if (thread.is_running()) {
            thread.stop();
        }

        if (render_client) {
            render_client->Release();
            render_client = nullptr;
        }
        if (audio_client) {
            audio_client->Release();
            audio_client = nullptr;
        }
        if (enumerator) {
            enumerator->Release();
            enumerator = nullptr;
        }
        data_handler = nullptr;
        return Status::ok_status();
    }

private:
    void render_loop() {
        while (playing.load()) {
            UINT32 padding = 0;
            HRESULT hr = audio_client->GetCurrentPadding(&padding);
            if (FAILED(hr)) break;

            UINT32 frames_available = buffer_frames - padding;
            if (frames_available < stream_rate / 100) {
                // Less than 10 ms of buffer space — sleep briefly.
                Sleep(1);
                continue;
            }

            // Request a reasonable chunk (20 ms).
            UINT32 frames_to_write = stream_rate / 50;
            if (frames_to_write > frames_available) {
                frames_to_write = frames_available;
            }

            BYTE* data = nullptr;
            hr = render_client->GetBuffer(frames_to_write, &data);
            if (FAILED(hr)) break;

            auto frame_result = data_handler();
            if (!frame_result.ok() || frame_result.value().data.empty()) {
                // End-of-stream — fill with silence and stop.
                UINT32 bytes = frames_to_write * stream_channels * 2;
                memset(data, 0, bytes);
                render_client->ReleaseBuffer(frames_to_write, 0);
                playing.store(false);
                break;
            }

            auto& frame = frame_result.value();
            UINT32 src_bytes = static_cast<UINT32>(frame.data.size());
            UINT32 dst_bytes = frames_to_write * stream_channels * 2;
            UINT32 copy_bytes = src_bytes < dst_bytes ? src_bytes : dst_bytes;

            memcpy(data, frame.data.data(), copy_bytes);
            if (copy_bytes < dst_bytes) {
                memset(data + copy_bytes, 0, dst_bytes - copy_bytes);
            }

            render_client->ReleaseBuffer(frames_to_write, 0);
        }
    }
};

// --- Platform-backed static helpers ---

Result<std::vector<AudioDevice>> wasapi_enumerate_output_devices() {
    WasapiComGuard com;

    IMMDeviceEnumerator* enumerator = nullptr;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                                   CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                                   reinterpret_cast<void**>(&enumerator));
    if (FAILED(hr)) {
        return hresult_to_status(hr, "CoCreateInstance");
    }

    IMMDeviceCollection* collection = nullptr;
    hr = enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &collection);
    if (FAILED(hr)) {
        enumerator->Release();
        return hresult_to_status(hr, "EnumAudioEndpoints");
    }

    UINT count = 0;
    collection->GetCount(&count);

    std::vector<AudioDevice> devices;
    for (UINT i = 0; i < count; ++i) {
        IMMDevice* dev = nullptr;
        if (SUCCEEDED(collection->Item(i, &dev))) {
            devices.push_back(device_info(dev));
            dev->Release();
        }
    }

    collection->Release();
    enumerator->Release();
    return devices;
}

Result<AudioDevice> wasapi_default_output_device() {
    WasapiComGuard com;

    IMMDeviceEnumerator* enumerator = nullptr;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                                   CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                                   reinterpret_cast<void**>(&enumerator));
    if (FAILED(hr)) {
        return hresult_to_status(hr, "CoCreateInstance");
    }

    IMMDevice* dev = nullptr;
    hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &dev);
    enumerator->Release();
    if (FAILED(hr)) {
        return hresult_to_status(hr, "GetDefaultAudioEndpoint");
    }

    auto info = device_info(dev);
    dev->Release();
    return info;
}

Result<std::unique_ptr<AudioPlayerStorage>>
wasapi_create_player_storage(AudioPlayerOptions options) {
    return std::unique_ptr<AudioPlayerStorage>(
        std::make_unique<WasapiAudioPlayerStorage>(std::move(options)));
}

} // namespace detail
} // namespace nexus::audio

#endif // NEXUS_AUDIO_HAS_WASAPI
