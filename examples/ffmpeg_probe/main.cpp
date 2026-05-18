extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

#include <iostream>

int main() {
    std::cout << "libavcodec=" << avcodec_version() << '\n';
    std::cout << "libavformat=" << avformat_version() << '\n';
    std::cout << "libavutil=" << avutil_version() << '\n';
    return 0;
}
