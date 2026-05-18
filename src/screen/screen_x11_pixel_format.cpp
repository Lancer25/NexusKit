#include "screen_x11_pixel_format.h"

namespace nexus::screen::detail {
namespace {

int mask_shift(unsigned long mask) {
    int shift = 0;
    while (mask != 0 && (mask & 1ul) == 0ul) {
        mask >>= 1;
        ++shift;
    }
    return shift;
}

int mask_bits(unsigned long mask) {
    int bits = 0;
    while (mask != 0) {
        if ((mask & 1ul) != 0ul) {
            ++bits;
        }
        mask >>= 1;
    }
    return bits;
}

std::uint8_t extract_channel(unsigned long pixel, unsigned long mask) {
    if (mask == 0ul) {
        return 0;
    }

    const int shift = mask_shift(mask);
    const int bits = mask_bits(mask >> shift);
    if (bits <= 0) {
        return 0;
    }

    const unsigned long value = (pixel & mask) >> shift;
    const unsigned long max_value =
        bits >= static_cast<int>(sizeof(unsigned long) * 8u) ? ~0ul : (1ul << bits) - 1ul;
    return static_cast<std::uint8_t>((value * 255ul + (max_value / 2ul)) / max_value);
}

} // namespace

void x11_pixel_to_bgra(unsigned long pixel, const X11PixelFormat& format, std::uint8_t* bgra) {
    if (!bgra) {
        return;
    }

    bgra[0] = extract_channel(pixel, format.blue_mask);
    bgra[1] = extract_channel(pixel, format.green_mask);
    bgra[2] = extract_channel(pixel, format.red_mask);
    bgra[3] = 255;
}

} // namespace nexus::screen::detail
