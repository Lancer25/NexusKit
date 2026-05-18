// V4L2 camera capture backend (Linux).

#ifdef NEXUS_CAMERA_HAS_V4L2

#include "capturer_internal.h"

#include <atomic>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <dirent.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <linux/videodev2.h>

#include <nexus/common/logging.h>
#include <nexus/common/thread.h>
#include <nexus/log/logger.h>

namespace nexus::camera {
namespace detail {

namespace {

constexpr int kInvalidFd = -1;
constexpr unsigned int kMaxBuffers = 4;

bool xioctl(int fd, unsigned long request, void* arg) {
    int ret;
    do {
        ret = ::ioctl(fd, request, arg);
    } while (ret == -1 && errno == EINTR);
    return ret != -1;
}

PixelFormat v4l2_to_nexus_pixel_format(unsigned int v4l2_fmt) {
    switch (v4l2_fmt) {
    case V4L2_PIX_FMT_MJPEG:
        return PixelFormat::kMJPEG;
    case V4L2_PIX_FMT_YUYV:
        return PixelFormat::kYUYV;
    default:
        return PixelFormat::kUnknown;
    }
}

std::int64_t timeval_to_ms(const timeval& tv) {
    return static_cast<std::int64_t>(tv.tv_sec) * 1000 +
           static_cast<std::int64_t>(tv.tv_usec) / 1000;
}

} // namespace

class V4l2CameraCapturerStorage : public CameraCapturerStorage {
public:
    nexus::common::Thread thread;
    int fd = kInvalidFd;

    // mmap buffer info
    struct BufferInfo {
        unsigned char* data = nullptr;
        unsigned int length = 0;
    };
    std::vector<BufferInfo> buffers;

    // Negotiated format
    unsigned int actual_width = 0;
    unsigned int actual_height = 0;
    PixelFormat pixel_format = PixelFormat::kUnknown;

    // Handler
    CameraCapturer::DataHandler data_handler;

    explicit V4l2CameraCapturerStorage(CameraCaptureOptions opts)
        : CameraCapturerStorage(std::move(opts)) {}

    ~V4l2CameraCapturerStorage() override {
        stop_capture();
    }

    Status start_capture(CameraCapturer::DataHandler handler) override {
        if (capturing.load()) {
            return Status(StatusCode::kFailedPrecondition, "Already capturing");
        }

        // Find target device
        std::string dev_path;
        if (!options.device_id.empty()) {
            dev_path = options.device_id;
        } else {
            auto devices_result = v4l2_enumerate_devices();
            if (!devices_result.ok()) {
                return devices_result.status();
            }
            auto& devices = devices_result.value();
            if (devices.empty()) {
                return Status(StatusCode::kNotFound, "No camera devices found");
            }
            dev_path = devices[0].path;
        }

        // Open device
        fd = ::open(dev_path.c_str(), O_RDWR | O_NONBLOCK);
        if (fd < 0) {
            return Status(StatusCode::kUnavailable, "Failed to open camera device: " + dev_path);
        }

        // Query capabilities
        v4l2_capability cap = {};
        if (!xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
            ::close(fd);
            fd = kInvalidFd;
            return Status(StatusCode::kUnavailable, "VIDIOC_QUERYCAP failed");
        }
        if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
            ::close(fd);
            fd = kInvalidFd;
            return Status(StatusCode::kUnavailable, "Device does not support video capture");
        }

        // Negotiate format — try MJPEG first, then YUYV
        unsigned int width = options.width > 0 ? options.width : 640;
        unsigned int height = options.height > 0 ? options.height : 480;
        unsigned int frame_rate = options.frame_rate > 0 ? options.frame_rate : 30;

        if (!try_format(width, height, V4L2_PIX_FMT_MJPEG) &&
            !try_format(width, height, V4L2_PIX_FMT_YUYV)) {
            ::close(fd);
            fd = kInvalidFd;
            return Status(StatusCode::kUnavailable, "No supported pixel format (MJPEG/YUYV) at requested resolution");
        }

        // Set frame rate
        v4l2_streamparm streamparm = {};
        streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (xioctl(fd, VIDIOC_G_PARM, &streamparm)) {
            if (streamparm.parm.capture.capability & V4L2_CAP_TIMEPERFRAME) {
                streamparm.parm.capture.timeperframe.numerator = 1;
                streamparm.parm.capture.timeperframe.denominator = frame_rate;
                xioctl(fd, VIDIOC_S_PARM, &streamparm);
            }
        }

