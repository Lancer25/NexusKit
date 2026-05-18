#include <nexus/screen/screen.h>

#include <iostream>
#include <optional>
#include <sstream>
#include <string>

namespace {

std::optional<nexus::screen::ScreenCaptureRegion> parse_region(const std::string& value) {
    std::stringstream stream(value);
    nexus::screen::ScreenCaptureRegion region;
    char first_comma = '\0';
    char second_comma = '\0';
    char third_comma = '\0';

    if (!(stream >> region.x >> first_comma >> region.y >> second_comma >> region.width >>
          third_comma >> region.height)) {
        return std::nullopt;
    }
    if (first_comma != ',' || second_comma != ',' || third_comma != ',') {
        return std::nullopt;
    }
    stream >> std::ws;
    if (!stream.eof()) {
        return std::nullopt;
    }
    if (region.width <= 0 || region.height <= 0 || region.x < 0 || region.y < 0) {
        return std::nullopt;
    }

    return region;
}

} // namespace

int main(int argc, char** argv) {
    std::string output_path = "screen_capture.ppm";
    std::optional<std::string> display_id;
    std::optional<std::string> window_id;
    std::optional<nexus::screen::ScreenCaptureRegion> region;
    nexus::screen::ScreenCaptureOptions options;
    bool list_displays = false;
    bool list_windows = false;

    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--cursor") {
            options.include_cursor = true;
        } else if (argument == "--list-displays") {
            list_displays = true;
        } else if (argument == "--list-windows") {
            list_windows = true;
        } else if (argument == "--display") {
            if (index + 1 >= argc) {
                std::cerr << "--display requires a display id\n";
                return 1;
            }
            display_id = argv[++index];
        } else if (argument == "--window") {
            if (index + 1 >= argc) {
                std::cerr << "--window requires a window id\n";
                return 1;
            }
            window_id = argv[++index];
        } else if (argument == "--region") {
            if (index + 1 >= argc) {
                std::cerr << "--region requires x,y,width,height\n";
                return 1;
            }
            region = parse_region(argv[++index]);
            if (!region) {
                std::cerr << "--region must be x,y,width,height with positive dimensions\n";
                return 1;
            }
        } else {
            output_path = argument;
        }
    }

    auto capturer = nexus::screen::ScreenCapturer::create(options);
    if (!capturer.ok()) {
        std::cerr << "Failed to create screen capturer: " << capturer.status().message() << '\n';
        return 1;
    }

    if (!capturer.value().is_available()) {
        const auto backend = nexus::screen::screen_backend_info();
        std::cerr << "Screen capture is not available: " << backend.description << '\n';
        return 1;
    }

    if (list_displays) {
        auto displays = capturer.value().displays();
        if (!displays.ok()) {
            std::cerr << "Failed to enumerate displays: " << displays.status().message() << '\n';
            return 1;
        }

        for (const auto& display : displays.value()) {
            std::cout << display.id << " \"" << display.name << "\" " << display.width << "x"
                      << display.height << "+" << display.x << "+" << display.y
                      << (display.primary ? " primary" : "") << '\n';
        }
        return 0;
    }

    if (list_windows) {
        auto windows = capturer.value().windows();
        if (!windows.ok()) {
            std::cerr << "Failed to enumerate windows: " << windows.status().message() << '\n';
            return 1;
        }

        for (const auto& window : windows.value()) {
            std::cout << window.id << " " << window.display_id << " pid=" << window.process_id
                      << (window.active ? " active" : "") << " \"" << window.title << "\" "
                      << window.width << "x" << window.height << "+" << window.x << "+"
                      << window.y << '\n';
        }
        return 0;
    }

    if (window_id && (display_id || region)) {
        std::cerr << "--window cannot be combined with --display or --region\n";
        return 1;
    }

    if (region && display_id) {
        region->display_id = *display_id;
    }

    auto frame =
        window_id ? capturer.value().capture_window(*window_id)
                  : (region ? capturer.value().capture_region(*region)
                            : (display_id ? capturer.value().capture_display(*display_id)
                                          : capturer.value().capture_primary()));
    if (!frame.ok()) {
        std::cerr << "Failed to capture screen: " << frame.status().message() << '\n';
        return 1;
    }

    const auto write_status = nexus::screen::write_ppm_file(output_path, frame.value());
    if (!write_status.ok()) {
        std::cerr << "Failed to write PPM output: " << write_status.message() << '\n';
        return 1;
    }

    std::cout << "Wrote " << output_path << " (" << frame.value().width << "x"
              << frame.value().height << ")\n";
    return 0;
}
