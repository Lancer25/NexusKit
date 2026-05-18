#pragma once

#include <nexus/screen/screen.h>

namespace nexus::screen::detail {

Result<ScreenFrame> crop_frame_bgra(const ScreenFrame& source,
                                    const ScreenCaptureRegion& region);

} // namespace nexus::screen::detail
