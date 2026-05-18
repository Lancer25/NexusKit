// WASAPI audio capture backend (Windows).

#ifdef NEXUS_AUDIO_HAS_WASAPI

#include "capturer_internal.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <audioclient.h>
#include <endpointvolume.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys.h>

// Define PKEY_Device_FriendlyName to avoid pulling in
// functiondiscoverykeys_devpkey.h which conflicts with other SDK headers.
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

std::string wide_to_utf8(const wchar_t* wstr) {
    if (!wstr || !*wstr) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) return {};
    std::string result(len - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, result.data(), len, nullptr, nullptr);
    return result;
}

std::wstring utf8_to_wide(const std::string& str) {
    if (str.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    if (len <= 0) return {};
    std::wstring result(len - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, result.data(), len);
    return result;
}

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

// Extract basic device info from an IMMDevice.
// Falls back gracefully — a device that returns no name still gets an id entry.
AudioDevice device_info(IMMDevice* dev) {
    AudioDevice info;

    LPWSTR wid = nullptr;
    if (SUCCEEDED(dev->GetId(&wid)) && wid) {
        info.id = wide_to_utf8(wid);
        CoTaskMemFree(wid);
    }

    IPropertyStore* props = nullptr;
    if (SUCCEEDED(dev->OpenPropertyStore(STGM_READ, &props))) {
        PROPVARIANT v;
        PropVariantInit(&v);
        if (SUCCEEDED(props->GetValue(PKEY_Device_FriendlyName, &v)) && v.pwszVal) {
            info.name = wide_to_utf8(v.pwszVal);
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

} // namespace

class WasapiAudioCapturerStorage : public AudioCapturerStorage {
public:
    // Capture thread
    nexus::common::Thread thread;

    // WASAPI objects
    IMMDeviceEnumerator* enumerator = nullptr;
    IAudioClient* audio_client = nullptr;
    IAudioCaptureClient* capture_client = nullptr;

    // Actual stream format
    unsigned int stream_channels = 0;
    unsigned int stream_rate = 0;

    // Handler
    AudioCapturer::DataHandler data_handler;

    explicit WasapiAudioCapturerStorage(AudioCaptureOptions opts)
        : AudioCapturerStorage(std::move(opts)) {}

    ~WasapiAudioCapturerStorage() override {
        stop_capture();
    }

    Status start_capture(AudioCapturer::DataHandler handler) override {
        if (capturing.load()) {
            return Status(StatusCode::kFailedPrecondition, "Already capturing");
        }

        WasapiComGuard com;

        // Create enumerator
        HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                                       CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                                       reinterpret_cast<void**>(&enumerator));
        if (FAILED(hr)) {
            return hresult_to_status(hr, "CoCreateInstance MMDeviceEnumerator");
        }

        // Get target device
        IMMDevice* device = nullptr;
        if (!options.device_id.empty()) {
            auto wid = utf8_to_wide(options.device_id);
            hr = enumerator->GetDevice(wid.c_str(), &device);
            if (FAILED(hr)) {
                enumerator->Release();
                enumerator = nullptr;
                return hresult_to_status(hr, "GetDevice");
            }
        } else {
            hr = enumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &device);
            if (FAILED(hr)) {
                enumerator->Release();
                enumerator = nullptr;
                return hresult_to_status(hr, "GetDefaultAudioEndpoint");
            }
        }

        // Activate audio client
        hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                              reinterpret_cast<void**>(&audio_client));
        device->Release();
        if (FAILED(hr)) {
            enumerator->Release();
            enumerator = nullptr;
            return hresult_to_status(hr, "Activate IAudioClient");
        }

        // Determine format
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
                              : (mix_fmt->nChannels > 0 ? mix_fmt->nChannels : 1);

        stream_rate = rate;
        stream_channels = ch;

        // Clamp
        if (ch < 1) ch = 1;
        if (ch > 2) ch = 2;
        if (rate < 8000) rate = 8000;

        // Build WAVEFORMATEXTENSIBLE for PCM int16
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

        // Initialize
        REFERENCE_TIME buf_duration = 10000000; // 1 second buffer
        hr = audio_client->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                       0, // polling mode — no event callback
                                       buf_duration, 0,
                                       &wfx.Format, nullptr);
        if (FAILED(hr)) {
            audio_client->Release();
            audio_client = nullptr;
            enumerator->Release();
            enumerator = nullptr;
            return hresult_to_status(hr, "IAudioClient::Initialize");
        }

        // Get capture client
        hr = audio_client->GetService(__uuidof(IAudioCaptureClient),
                                       reinterpret_cast<void**>(&capture_client));
        if (FAILED(hr)) {
            audio_client->Release();
            audio_client = nullptr;
            enumerator->Release();
            enumerator = nullptr;
            return hresult_to_status(hr, "GetService IAudioCaptureClient");
        }

        // Start streaming
        hr = audio_client->Start();
        if (FAILED(hr)) {
            capture_client->Release();
            capture_client = nullptr;
            audio_client->Release();
            audio_client = nullptr;
            enumerator->Release();
            enumerator = nullptr;
            return hresult_to_status(hr, "IAudioClient::Start");
        }

        data_handler = std::move(handler);
        capturing.store(true);

        // Launch capture thread
        auto* self = this;
        thread.start([self] { self->capture_loop(); });

        return Status::ok_status();
    }

    Status stop_capture() override {
        if (!capturing.load()) return Status::ok_status();
        capturing.store(false);

        // Tell WASAPI to stop
        if (audio_client) {
            audio_client->Stop();
        }

        // Wait for capture thread to finish
        if (thread.is_running()) {
            thread.stop();
        }

        if (capture_client) {
            capture_client->Release();
            capture_client = nullptr;
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
    void capture_loop() {
        while (capturing.load()) {
            UINT32 packet_length = 0;
            HRESULT hr = capture_client->GetNextPacketSize(&packet_length);
            if (FAILED(hr)) {
                data_handler(hresult_to_status(hr, "GetNextPacketSize"));
                break;
            }

            while (packet_length > 0 && capturing.load()) {
                BYTE* data = nullptr;
                UINT32 frames_available = 0;
                DWORD flags = 0;
                UINT64 device_pos = 0;
                UINT64 qpc_pos = 0;

                hr = capture_client->GetBuffer(&data, &frames_available, &flags,
                                                &device_pos, &qpc_pos);
                if (FAILED(hr)) {
                    data_handler(hresult_to_status(hr, "GetBuffer"));
                    break;
                }

                if (data && frames_available > 0 && !(flags & AUDCLNT_BUFFERFLAGS_SILENT)) {
                    AudioFrame frame;
                    frame.timestamp_ms = static_cast<std::int64_t>(
                        qpc_pos / 10000); // QPC to ms approx
                    frame.sample_rate = stream_rate;
                    frame.channels = stream_channels;
                    UINT32 bytes_per_frame =
                        frames_available * stream_channels * 2; // 16-bit
                    frame.data.assign(data, data + bytes_per_frame);
                    data_handler(std::move(frame));
                }

                hr = capture_client->ReleaseBuffer(frames_available);
                if (FAILED(hr)) {
                    data_handler(hresult_to_status(hr, "ReleaseBuffer"));
                    break;
                }

                hr = capture_client->GetNextPacketSize(&packet_length);
                if (FAILED(hr)) {
                    data_handler(hresult_to_status(hr, "GetNextPacketSize"));
                    break;
                }
            }

            // Sleep briefly to avoid busy-waiting
            Sleep(10);
        }
    }
};

// --- Platform-backed static helpers ---

Result<std::vector<AudioDevice>> wasapi_enumerate_input_devices() {
    WasapiComGuard com;

    IMMDeviceEnumerator* enumerator = nullptr;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                                   CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                                   reinterpret_cast<void**>(&enumerator));
    if (FAILED(hr)) {
        return hresult_to_status(hr, "CoCreateInstance");
    }

    IMMDeviceCollection* collection = nullptr;
    hr = enumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &collection);
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

