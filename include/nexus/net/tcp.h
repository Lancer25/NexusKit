#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#include <nexus/core/result.h>
#include <nexus/core/status.h>
#include <nexus/net/export.h>

namespace nexus::net {

namespace detail {
class TcpClientStorage;
}

struct TcpEndpoint {
    std::string host;
    std::uint16_t port = 0;
};

class NEXUS_NET_API TcpClient {
public:
    TcpClient(const TcpClient&) = delete;
    TcpClient& operator=(const TcpClient&) = delete;
    TcpClient(TcpClient&& other) noexcept;
    TcpClient& operator=(TcpClient&& other) noexcept;
    ~TcpClient();

    static Result<TcpClient> connect(const TcpEndpoint& endpoint);

    bool is_open() const;
    Status write_all(std::string_view data);
    Result<std::string> read_some(std::size_t max_bytes);
    Status close();

private:
    explicit TcpClient(std::unique_ptr<detail::TcpClientStorage> storage);

    std::unique_ptr<detail::TcpClientStorage> storage_;
};

} // namespace nexus::net
