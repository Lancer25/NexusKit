#include <nexus/media/media.h>

#if defined(NEXUS_MEDIA_WITH_FFMPEG)
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}
#endif

namespace nexus::media {

FfmpegBackendInfo ffmpeg_backend_info() {
    FfmpegBackendInfo info;

#if defined(NEXUS_MEDIA_WITH_FFMPEG)
    info.available = true;
    info.avutil = avutil_version();
    info.avcodec = avcodec_version();
    info.avformat = avformat_version();
    info.configuration = avutil_configuration();
#endif

    return info;
}

} // namespace nexus::media
