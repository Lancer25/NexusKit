#include <nexus/camera/capturer.h>

#include <iostream>
#include <string_view>
#include <vector>

namespace {

void print_status(std::string_view label, const nexus::Status& status) {
    std::cout << label << ": " << status.message() << '\n';
}

void print_device(std::string_view prefix, const nexus::camera::CameraDevice& device) {
    std::cout << prefix << " name=\"" << device.name << "\""
              << " id=\"" << device.id << "\""
              << " path=\"" << device.path << "\""
              << " vid=" << device.vid
              << " pid=" << device.pid << '\n';
}

void print_devices(
    std::string_view label,
    const nexus::Result<std::vector<nexus::camera::CameraDevice>>& result) {
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
    const nexus::Result<nexus::camera::CameraDevice>& result) {
    if (!result.ok()) {
        print_status(label, result.status());
        return;
    }

    print_device(label, result.value());
}

} // namespace

int main() {
    print_devices("camera devices", nexus::camera::CameraCapturer::enumerate_devices());
    print_default("default camera", nexus::camera::CameraCapturer::default_device());
    return 0;
}
