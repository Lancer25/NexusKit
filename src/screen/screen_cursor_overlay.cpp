#include "screen_cursor_overlay.h"

#include <algorithm>
#include <cstddef>

namespace nexus::screen::detail {
namespace {

std::uint8_t blend_channel(const std::uint8_t source, const std::uint8_t destination,
                           const std::uint8_t alpha) {
    const auto inverse_alpha = static_cast<unsigned>(255u - alpha);
    const auto value = static_cast<unsigned>(source) * alpha +
                       static_cast<unsigned>(destination) * inverse_alpha + 127u;
    return static_cast<std::uint8_t>(value / 255u);
}

} // namespace

void overlay_cursor_bgra(ScreenFrame& frame, const CursorOverlayImage& cursor) {
    if (frame.pixel_format != ScreenPixelFormat::bgra || frame.width <= 0 || frame.height <= 0 ||
        cursor.width <= 0 || cursor.height <= 0) {
        return;
    }

    const std::size_t frame_size = static_cast<std::size_t>(frame.width) *
                                   static_cast<std::size_t>(frame.height) * 4u;
    const std::size_t cursor_size = static_cast<std::size_t>(cursor.width) *
                                    static_cast<std::size_t>(cursor.height) * 4u;
    if (frame.data.size() < frame_size || cursor.pixels_bgra.size() < cursor_size) {
        return;
    }

    const int start_x = std::max(cursor.x, 0);
    const int start_y = std::max(cursor.y, 0);
    const int end_x = std::min(cursor.x + cursor.width, frame.width);
    const int end_y = std::min(cursor.y + cursor.height, frame.height);
    if (start_x >= end_x || start_y >= end_y) {
        return;
    }

    for (int y = start_y; y < end_y; ++y) {
        const int cursor_y = y - cursor.y;
        for (int x = start_x; x < end_x; ++x) {
            const int cursor_x = x - cursor.x;
            const std::size_t cursor_index =
                (static_cast<std::size_t>(cursor_y) * static_cast<std::size_t>(cursor.width) +
                 static_cast<std::size_t>(cursor_x)) *
                4u;
            const std::uint8_t alpha = cursor.pixels_bgra[cursor_index + 3u];
            if (alpha == 0u) {
                continue;
            }

            const std::size_t frame_index =
                (static_cast<std::size_t>(y) * static_cast<std::size_t>(frame.width) +
                 static_cast<std::size_t>(x)) *
                4u;
            if (alpha == 255u) {
                frame.data[frame_index] = cursor.pixels_bgra[cursor_index];
                frame.data[frame_index + 1u] = cursor.pixels_bgra[cursor_index + 1u];
                frame.data[frame_index + 2u] = cursor.pixels_bgra[cursor_index + 2u];
                frame.data[frame_index + 3u] = 255u;
                continue;
            }

            frame.data[frame_index] =
                blend_channel(cursor.pixels_bgra[cursor_index], frame.data[frame_index], alpha);
            frame.data[frame_index + 1u] = blend_channel(cursor.pixels_bgra[cursor_index + 1u],
                                                        frame.data[frame_index + 1u], alpha);
            frame.data[frame_index + 2u] = blend_channel(cursor.pixels_bgra[cursor_index + 2u],
                                                        frame.data[frame_index + 2u], alpha);
            frame.data[frame_index + 3u] = 255u;
        }
    }
}

} // namespace nexus::screen::detail
