#include "screen_frame_crop.h"

#include <cstdint>
#include <cstring>

namespace nexus::screen::detail {

Result<ScreenFrame> crop_frame_bgra(const ScreenFrame& source,
                                    const ScreenCaptureRegion& region) {
    if (region.width <= 0 || region.height <= 0) {
        return Status::invalid_argument("screen capture region dimensions must be positive");
    }
    if (region.x < 0 || region.y < 0) {
        return Status::invalid_argument("screen capture region origin must be non-negative");
    }
    if (source.width <= 0 || source.height <= 0 || source.data.empty()) {
        return Status::invalid_argument("screen capture region requires a non-empty source frame");
    }
    if (source.pixel_format != ScreenPixelFormat::bgra) {
        return Status::invalid_argument("screen capture region requires BGRA screen frames");
    }

    const auto source_width = static_cast<std::int64_t>(source.width);
    const auto source_height = static_cast<std::int64_t>(source.height);
    const auto x = static_cast<std::int64_t>(region.x);
    const auto y = static_cast<std::int64_t>(region.y);
    const auto width = static_cast<std::int64_t>(region.width);
    const auto height = static_cast<std::int64_t>(region.height);

    if (x >= source_width || y >= source_height || x + width > source_width ||
        y + height > source_height) {
        return Status::invalid_argument("screen capture region is outside the source frame");
    }

    const auto expected_source_size =
        static_cast<std::size_t>(source.width) * static_cast<std::size_t>(source.height) * 4u;
    if (source.data.size() < expected_source_size) {
        return Status::invalid_argument("screen capture source frame data is smaller than expected");
    }

    ScreenFrame cropped;
    cropped.width = region.width;
    cropped.height = region.height;
    cropped.pixel_format = source.pixel_format;
    cropped.timestamp_ms = source.timestamp_ms;
    cropped.data.resize(static_cast<std::size_t>(region.width) *
                        static_cast<std::size_t>(region.height) * 4u);

    const auto row_bytes = static_cast<std::size_t>(region.width) * 4u;
    for (int row = 0; row < region.height; ++row) {
        const auto source_offset =
            (static_cast<std::size_t>(region.y + row) * static_cast<std::size_t>(source.width) +
             static_cast<std::size_t>(region.x)) *
            4u;
        const auto target_offset = static_cast<std::size_t>(row) * row_bytes;
        std::memcpy(cropped.data.data() + target_offset,
                    source.data.data() + source_offset,
                    row_bytes);
    }

    return cropped;
}

} // namespace nexus::screen::detail