        // Allocate mmap buffers
        if (!init_buffers()) {
            ::close(fd);
            fd = kInvalidFd;
            return Status(StatusCode::kUnavailable, "Failed to initialize V4L2 buffers");
        }

        // Start streaming
        v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (!xioctl(fd, VIDIOC_STREAMON, &type)) {
            uninit_buffers();
            ::close(fd);
            fd = kInvalidFd;
            return Status(StatusCode::kUnavailable, "VIDIOC_STREAMON failed");
        }

        data_handler = std::move(handler);
        capturing.store(true);

        auto* self = this;
        thread.start([self] { self->capture_loop(); });

        nexus::common::diagnostic_log(log::Level::debug, "V4L2 camera capture started: "
            + std::to_string(actual_width) + "x" + std::to_string(actual_height));
        return Status::ok_status();
    }

    Status stop_capture() override {
        if (!capturing.load()) return Status::ok_status();
        capturing.store(false);

        // Stop streaming
        if (fd >= 0) {
            v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            ioctl(fd, VIDIOC_STREAMOFF, &type);
        }

        // Wait for capture thread
        if (thread.is_running()) {
            thread.stop();
        }

        uninit_buffers();

        if (fd >= 0) {
            ::close(fd);
            fd = kInvalidFd;
        }

        data_handler = nullptr;
        return Status::ok_status();
    }

private:
    bool try_format(unsigned int width, unsigned int height, unsigned int pixel_fmt) {
        v4l2_format fmt = {};
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.width = width;
        fmt.fmt.pix.height = height;
        fmt.fmt.pix.pixelformat = pixel_fmt;

        if (!xioctl(fd, VIDIOC_S_FMT, &fmt)) {
            return false;
        }

        if (fmt.fmt.pix.pixelformat != pixel_fmt) {
            return false;
        }

        actual_width = fmt.fmt.pix.width;
        actual_height = fmt.fmt.pix.height;
        pixel_format = v4l2_to_nexus_pixel_format(fmt.fmt.pix.pixelformat);
        return true;
    }

    bool init_buffers() {
        v4l2_requestbuffers req = {};
        req.count = kMaxBuffers;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_MMAP;

        if (!xioctl(fd, VIDIOC_REQBUFS, &req)) {
            return false;
        }

        for (unsigned int i = 0; i < kMaxBuffers; ++i) {
            v4l2_buffer buf = {};
            buf.index = i;
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;

            if (!xioctl(fd, VIDIOC_QUERYBUF, &buf)) {
                return false;
            }

            void* ptr = mmap(nullptr, buf.length, PROT_READ | PROT_WRITE,
                             MAP_SHARED, fd, buf.m.offset);
            if (MAP_FAILED == ptr) {
                return false;
            }

            buffers.push_back({static_cast<unsigned char*>(ptr), buf.length});

            // Enqueue buffer
            if (!xioctl(fd, VIDIOC_QBUF, &buf)) {
                return false;
            }
        }

        return true;
    }

    void uninit_buffers() {
        for (auto& buf : buffers) {
            if (buf.data) {
                munmap(buf.data, buf.length);
            }
        }
        buffers.clear();
    }

