#include "screen_frame_crop.h"
#include "screen_private.h"
#include "screen_x11_pixel_format.h"

#include <nexus/common/time.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#if defined(NEXUS_SCREEN_WITH_XRANDR)
#include <X11/extensions/Xrandr.h>
#endif

#include <algorithm>
#include <cstdint>
#include <exception>
#include <memory>
#include <string>
#include <utility>

namespace nexus::screen::detail {
namespace {

Status failed_precondition(std::string message) {
    return Status(StatusCode::kFailedPrecondition, std::move(message));
}

Status internal(std::string message) {
    return Status(StatusCode::kInternal, std::move(message));
}

struct DisplayCloser {
    void operator()(Display* display) const {
        if (display) {
            XCloseDisplay(display);
        }
    }
};

struct XImageDestroyer {
    void operator()(XImage* image) const {
        if (image) {
            XDestroyImage(image);
        }
    }
};

using DisplayHandle = std::unique_ptr<Display, DisplayCloser>;
using XImageHandle = std::unique_ptr<XImage, XImageDestroyer>;

class X11ScreenCapturerStorage final : public ScreenCapturerStorage {
public:
    explicit X11ScreenCapturerStorage(ScreenCaptureOptions options)
        : options_(options), display_(XOpenDisplay(nullptr)) {
        if (!display_) {
            setup_status_ = failed_precondition("X11 display is not available");
            return;
        }

        screen_ = DefaultScreen(display_.get());
        root_ = RootWindow(display_.get(), screen_);
        width_ = DisplayWidth(display_.get(), screen_);
        height_ = DisplayHeight(display_.get(), screen_);

        Visual* visual = DefaultVisual(display_.get(), screen_);
        if (!visual || visual->red_mask == 0 || visual->green_mask == 0 ||
            visual->blue_mask == 0 || width_ <= 0 || height_ <= 0) {
            setup_status_ = failed_precondition("X11 default screen is not truecolor");
            return;
        }

        pixel_format_.red_mask = visual->red_mask;
        pixel_format_.green_mask = visual->green_mask;
        pixel_format_.blue_mask = visual->blue_mask;
        setup_status_ = Status::ok_status();
    }

    bool is_available() const override {
        return setup_status_.ok() && display_ && root_ != 0 && width_ > 0 && height_ > 0;
    }

    Result<std::vector<ScreenDisplay>> displays() const override {
        if (!setup_status_.ok()) {
            return setup_status_;
        }
        if (!is_available()) {
            return failed_precondition("X11 screen capture is not available");
        }

        std::vector<ScreenDisplay> result;

#if defined(NEXUS_SCREEN_WITH_XRANDR)
        int event_base = 0;
        int error_base = 0;
        if (XRRQueryExtension(display_.get(), &event_base, &error_base)) {
            XRRScreenResources* resources =
                XRRGetScreenResources(display_.get(), root_);
            if (resources) {
                const RROutput primary_output =
                    XRRGetOutputPrimary(display_.get(), root_);

                for (int i = 0; i < resources->noutput; ++i) {
                    XRROutputInfo* output_info = XRRGetOutputInfo(
                        display_.get(), resources, resources->outputs[i]);
                    if (!output_info) {
                        continue;
                    }
                    if (output_info->connection != RR_Connected ||
                        output_info->crtc == 0) {
                        XRRFreeOutputInfo(output_info);
                        continue;
                    }

                    XRRCrtcInfo* crtc_info = XRRGetCrtcInfo(
                        display_.get(), resources, output_info->crtc);
                    if (!crtc_info) {
                        XRRFreeOutputInfo(output_info);
                        continue;
                    }

                    ScreenDisplay display;
                    display.id =
                        std::string("xrandr-") +
                        std::string(output_info->name,
                                    strnlen(output_info->name,
                                            static_cast<std::size_t>(
                                                output_info->nameLen)));
                    display.name = display.id.substr(strlen("xrandr-"));
                    display.x = crtc_info->x;
                    display.y = crtc_info->y;
                    display.width = static_cast<int>(crtc_info->width);
                    display.height = static_cast<int>(crtc_info->height);
                    display.primary =
                        (resources->outputs[i] == primary_output);

                    XRRFreeCrtcInfo(crtc_info);
                    XRRFreeOutputInfo(output_info);

                    result.push_back(std::move(display));
                }
                XRRFreeScreenResources(resources);

                if (!result.empty()) {
                    cached_displays_ = result;
                    return result;
                }
            }
        }
#endif

        cached_displays_.clear();

        ScreenDisplay display;
        display.id = kDefaultDisplayId;
        display.name = "X11 default screen";
        display.x = 0;
        display.y = 0;
        display.width = width_;
        display.height = height_;
        display.primary = true;
        result.push_back(std::move(display));
        cached_displays_ = result;
        return result;
    }

