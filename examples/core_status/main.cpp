#include <iostream>

#include <nexus/core/result.h>
#include <nexus/core/status.h>
#include <nexus/core/version.h>

namespace {

nexus::Result<int> parse_demo_value(bool valid) {
    if (!valid) {
        return nexus::Status::invalid_argument("demo value is invalid");
    }
    return 7;
}

} // namespace

int main() {
    std::cout << "NexusKit " << nexus::core::version_string() << '\n';

    const auto result = parse_demo_value(true);
    if (!result.ok()) {
        std::cerr << result.status().message() << '\n';
        return 1;
    }

    std::cout << "value=" << result.value() << '\n';
    return 0;
}
