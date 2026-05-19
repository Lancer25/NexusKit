# nexus_audio

`nexus_audio` provides cross-platform audio input device enumeration, PCM audio frame capture, and PCM audio playback. On Windows the backend uses WASAPI (Windows Core Audio API); on Linux the backend uses PulseAudio. Platform SDK headers are kept private — public headers only expose NexusKit types.

## Current API

### Capture

- `nexus::audio::AudioDevice`: audio device descriptor (id, name, channels, supported sample rates).
- `nexus::audio::AudioFrame`: PCM int16 audio frame (timestamp, data, sample rate, channels).
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

### Playback

- `nexus::audio::AudioPlayerOptions`: playback configuration (device id, sample rate, channels).
- `nexus::audio::AudioPlayer`: move-only RAII audio player with static enumeration helpers.
- `AudioPlayer::create`: validates options and returns `nexus::Result<AudioPlayer>`.
- `AudioPlayer::enumerate_output_devices`: returns a list of available audio output devices.
- `AudioPlayer::default_output_device`: returns the system default output device.
- `AudioPlayer::start`: begins playback, pulling `Result<AudioFrame>` via callback on an internal render thread. Return an empty `AudioFrame` (data empty) to signal end-of-stream.
- `AudioPlayer::stop`: stops playback (idempotent).
- `AudioPlayer::is_playing`: true while actively playing audio.

## Scope

The capture module supports enumerating input devices, selecting a device by id (or using the system default), and capturing PCM int16 audio frames delivered via callback. The playback module supports enumerating output devices, selecting a device by id (or using the system default), and rendering PCM int16 audio frames pulled via callback on an internal render thread. Callbacks are invoked on internal background threads; callers must return quickly to avoid audio glitches. WASAPI provides volume and mute control via `IAudioEndpointVolume`. Device hotplug events are a planned follow-up.

## Backend

On Windows the backend is WASAPI (`mmdeviceapi.h`, `audioclient.h`) using shared-mode `IAudioClient` with `IAudioCaptureClient` for capture and `IAudioRenderClient` for playback. On Linux the backend is PulseAudio using `libpulse-simple`. These headers are included only from implementation files — users of `nexus_audio` depend on NexusKit headers.

## Diagnostics

Device enumeration, capturer/player creation, and capture/playback start/stop events write diagnostic messages through `nexus::log::write`. Install a default logger with `nexus::log::set_default_logger` to capture these events. Frame delivery does not log per-frame diagnostics.

## Error Model

- No supported backend → `StatusCode::kFailedPrecondition`
- Device not found → `StatusCode::kNotFound`
- Invalid options → `StatusCode::kInvalidArgument`
- Capture/playback already running → `StatusCode::kFailedPrecondition`
- Backend API failure → `StatusCode::kUnavailable`
- End of stream (playback) → return empty `AudioFrame` from callback