    Result<std::vector<ScreenWindow>> windows() const override {
        if (!setup_status_.ok()) {
            return setup_status_;
        }
        if (!is_available()) {
            return failed_precondition("X11 screen capture is not available");
        }

        Window root_return = 0;
        Window parent_return = 0;
        Window* children = nullptr;
        unsigned nchildren = 0;
        if (!XQueryTree(display_.get(), root_, &root_return, &parent_return, &children,
                        &nchildren) ||
            children == nullptr) {
            return internal("XQueryTree failed for root window");
        }

        Atom utf8_string_atom = XInternAtom(display_.get(), "UTF8_STRING", False);
        Atom net_active_window = XInternAtom(display_.get(), "_NET_ACTIVE_WINDOW", True);
        Atom net_wm_name = XInternAtom(display_.get(), "_NET_WM_NAME", True);
        Atom net_wm_pid = XInternAtom(display_.get(), "_NET_WM_PID", True);

        Window active_window = 0;
        if (net_active_window != None) {
            Atom actual_type = 0;
            int actual_format = 0;
            unsigned long nitems = 0;
            unsigned long bytes_after = 0;
            unsigned char* data = nullptr;
            if (XGetWindowProperty(display_.get(), root_, net_active_window, 0, 1, False,
                                   XA_WINDOW, &actual_type, &actual_format, &nitems, &bytes_after,
                                   &data) == Success &&
                data != nullptr) {
                if (nitems > 0 && actual_type == XA_WINDOW && actual_format == 32) {
                    active_window = *reinterpret_cast<Window*>(data);
                }
                XFree(data);
            }
        }

        auto read_window_property_string = [&](Window w, Atom property, Atom type) -> std::string {
            Atom actual_type = 0;
            int actual_format = 0;
            unsigned long nitems = 0;
            unsigned long bytes_after = 0;
            unsigned char* data = nullptr;
            if (XGetWindowProperty(display_.get(), w, property, 0, 1024, False, type, &actual_type,
                                   &actual_format, &nitems, &bytes_after,
                                   &data) == Success &&
                data != nullptr) {
                std::string result;
                if (nitems > 0 && actual_format == 8) {
                    result.assign(reinterpret_cast<char*>(data), nitems);
                }
                XFree(data);
                return result;
            }
            return {};
        };

        auto read_window_pid = [&](Window w) -> std::uint32_t {
            Atom actual_type = 0;
            int actual_format = 0;
            unsigned long nitems = 0;
            unsigned long bytes_after = 0;
            unsigned char* data = nullptr;
            if (XGetWindowProperty(display_.get(), w, net_wm_pid, 0, 1, False, XA_CARDINAL,
                                   &actual_type, &actual_format, &nitems, &bytes_after,
                                   &data) == Success &&
                data != nullptr) {
                std::uint32_t pid = 0;
                if (nitems > 0 && actual_format > 0) {
                    pid = static_cast<std::uint32_t>(*reinterpret_cast<unsigned long*>(data));
                }
                XFree(data);
                return pid;
            }
            return 0;
        };

        std::vector<ScreenWindow> windows;
        for (unsigned i = 0; i < nchildren; ++i) {
            Window w = children[i];
            XWindowAttributes attrs{};
            if (!XGetWindowAttributes(display_.get(), w, &attrs)) {
                continue;
            }
            if (attrs.map_state != IsViewable) {
                continue;
            }
            if (attrs.width <= 0 || attrs.height <= 0) {
                continue;
            }

            Window child = 0;
            int wx = 0;
            int wy = 0;
            XTranslateCoordinates(display_.get(), w, root_, 0, 0, &wx, &wy, &child);

            std::string title;
            if (net_wm_name != None) {
                title = read_window_property_string(w, net_wm_name, utf8_string_atom);
            }
            if (title.empty()) {
                char* wm_name = nullptr;
                if (XFetchName(display_.get(), w, &wm_name) && wm_name != nullptr) {
                    title = wm_name;
                    XFree(wm_name);
                }
            }
            if (title.empty()) {
                continue;
            }

            ScreenWindow window;
            window.id = "x11-window-" + std::to_string(w);
            window.process_id = net_wm_pid != None ? read_window_pid(w) : 0;
            window.title = std::move(title);
            window.active = (active_window != 0 && w == active_window);
            window.x = wx;
            window.y = wy;
            window.width = attrs.width;
            window.height = attrs.height;

            const int center_x = wx + attrs.width / 2;
            const int center_y = wy + attrs.height / 2;
            window.display_id = kDefaultDisplayId;
            for (const auto& d : cached_displays_) {
                if (center_x >= d.x && center_x < d.x + d.width &&
                    center_y >= d.y && center_y < d.y + d.height) {
                    window.display_id = d.id;
                    break;
                }
            }

            windows.push_back(std::move(window));
        }

        XFree(children);
        return windows;
    }

