#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <string_view>

#include <nexus/core/result.h>
#include <nexus/core/status.h>
#include <nexus/net/export.h>

namespace nexus::net {

namespace detail {
class WebSocketClientStorage;
}

struct WebSocketClientOptions {
    std::chrono::milliseconds connect_timeout{5000};
    std::chrono::milliseconds receive_timeout{5000};
    std::chrono::milliseconds close_timeout{1000};
};

class NEXUS_NET_API WebSocketClient {
public:
    WebSocketClient(const WebSocketClient&) = delete;
    WebSocketClient& operator=(const WebSocketClient&) = delete;
    WebSocketClient(WebSocketClient&& other) noexcept;
    WebSocketClient& operator=(WebSocketClient&& other) noexcept;
    ~WebSocketClient();

    static Result<WebSocketClient> connect(
        std::string url,
        WebSocketClientOptions options = {});

    bool is_open() const;
    Status send_text(std::string_view message);
    Result<std::string> receive_text();
    Status close();

private:
    explicit WebSocketClient(std::unique_ptr<detail::WebSocketClientStorage> storage);

    std::unique_ptr<detail::WebSocketClientStorage> storage_;
};

} // namespace nexus::net
