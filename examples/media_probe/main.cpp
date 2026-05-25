#include <nexus/media/media.h>

#include <filesystem>
#include <iostream>
#include <string>

namespace {

void print_backend() {
    const auto backend = nexus::media::ffmpeg_backend_info();
    std::cout << "backend=" << backend.name << '\n';
    std::cout << "available=" << (backend.available ? "true" : "false") << '\n';
    std::cout << "avutil=" << backend.avutil << '\n';
    std::cout << "avcodec=" << backend.avcodec << '\n';
    std::cout << "avformat=" << backend.avformat << '\n';
}

void print_usage(const char* program) {
    std::cout << "Usage: " << program << " [media-file]\n";
}

void print_probe(const nexus::media::MediaProbeInfo& probe) {
    std::cout << "format=" << probe.format_name << '\n';
    std::cout << "duration_ms=" << probe.duration_ms << '\n';
    std::cout << "bit_rate=" << probe.bit_rate << '\n';
    std::cout << "streams=" << probe.streams.size() << '\n';

    for (const auto& stream : probe.streams) {
        std::cout << "stream[" << stream.index << "]"
                  << " type=" << nexus::media::media_stream_type_name(stream.type)
                  << " codec=" << stream.codec_name;
        if (stream.width > 0 || stream.height > 0) {
            std::cout << " size=" << stream.width << 'x' << stream.height;
        }
        if (stream.sample_rate > 0 || stream.channels > 0) {
            std::cout << " audio=" << stream.sample_rate << "Hz/"
                      << stream.channels << "ch";
        }
        std::cout << '\n';
    }
}

} // namespace

int main(int argc, char** argv) {
    if (argc > 2) {
        print_usage(argv[0]);
        return 2;
    }

    print_backend();

    if (argc == 1) {
        print_usage(argv[0]);
        return 0;
    }

    const auto probe = nexus::media::probe_media(std::filesystem::path(argv[1]));
    if (!probe.ok()) {
        std::cerr << "probe failed: " << probe.status().message() << '\n';
        return 1;
    }

    print_probe(probe.value());
    return 0;
}
