#include <nexus/common/string.h>
#include <nexus/core/status.h>
#include <nexus/log/logger.h>
#include <nexus/net/http.h>

#include <string>

int main() {
    const auto status = nexus::Status::ok_status();
    const auto trimmed = nexus::common::trim(" NexusKit ");
    const auto path = nexus::net::build_query_path("/health", {{"name", "NexusKit"}});
    return status.ok() && trimmed == "NexusKit" && path == "/health?name=NexusKit" ? 0 : 1;
}
