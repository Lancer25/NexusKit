// libuvc camera capture backend (Windows).

#ifdef NEXUS_CAMERA_HAS_LIBUVC

#include "capturer_internal.h"

#include <atomic>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <libuvc/libuvc.h>

#include <nexus/common/logging.h>
#include <nexus/common/thread.h>
#include <nexus/log/logger.h>

namespace nexus::camera {
namespace detail {

namespace {

PixelFormat uvc_to_nexus_pixel_format(uvc_frame_format format) {
    switch (format) {
    case UVC_FRAME_FORMAT_MJPEG:
        return PixelFormat::kMJPEG;
    case UVC_FRAME_FORMAT_YUYV:
        return PixelFormat::kYUYV;
    default:
        return PixelFormat::kUnknown;
    }
}

uvc_frame_format nexus_to_uvc_format(PixelFormat format) {
    switch (format) {
    case PixelFormat::kMJPEG:
        return UVC_FRAME_FORMAT_MJPEG;
    case PixelFormat::kYUYV:
        return UVC_FRAME_FORMAT_YUYV;
    default:
        return UVC_FRAME_FORMAT_MJPEG;
    }
}

std::int64_t timeval_to_ms(const timeval& tv) {
    return static_cast<std::int64_t>(tv.tv_sec) * 1000 +
           static_cast<std::int64_t>(tv.tv_usec) / 1000;
}

} // namespace

class UvcCameraCapturerStorage : public CameraCapturerStorage {
public:
    // libuvc handles — must be released in reverse order
    uvc_context_t* ctx = nullptr;
    uvc_device_t* dev = nullptr;
    uvc_device_handle_t* devh = nullptr;

    // Negotiated format
    unsigned int actual_width = 0;
    unsigned int actual_height = 0;
    PixelFormat pixel_format = PixelFormat::kUnknown;

    // Handler
    CameraCapturer::DataHandler data_handler;

    explicit UvcCameraCapturerStorage(CameraCaptureOptions opts)
        : CameraCapturerStorage(std::move(opts)) {}

    ~UvcCameraCapturerStorage() override {
        stop_capture();
    }

    Status start_capture(CameraCapturer::DataHandler handler) override {
        if (capturing.load()) {
            return Status(StatusCode::kFailedPrecondition, "Already capturing");
        }

        // Initialize UVC context
        uvc_error_t err = uvc_init(&ctx, nullptr);
        if (err != UVC_SUCCESS) {
            return Status(StatusCode::kUnavailable, std::string("uvc_init failed: ") + uvc_strerror(err));
        }

        // Find devices
        uvc_device_t** devices = nullptr;
        err = uvc_find_devices(ctx, &devices, 0, 0, nullptr);
        if (err != UVC_SUCCESS) {
            uvc_exit(ctx);
            ctx = nullptr;
            return Status(StatusCode::kUnavailable, std::string("uvc_find_devices failed: ") + uvc_strerror(err));
        }

        if (!devices || !devices[0]) {
            if (devices) uvc_free_device_list(devices, 1);
            uvc_exit(ctx);
            ctx = nullptr;
            return Status(StatusCode::kNotFound, "No UVC camera devices found");
        }

        // Select target device
        if (!options.device_id.empty()) {
            int idx = 0;
            while (devices[idx]) {
                uvc_device_descriptor_t* desc = nullptr;
                if (uvc_get_device_descriptor(devices[idx], &desc) == UVC_SUCCESS && desc) {
                    if (options.device_id == desc->serialNumber ||
                        options.device_id == std::to_string(desc->idVendor) + ":" + std::to_string(desc->idProduct)) {
                        dev = devices[idx];
                        uvc_free_device_descriptor(desc);
                        break;
                    }
                    uvc_free_device_descriptor(desc);
                }
                uvc_unref_device(devices[idx]);
                ++idx;
            }
            // Unref remaining devices
            for (int i = idx + 1; devices[i]; ++i) {
                uvc_unref_device(devices[i]);
            }
        } else {
            dev = devices[0];
            for (int i = 1; devices[i]; ++i) {
                uvc_unref_device(devices[i]);
            }
        }

        uvc_free_device_list(devices, 0);

        if (!dev) {
            uvc_exit(ctx);
            ctx = nullptr;
            return Status(StatusCode::kNotFound, "Target camera device not found");
        }

        // Open device
        err = uvc_open(dev, &devh);
        if (err != UVC_SUCCESS) {
            uvc_unref_device(dev);
            dev = nullptr;
            uvc_exit(ctx);
            ctx = nullptr;
            return Status(StatusCode::kUnavailable, std::string("uvc_open failed: ") + uvc_strerror(err));
        }

        // Negotiate format — try MJPEG first, then YUYV
        unsigned int width = options.width > 0 ? options.width : 640;
        unsigned int height = options.height > 0 ? options.height : 480;
        unsigned int fps = options.frame_rate > 0 ? options.frame_rate : 30;

        uvc_stream_ctrl_t ctrl = {};
        err = uvc_get_stream_ctrl_format_size(devh, &ctrl, UVC_FRAME_FORMAT_MJPEG,
                                               width, height, fps);
        if (err == UVC_SUCCESS) {
            pixel_format = PixelFormat::kMJPEG;
        } else {
            err = uvc_get_stream_ctrl_format_size(devh, &ctrl, UVC_FRAME_FORMAT_YUYV,
                                                   width, height, fps);
            if (err == UVC_SUCCESS) {
                pixel_format = PixelFormat::kYUYV;
            } else {
                uvc_close(devh);
                devh = nullptr;
                uvc_unref_device(dev);
                dev = nullptr;
                uvc_exit(ctx);
                ctx = nullptr;
                return Status(StatusCode::kUnavailable, "No supported pixel format at requested resolution/fps");
            }
        }

        actual_width = width;
        actual_height = height;

        // Start streaming with frame callback
        data_handler = std::move(handler);
        capturing.store(true);

        err = uvc_start_streaming(devh, &ctrl, &on_frame_callback, this, 0);
        if (err != UVC_SUCCESS) {
            capturing.store(false);
            uvc_close(devh);
            devh = nullptr;
            uvc_unref_device(dev);
            dev = nullptr;
            uvc_exit(ctx);
            ctx = nullptr;
            return Status(StatusCode::kUnavailable, std::string("uvc_start_streaming failed: ") + uvc_strerror(err));
        }

        nexus::common::diagnostic_log(log::Level::debug, "UVC camera capture started: "
            + std::to_string(actual_width) + "x" + std::to_string(actual_height));
        return Status::ok_status();
    }

