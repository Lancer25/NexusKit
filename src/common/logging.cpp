#include <nexus/common/logging.h>

#include <nexus/log/logger.h>

namespace nexus::common {

void diagnostic_log(nexus::log::Level level, const std::string& message) {
    nexus::log::write(level, message);
}

} // namespace nexus::common