    void capture_loop() {
        while (capturing.load()) {
            // Wait for frame with timeout
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(fd, &fds);

            timeval tv = {};
            tv.tv_sec = 2;
            tv.tv_usec = 0;

            int ret = select(fd + 1, &fds, nullptr, nullptr, &tv);
            if (ret <= 0) {
                continue;
            }

            // Dequeue frame
            v4l2_buffer buf = {};
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;

            if (!xioctl(fd, VIDIOC_DQBUF, &buf)) {
                continue;
            }

            // Build frame
            if (buf.index < buffers.size() && buf.bytesused > 0) {
                CameraFrame frame;
                frame.timestamp_ms = timeval_to_ms(buf.timestamp);
                frame.width = actual_width;
                frame.height = actual_height;
                frame.pixel_format = pixel_format;
                frame.data.assign(buffers[buf.index].data,
                                  buffers[buf.index].data + buf.bytesused);
                data_handler(std::move(frame));
            }

            // Re-enqueue buffer
            if (!xioctl(fd, VIDIOC_QBUF, &buf)) {
                data_handler(Status(StatusCode::kUnavailable, "VIDIOC_QBUF failed"));
                break;
            }
        }
    }
};

// --- Platform-backed static helpers ---

Result<std::vector<CameraDevice>> v4l2_enumerate_devices() {
    std::vector<CameraDevice> devices;

    DIR* dir = ::opendir("/dev");
    if (!dir) {
        return devices;
    }

    while (dirent* entry = ::readdir(dir)) {
        if (std::strncmp(entry->d_name, "video", 5) != 0) {
            continue;
        }

        std::string path = std::string("/dev/") + entry->d_name;
        int fd = ::open(path.c_str(), O_RDONLY | O_NONBLOCK);
        if (fd < 0) {
            continue;
        }

        v4l2_capability cap = {};
        if (xioctl(fd, VIDIOC_QUERYCAP, &cap) &&
            (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
            CameraDevice dev;
            dev.id = path;
            dev.path = path;
            dev.name = reinterpret_cast<const char*>(cap.card);
            if (dev.name.empty()) {
                dev.name = path;
            }
            devices.push_back(std::move(dev));
        }

        ::close(fd);
    }

    ::closedir(dir);
    return devices;
}

Result<std::unique_ptr<CameraCapturerStorage>>
v4l2_create_storage(CameraCaptureOptions options) {
    return std::unique_ptr<CameraCapturerStorage>(
        std::make_unique<V4l2CameraCapturerStorage>(std::move(options)));
}

// --- Camera control helpers ---

namespace {

int control_to_v4l2_cid(CameraControlKind kind) {
    switch (kind) {
    case CameraControlKind::kBrightness:          return V4L2_CID_BRIGHTNESS;
    case CameraControlKind::kContrast:            return V4L2_CID_CONTRAST;
    case CameraControlKind::kSaturation:          return V4L2_CID_SATURATION;
    case CameraControlKind::kHue:                 return V4L2_CID_HUE;
    case CameraControlKind::kSharpness:           return V4L2_CID_SHARPNESS;
    case CameraControlKind::kGamma:               return V4L2_CID_GAMMA;
    case CameraControlKind::kWhiteBalance:        return V4L2_CID_WHITE_BALANCE_TEMPERATURE;
    case CameraControlKind::kBacklightCompensation: return V4L2_CID_BACKLIGHT_COMPENSATION;
    case CameraControlKind::kGain:                return V4L2_CID_GAIN;
    case CameraControlKind::kFocus:               return V4L2_CID_FOCUS_ABSOLUTE;
    case CameraControlKind::kZoom:                return V4L2_CID_ZOOM_ABSOLUTE;
    case CameraControlKind::kExposure:            return V4L2_CID_EXPOSURE_ABSOLUTE;
    case CameraControlKind::kIris:                return V4L2_CID_IRIS_ABSOLUTE;
    case CameraControlKind::kPan:                 return V4L2_CID_PAN_ABSOLUTE;
    case CameraControlKind::kTilt:                return V4L2_CID_TILT_ABSOLUTE;
    case CameraControlKind::kAutoFocus:           return V4L2_CID_FOCUS_AUTO;
    case CameraControlKind::kAutoWhiteBalance:    return V4L2_CID_AUTO_WHITE_BALANCE;
    case CameraControlKind::kAutoExposure:        return V4L2_CID_EXPOSURE_AUTO;
    case CameraControlKind::kPowerLineFrequency:  return V4L2_CID_POWER_LINE_FREQUENCY;
    default: return -1;
    }
}

bool is_auto_control(CameraControlKind kind) {
    return kind == CameraControlKind::kAutoFocus ||
           kind == CameraControlKind::kAutoWhiteBalance ||
           kind == CameraControlKind::kAutoExposure;
}

bool control_has_auto(CameraControlKind kind) {
    return kind == CameraControlKind::kFocus ||
           kind == CameraControlKind::kWhiteBalance ||
           kind == CameraControlKind::kExposure;
}

} // namespace

std::vector<CameraControlKind> v4l2_supported_controls(const std::string& device_path) {
    // Statically return all standard V4L2 controls — runtime probing is heavy.
    return {
        CameraControlKind::kBrightness,
        CameraControlKind::kContrast,
        CameraControlKind::kSaturation,
        CameraControlKind::kHue,
        CameraControlKind::kSharpness,
        CameraControlKind::kGamma,
        CameraControlKind::kWhiteBalance,
        CameraControlKind::kBacklightCompensation,
        CameraControlKind::kGain,
        CameraControlKind::kFocus,
        CameraControlKind::kZoom,
        CameraControlKind::kExposure,
        CameraControlKind::kIris,
        CameraControlKind::kPan,
        CameraControlKind::kTilt,
        CameraControlKind::kAutoFocus,
        CameraControlKind::kAutoWhiteBalance,
        CameraControlKind::kAutoExposure,
        CameraControlKind::kPowerLineFrequency
    };
}

Result<int> v4l2_get_control(const std::string& device_path, CameraControlKind kind, bool& auto_enabled) {
    int fd = ::open(device_path.c_str(), O_RDWR | O_NONBLOCK);
    if (fd < 0)
        return Status(StatusCode::kNotFound, "Cannot open camera device");

    int cid = control_to_v4l2_cid(kind);
    if (cid < 0) {
        ::close(fd);
        return Status(StatusCode::kInvalidArgument, "Unsupported control");
    }

    v4l2_control ctrl = {};
    ctrl.id = static_cast<unsigned int>(cid);
    bool ok = xioctl(fd, VIDIOC_G_CTRL, &ctrl);
    ::close(fd);

    if (!ok)
        return Status(StatusCode::kNotFound, "Control not available");

    auto_enabled = false;
    if (kind == CameraControlKind::kAutoExposure) {
        auto_enabled = (ctrl.value == V4L2_EXPOSURE_AUTO);
    } else if (is_auto_control(kind)) {
        auto_enabled = (ctrl.value != 0);
    }
    return ctrl.value;
}

Status v4l2_set_control(const std::string& device_path, CameraControlKind kind, int value, bool auto_enabled) {
    int fd = ::open(device_path.c_str(), O_RDWR | O_NONBLOCK);
    if (fd < 0)
        return Status(StatusCode::kNotFound, "Cannot open camera device");

    int cid = control_to_v4l2_cid(kind);
    if (cid < 0) {
        ::close(fd);
        return Status(StatusCode::kInvalidArgument, "Unsupported control");
    }

    v4l2_control ctrl = {};
    ctrl.id = static_cast<unsigned int>(cid);

    if (kind == CameraControlKind::kAutoExposure) {
        ctrl.value = auto_enabled ? V4L2_EXPOSURE_AUTO : V4L2_EXPOSURE_MANUAL;
    } else if (is_auto_control(kind)) {
        ctrl.value = auto_enabled ? 1 : 0;
    } else {
        ctrl.value = value;
    }

    bool ok = xioctl(fd, VIDIOC_S_CTRL, &ctrl);
    ::close(fd);

    if (!ok)
        return Status(StatusCode::kUnavailable, "Failed to set control");
    return Status::ok_status();
}

Result<CameraControlRange> v4l2_get_control_range(const std::string& device_path, CameraControlKind kind) {
    int fd = ::open(device_path.c_str(), O_RDWR | O_NONBLOCK);
    if (fd < 0)
        return Status(StatusCode::kNotFound, "Cannot open camera device");

    int cid = control_to_v4l2_cid(kind);
    if (cid < 0) {
        ::close(fd);
        return Status(StatusCode::kInvalidArgument, "Unsupported control");
    }

    v4l2_queryctrl query = {};
    query.id = static_cast<unsigned int>(cid);
    bool ok = xioctl(fd, VIDIOC_QUERYCTRL, &query);
    ::close(fd);

    if (!ok)
        return Status(StatusCode::kNotFound, "Control not available");

    CameraControlRange range;
    range.minimum = query.minimum;
    range.maximum = query.maximum;
    range.step = query.step;
    range.default_value = query.default_value;
    range.supports_auto = control_has_auto(kind);
    range.available = true;
    return range;
}

} // namespace detail
} // namespace nexus::camera

#endif // NEXUS_CAMERA_HAS_V4L2
