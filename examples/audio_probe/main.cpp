#include <nexus/audio/capturer.h>
#include <nexus/audio/player.h>

#include <cstddef>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

void print_status(std::string_view label, const nexus::Status& status) {
    std::cout << label << ": " << status.message() << '\n';
}

void print_rates(const std::vector<unsigned int>& rates) {
    if (rates.empty()) {
        std::cout << " rates=any";
        return;
    }

    std::cout << " rates=";
    for (std::size_t i = 0; i < rates.size(); ++i) {
        if (i > 0) {
            std::cout << ',';
        }
        std::cout << rates[i];
    }
}

void print_device(std::string_view prefix, const nexus::audio::AudioDevice& device) {
    std::cout << prefix << " name=\"" << device.name << "\""
              << " id=\"" << device.id << "\""
              << " channels=" << device.channels;
    print_rates(device.sample_rates);
    std::cout << '\n';
}

void print_devices(
    std::string_view label,
    const nexus::Result<std::vector<nexus::audio::AudioDevice>>& result) {
    if (!result.ok()) {
        print_status(label, result.status());
        return;
    }

    std::cout << label << ": " << result.value().size() << '\n';
    for (const auto& device : result.value()) {
        print_device("  device", device);
    }
}

void print_default(
    std::string_view label,
    const nexus::Result<nexus::audio::AudioDevice>& result) {
    if (!result.ok()) {
        print_status(label, result.status());
        return;
    }

    print_device(label, result.value());
}

} // namespace

int main() {
    print_devices("input devices", nexus::audio::AudioCapturer::enumerate_input_devices());
    print_default("default input", nexus::audio::AudioCapturer::default_input_device());
    print_devices("output devices", nexus::audio::AudioPlayer::enumerate_output_devices());
    print_default("default output", nexus::audio::AudioPlayer::default_output_device());
    return 0;
}