    Result<ScreenFrame> capture_primary() override {
        if (!is_available()) {
            return setup_status_.ok() ? failed_precondition("X11 screen capture is not available")
                                      : setup_status_;
        }

        const std::string* primary_id = &kDefaultDisplayId;
        for (const auto& d : cached_displays_) {
            if (d.primary) {
                primary_id = &d.id;
                break;
            }
        }
        return capture_display(*primary_id);
    }

    Result<ScreenFrame> capture_display(const std::string& display_id) override {
        if (!is_available()) {
            return setup_status_.ok() ? failed_precondition("X11 screen capture is not available")
                                      : setup_status_;
        }

        int capture_x = 0;
        int capture_y = 0;
        int capture_w = width_;
        int capture_h = height_;

        if (display_id == kDefaultDisplayId) {
            // Use full root-window dimensions.
        } else {
            const auto it = std::find_if(
                cached_displays_.begin(), cached_displays_.end(),
                [&](const ScreenDisplay& d) { return d.id == display_id; });
            if (it == cached_displays_.end()) {
                return Status(StatusCode::kNotFound,
                              "screen display was not found: " + display_id);
            }
            capture_x = it->x;
            capture_y = it->y;
            capture_w = it->width;
            capture_h = it->height;
        }

        XImageHandle image(XGetImage(display_.get(), root_, capture_x, capture_y,
                                     static_cast<unsigned>(capture_w),
                                     static_cast<unsigned>(capture_h),
                                     AllPlanes, ZPixmap));
        if (!image) {
            return internal("XGetImage failed for the screen display");
        }

        ScreenFrame frame;
        frame.width = capture_w;
        frame.height = capture_h;
        frame.pixel_format = ScreenPixelFormat::bgra;
        frame.timestamp_ms = nexus::common::steady_timestamp_ms();
        frame.data.resize(static_cast<std::size_t>(frame.width) *
                          static_cast<std::size_t>(frame.height) * 4u);

        for (int y = 0; y < frame.height; ++y) {
            for (int x = 0; x < frame.width; ++x) {
                const unsigned long pixel = XGetPixel(image.get(), x, y);
                const std::size_t index =
                    (static_cast<std::size_t>(y) * static_cast<std::size_t>(frame.width) +
                     static_cast<std::size_t>(x)) *
                    4u;
                x11_pixel_to_bgra(pixel, pixel_format_, frame.data.data() + index);
            }
        }

        return frame;
    }

    Result<ScreenFrame> capture_window(const std::string& window_id) override {
        if (!setup_status_.ok()) {
            return setup_status_;
        }
        if (!is_available()) {
            return failed_precondition("X11 screen capture is not available");
        }

        const std::string prefix = "x11-window-";
        if (window_id.size() <= prefix.size() ||
            window_id.substr(0, prefix.size()) != prefix) {
            return Status::invalid_argument("X11 window id format is invalid: " + window_id);
        }

        Window w = 0;
        try {
            w = static_cast<Window>(std::stoull(window_id.substr(prefix.size())));
        } catch (const std::exception&) {
            return Status::invalid_argument("X11 window id is not a valid XID: " + window_id);
        }

        XWindowAttributes attrs{};
        if (!XGetWindowAttributes(display_.get(), w, &attrs)) {
            return failed_precondition("X11 window is not accessible: " + window_id);
        }
        if (attrs.width <= 0 || attrs.height <= 0) {
            return failed_precondition("X11 window has invalid dimensions: " + window_id);
        }

        Window child = 0;
        int wx = 0;
        int wy = 0;
        XTranslateCoordinates(display_.get(), w, root_, 0, 0, &wx, &wy, &child);

        auto frame = capture_display(kDefaultDisplayId);
        if (!frame.ok()) {
            return frame.status();
        }

        ScreenCaptureRegion region;
        region.x = wx;
        region.y = wy;
        region.width = attrs.width;
        region.height = attrs.height;

        return crop_frame_bgra(frame.value(), region);
    }

    void close() override {
        display_.reset();
        root_ = 0;
        width_ = 0;
        height_ = 0;
        cached_displays_.clear();
        setup_status_ = failed_precondition("X11 screen capture backend is closed");
    }

private:
    static constexpr const char* kDefaultDisplayId = "x11-default";

    ScreenCaptureOptions options_;
    Status setup_status_;
    DisplayHandle display_;
    int screen_ = 0;
    Window root_ = 0;
    int width_ = 0;
    int height_ = 0;
    X11PixelFormat pixel_format_;
    mutable std::vector<ScreenDisplay> cached_displays_;
};

} // namespace

std::unique_ptr<ScreenCapturerStorage> create_screen_capturer_storage(
    const ScreenCaptureOptions& options) {
    return std::make_unique<X11ScreenCapturerStorage>(options);
}

ScreenBackendInfo query_screen_backend_info() {
    ScreenBackendInfo info;
    info.available = true;
    info.description = "Linux X11 backend for primary display BGRA capture, window enumeration and capture";
    return info;
}

} // namespace nexus::screen::detail
