#pragma once

#include <atomic>
#include <memory>

#include <asio.hpp>

#include <nexus/common/thread.h>

namespace nexus::net {
namespace detail {

class TcpClientStorage {
public:
    asio::io_context io;
    asio::ip::tcp::socket socket;
    asio::executor_work_guard<asio::io_context::executor_type> work;
    nexus::common::Thread worker;
    std::atomic<bool> open{false};
    std::atomic<bool> stopped{false};

    TcpClientStorage()
        : socket(io), work(asio::make_work_guard(io)) {}
};

} // namespace detail
} // namespace nexus::net