Result<AudioDevice> wasapi_default_input_device() {
    WasapiComGuard com;

    IMMDeviceEnumerator* enumerator = nullptr;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                                   CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                                   reinterpret_cast<void**>(&enumerator));
    if (FAILED(hr)) {
        return hresult_to_status(hr, "CoCreateInstance");
    }

    IMMDevice* dev = nullptr;
    hr = enumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &dev);
    enumerator->Release();
    if (FAILED(hr)) {
        return hresult_to_status(hr, "GetDefaultAudioEndpoint");
    }

    auto info = device_info(dev);
    dev->Release();
    return info;
}

Result<std::unique_ptr<AudioCapturerStorage>>
wasapi_create_storage(AudioCaptureOptions options) {
    return std::unique_ptr<AudioCapturerStorage>(
        std::make_unique<WasapiAudioCapturerStorage>(std::move(options)));
}

// --- Volume / mute control ---

namespace {

IMMDevice* open_audio_device(IMMDeviceEnumerator* enumerator,
                              const std::string& device_id) {
    if (device_id.empty()) {
        IMMDevice* dev = nullptr;
        HRESULT hr = enumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &dev);
        if (FAILED(hr)) return nullptr;
        return dev;
    }
    IMMDevice* dev = nullptr;
    HRESULT hr = enumerator->GetDevice(utf8_to_wide(device_id).c_str(), &dev);
    if (FAILED(hr)) return nullptr;
    return dev;
}

} // namespace

