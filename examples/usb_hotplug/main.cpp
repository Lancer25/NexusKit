#include <nexus/usb/usb.h>

#include <chrono>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <string>
#include <thread>

namespace {

struct Options {
    bool help = false;
    bool list = false;
    bool watch = false;
    int seconds = 10;
};

void print_usage() {
    std::cout
        << "Usage: nexus_example_usb_hotplug [--list] [--watch] [--seconds N]\n"
        << "  --list       List current USB/HID devices\n"
        << "  --watch      Watch USB device arrival/removal events\n"
        << "  --seconds N  Watch duration in seconds (default: 10)\n";
}

bool parse_int(const std::string& value, int& result) {
    char* end = nullptr;
    const auto parsed = std::strtol(value.c_str(), &end, 10);
    if (end == value.c_str() || *end != '\0' || parsed < 0 || parsed > 86400) {
        return false;
    }
    result = static_cast<int>(parsed);
    return true;
}

bool parse_options(int argc, char** argv, Options& options) {
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--help" || argument == "-h") {
            options.help = true;
        } else if (argument == "--list") {
            options.list = true;
        } else if (argument == "--watch") {
            options.watch = true;
        } else if (argument == "--seconds") {
            if (index + 1 >= argc || !parse_int(argv[++index], options.seconds)) {
                std::cerr << "--seconds requires a value from 0 to 86400\n";
                return false;
            }
        } else {
            std::cerr << "Unknown argument: " << argument << '\n';
            print_usage();
            return false;
        }
    }

    if (!options.list && !options.watch) {
        options.list = true;
    }
    return true;
}

std::string hex_id(std::uint16_t value) {
    constexpr char digits[] = "0123456789abcdef";
    std::string output = "0x0000";
    output[2] = digits[(value >> 12) & 0x0f];
    output[3] = digits[(value >> 8) & 0x0f];
    output[4] = digits[(value >> 4) & 0x0f];
    output[5] = digits[value & 0x0f];
    return output;
}

void print_device(const nexus::usb::UsbDeviceInfo& device) {
    std::cout << "vid=" << hex_id(device.vendor_id)
              << " pid=" << hex_id(device.product_id);
    if (!device.product.empty()) {
        std::cout << " product=\"" << device.product << "\"";
    }
    if (!device.serial_number.empty()) {
        std::cout << " serial=\"" << device.serial_number << "\"";
    }
    if (!device.path.empty()) {
        std::cout << " path=\"" << device.path << "\"";
    }
    std::cout << '\n';
}

int list_devices() {
    auto devices = nexus::usb::enumerate_devices();
    if (!devices.ok()) {
        std::cerr << "USB enumeration failed: " << devices.status().message() << '\n';
        return 1;
    }

    std::cout << "USB devices: " << devices.value().size() << '\n';
    for (const auto& device : devices.value()) {
        print_device(device);
    }
    return 0;
}

int watch_devices(int seconds) {
    auto monitor = nexus::usb::UsbHotplugMonitor::start(
        [](const nexus::usb::UsbHotplugEvent& event) {
            std::cout << (event.arrived ? "arrived " : "removed ");
            print_device(event.device);
        });
    if (!monitor.ok()) {
        std::cerr << "USB hotplug monitor failed: " << monitor.status().message() << '\n';
        return 1;
    }

    std::cout << "Watching USB hotplug events for " << seconds << " second(s)\n";
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
    const auto stop_status = monitor.value().stop();
    if (!stop_status.ok()) {
        std::cerr << "USB hotplug monitor stop failed: " << stop_status.message() << '\n';
        return 1;
    }
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    Options options;
    if (!parse_options(argc, argv, options)) {
        return 1;
    }
    if (options.help) {
        print_usage();
        return 0;
    }

    if (options.list && list_devices() != 0) {
        return 1;
    }
    if (options.watch && watch_devices(options.seconds) != 0) {
        return 1;
    }
    return 0;
}
