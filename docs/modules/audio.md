# nexus_audio

`nexus_audio` provides cross-platform audio input device enumeration and PCM audio frame capture. On Windows the backend uses WASAPI (Windows Core Audio API); on Linux the backend uses PulseAudio. Platform SDK headers are kept private — public headers only expose NexusKit types.

## Current API

- `nexus::audio::AudioDevice`: audio input device descriptor (id, name, channels, supported sample rates).
- `nexus::audio::AudioFrame`: captured PCM int16 audio frame (timestamp, data, sample rate, channels).
- `nexus::audio::AudioCaptureOptions`: capture configuration (device id, sample rate, channels).
- `nexus::audio::AudioCapturer`: move-only RAII audio capturer with static enumeration helpers.
- `AudioCapturer::create`: validates options and returns `nexus::Result<AudioCapturer>`.
- `AudioCapturer::enumerate_input_devices`: returns a list of available audio input devices.
- `AudioCapturer::default_input_device`: returns the system default input device.
- `AudioCapturer::start`: begins capturing, delivering `Result<AudioFrame>` via callback on an internal capture thread.
- `AudioCapturer::stop`: stops capturing (idempotent).
- `AudioCapturer::is_capturing`: true while actively capturing frames.
- `AudioCapturer::volume`: gets master volume level (0–100) for a device.
- `AudioCapturer::set_volume`: sets master volume level (0–100) for a device.
- `AudioCapturer::set_mute`: mutes or unmutes a device.

## Scope

The audio capture module supports enumerating input devices, selecting a device by id (or using the system default), and capturing PCM int16 audio frames delivered via callback. Capture runs on an internal background thread; callers receive frames on that thread. WASAPI provides volume and mute control via `IAudioEndpointVolume`. Playback and device hotplug events are planned follow-ups.

## Backend

On Windows the backend is WASAPI (`mmdeviceapi.h`, `audioclient.h`) using shared-mode `IAudioClient` with `IAudioCaptureClient` for polling-based capture. On Linux the backend is PulseAudio using `libpulse-simple`. These headers are included only from implementation files — users of `nexus_audio` depend on NexusKit headers.

## Diagnostics

Device enumeration, caputurer creation, and capture start/stop events write diagnostic messages through `nexus::log::write`. Install a default logger with `nexus::log::set_default_logger` to capture these events. Frame delivery does not log per-frame diagnostics.

## Error Model

- No supported backend → `StatusCode::kFailedPrecondition`
- Device not found → `StatusCode::kNotFound`
- Invalid options → `StatusCode::kInvalidArgument`
- Capture already running → `StatusCode::kFailedPrecondition`
- Backend API failure → `StatusCode::kUnavailable`
