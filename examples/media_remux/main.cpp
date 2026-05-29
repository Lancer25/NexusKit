#include <nexus/media/media.h>

#include <filesystem>
#include <iostream>

namespace {

void print_usage(const char* program) {
    std::cout << "Usage: " << program << " <input-media> <output-media>\n";
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 3) {
        print_usage(argv[0]);
        return 2;
    }

    const auto source = std::filesystem::path(argv[1]);
    const auto dest = std::filesystem::path(argv[2]);

    const auto status = nexus::media::remux_file(source, dest);
    if (!status.ok()) {
        std::cerr << "remux failed: " << status.message() << '\n';
        return 1;
    }

    std::cout << "remuxed " << source << " -> " << dest << '\n';
    return 0;
}
