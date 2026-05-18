#pragma once

#include <cstdint>

namespace nexus::screen::detail {

struct X11PixelFormat {
    unsigned long red_mask = 0;
    unsigned long green_mask = 0;
    unsigned long blue_mask = 0;
};

void x11_pixel_to_bgra(unsigned long pixel, const X11PixelFormat& format, std::uint8_t* bgra);

} // namespace nexus::screen::detail
