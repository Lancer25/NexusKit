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
class UdpSocketStorage;
}

struct UdpEndpoint {
    std::string host;
    std::uint16_t port = 0;
};

struct UdpDatagram {
    std::string data;
    UdpEndpoint remote;
};

class NEXUS_NET_API UdpSocket {
public:
    UdpSocket(const UdpSocket&) = delete;
    UdpSocket& operator=(const UdpSocket&) = delete;
    UdpSocket(UdpSocket&& other) noexcept;
    UdpSocket& operator=(UdpSocket&& other) noexcept;
    ~UdpSocket();

    static Result<UdpSocket> bind(const UdpEndpoint& local);

    bool is_open() const;
    Result<std::size_t> send_to(std::string_view data, const UdpEndpoint& remote);
    Result<UdpDatagram> receive_from(std::size_t max_bytes);
    Status close();

private:
    explicit UdpSocket(std::unique_ptr<detail::UdpSocketStorage> storage);

    std::unique_ptr<detail::UdpSocketStorage> storage_;
};

} // namespace nexus::net
