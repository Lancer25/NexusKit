#pragma once

#include <cstdint>
#include <vector>

#include <nexus/screen/screen.h>

namespace nexus::screen::detail {

struct CursorOverlayImage {
    int width = 0;
    int height = 0;
    int x = 0;
    int y = 0;
    std::vector<std::uint8_t> pixels_bgra;
};

void overlay_cursor_bgra(ScreenFrame& frame, const CursorOverlayImage& cursor);

} // namespace nexus::screen::detail
