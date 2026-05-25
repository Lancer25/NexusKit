#include <nexus/net/tcp.h>
#include <nexus/net/tcp_listener.h>

#include <future>
#include <iostream>
#include <string>

namespace {

int fail(const std::string& message, const nexus::Status& status) {
    std::cerr << message << ": " << status.message() << '\n';
    return 1;
}

} // namespace

int main() {
    constexpr auto kPayload = "hello nexus";

    auto listener = nexus::net::TcpListener::create({"127.0.0.1", 0});
    if (!listener.ok()) {
        return fail("listener failed", listener.status());
    }

    std::promise<nexus::Result<nexus::net::TcpClient>> accepted_promise;
    auto accepted_future = accepted_promise.get_future();

    listener.value().async_accept(
        [&accepted_promise](nexus::Result<nexus::net::TcpClient> accepted) mutable {
            accepted_promise.set_value(std::move(accepted));
        });

    auto client = nexus::net::TcpClient::connect({"127.0.0.1", listener.value().port()});
    if (!client.ok()) {
        return fail("client connect failed", client.status());
    }

    auto accepted = accepted_future.get();
    if (!accepted.ok()) {
        return fail("accept failed", accepted.status());
    }

    auto write_status = client.value().write_all(kPayload);
    if (!write_status.ok()) {
        return fail("client write failed", write_status);
    }

    auto received = accepted.value().read_some(64);
    if (!received.ok()) {
        return fail("server read failed", received.status());
    }

    write_status = accepted.value().write_all(received.value());
    if (!write_status.ok()) {
        return fail("server write failed", write_status);
    }

    auto echoed = client.value().read_some(64);
    if (!echoed.ok()) {
        return fail("client read failed", echoed.status());
    }

    std::cout << "echo=" << echoed.value() << '\n';

    accepted.value().close();
    client.value().close();
    listener.value().close();
    return echoed.value() == kPayload ? 0 : 1;
}