Result<unsigned int> wasapi_get_volume(const std::string& device_id) {
    WasapiComGuard com;

    IMMDeviceEnumerator* enumerator = nullptr;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                                   CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                                   reinterpret_cast<void**>(&enumerator));
    if (FAILED(hr)) return hresult_to_status(hr, "CoCreateInstance");

    IMMDevice* dev = open_audio_device(enumerator, device_id);
    enumerator->Release();
    if (!dev) return Status(StatusCode::kNotFound, "Audio device not found");

    IAudioEndpointVolume* vol = nullptr;
    hr = dev->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr,
                        reinterpret_cast<void**>(&vol));
    dev->Release();
    if (FAILED(hr)) return hresult_to_status(hr, "Activate IAudioEndpointVolume");

    float level = 0;
    hr = vol->GetMasterVolumeLevelScalar(&level);
    vol->Release();
    if (FAILED(hr)) return hresult_to_status(hr, "GetMasterVolumeLevelScalar");

    return static_cast<unsigned int>(level * 100.0f + 0.5f);
}

Status wasapi_set_volume(const std::string& device_id, unsigned int level) {
    if (level > 100) level = 100;

    WasapiComGuard com;

    IMMDeviceEnumerator* enumerator = nullptr;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                                   CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                                   reinterpret_cast<void**>(&enumerator));
    if (FAILED(hr)) return hresult_to_status(hr, "CoCreateInstance");

    IMMDevice* dev = open_audio_device(enumerator, device_id);
    enumerator->Release();
    if (!dev) return Status(StatusCode::kNotFound, "Audio device not found");

    IAudioEndpointVolume* vol = nullptr;
    hr = dev->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr,
                        reinterpret_cast<void**>(&vol));
    dev->Release();
    if (FAILED(hr)) return hresult_to_status(hr, "Activate IAudioEndpointVolume");

    hr = vol->SetMasterVolumeLevelScalar(level / 100.0f, nullptr);
    vol->Release();
    if (FAILED(hr)) return hresult_to_status(hr, "SetMasterVolumeLevelScalar");

    return Status::ok_status();
}

Status wasapi_set_mute(const std::string& device_id, bool mute) {
    WasapiComGuard com;

    IMMDeviceEnumerator* enumerator = nullptr;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                                   CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                                   reinterpret_cast<void**>(&enumerator));
    if (FAILED(hr)) return hresult_to_status(hr, "CoCreateInstance");

    IMMDevice* dev = open_audio_device(enumerator, device_id);
    enumerator->Release();
    if (!dev) return Status(StatusCode::kNotFound, "Audio device not found");

    IAudioEndpointVolume* vol = nullptr;
    hr = dev->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr,
                        reinterpret_cast<void**>(&vol));
    dev->Release();
    if (FAILED(hr)) return hresult_to_status(hr, "Activate IAudioEndpointVolume");

    hr = vol->SetMute(mute ? TRUE : FALSE, nullptr);
    vol->Release();
    if (FAILED(hr)) return hresult_to_status(hr, "SetMute");

    return Status::ok_status();
}

} // namespace detail
} // namespace nexus::audio

#endif // NEXUS_AUDIO_HAS_WASAPI