    Status stop_capture() override {
        if (!capturing.load()) return Status::ok_status();
        capturing.store(false);

        // Stop streaming — must happen before closing device
        if (devh) {
            uvc_stop_streaming(devh);
            uvc_close(devh);
            devh = nullptr;
        }

        if (dev) {
            uvc_unref_device(dev);
            dev = nullptr;
        }

        if (ctx) {
            uvc_exit(ctx);
            ctx = nullptr;
        }

        data_handler = nullptr;
        return Status::ok_status();
    }

private:
    static void on_frame_callback(uvc_frame_t* frame, void* user_ptr) {
        auto* self = static_cast<UvcCameraCapturerStorage*>(user_ptr);
        if (!self || !self->capturing.load() || !frame || !frame->data) {
            return;
        }

        CameraFrame cframe;
        cframe.timestamp_ms = timeval_to_ms(frame->capture_time);
        cframe.width = frame->width;
        cframe.height = frame->height;
        cframe.pixel_format = uvc_to_nexus_pixel_format(frame->frame_format);
        cframe.data.assign(static_cast<const std::uint8_t*>(frame->data),
                           static_cast<const std::uint8_t*>(frame->data) + frame->data_bytes);
        self->data_handler(std::move(cframe));
    }
};

// --- Platform-backed static helpers ---

Result<std::vector<CameraDevice>> uvc_enumerate_devices() {
    std::vector<CameraDevice> devices;

    uvc_context_t* ctx = nullptr;
    uvc_error_t err = uvc_init(&ctx, nullptr);
    if (err != UVC_SUCCESS) {
        return Status(StatusCode::kUnavailable, std::string("uvc_init failed: ") + uvc_strerror(err));
    }

    uvc_device_t** dev_list = nullptr;
    err = uvc_find_devices(ctx, &dev_list, 0, 0, nullptr);
    if (err != UVC_SUCCESS) {
        uvc_exit(ctx);
        return devices;
    }

    if (!dev_list) {
        uvc_exit(ctx);
        return devices;
    }

    for (int i = 0; dev_list[i]; ++i) {
        uvc_device_descriptor_t* desc = nullptr;
        if (uvc_get_device_descriptor(dev_list[i], &desc) == UVC_SUCCESS && desc) {
            CameraDevice dev;
            dev.id = std::to_string(desc->idVendor) + ":" + std::to_string(desc->idProduct);
            if (desc->serialNumber) {
                dev.id = std::string(desc->serialNumber);
            }
            dev.name = desc->product ? desc->product : dev.id;
            dev.vid = desc->idVendor;
            dev.pid = desc->idProduct;
            devices.push_back(std::move(dev));
            uvc_free_device_descriptor(desc);
        }
    }

    uvc_free_device_list(dev_list, 1);
    uvc_exit(ctx);
    return devices;
}

Result<std::unique_ptr<CameraCapturerStorage>>
uvc_create_storage(CameraCaptureOptions options) {
    return std::unique_ptr<CameraCapturerStorage>(
        std::make_unique<UvcCameraCapturerStorage>(std::move(options)));
}

// --- Camera control stubs (libuvc control API requires per-device unit/selector mapping) ---

std::vector<CameraControlKind> uvc_supported_controls(const std::string& /*device_id*/) {
    return {}; // libuvc control mapping not yet implemented
}

Result<int> uvc_get_control(const std::string& /*device_id*/, CameraControlKind /*kind*/, bool& /*auto_enabled*/) {
    return Status(StatusCode::kUnavailable, "libuvc camera controls not yet implemented");
}

Status uvc_set_control(const std::string& /*device_id*/, CameraControlKind /*kind*/, int /*value*/, bool /*auto_enabled*/) {
    return Status(StatusCode::kUnavailable, "libuvc camera controls not yet implemented");
}

Result<CameraControlRange> uvc_get_control_range(const std::string& /*device_id*/, CameraControlKind /*kind*/) {
    return Status(StatusCode::kUnavailable, "libuvc camera controls not yet implemented");
}

} // namespace detail
} // namespace nexus::camera

#endif // NEXUS_CAMERA_HAS_LIBUVC
