#include <nexus/net/websocket_client.h>

#include <atomic>
#include <cctype>
#include <condition_variable>
#include <exception>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

#include <websocketpp/client.hpp>
#ifdef NEXUS_NET_HAS_TLS
#include <websocketpp/config/asio_client.hpp>
#else
#include <websocketpp/config/asio_no_tls_client.hpp>
#endif

#include <nexus/common/logging.h>
#include <nexus/common/string.h>
#include <nexus/common/thread.h>
#include <nexus/log/logger.h>

namespace nexus::net {

namespace detail {

class WebSocketClientStorage {
public:
#ifdef NEXUS_NET_HAS_TLS
    using WsppConfig = websocketpp::config::asio_tls_client;
#else
    using WsppConfig = websocketpp::config::asio_client;
#endif
    using Client = websocketpp::client<WsppConfig>;

    explicit WebSocketClientStorage(WebSocketClientOptions client_options)
        : options(client_options) {
        client.clear_access_channels(websocketpp::log::alevel::all);
        client.clear_error_channels(websocketpp::log::elevel::all);
        client.init_asio();
    }

    Client client;
    websocketpp::connection_hdl handle;
    WebSocketClientOptions options;

    mutable std::mutex mutex;
    std::condition_variable cv;
    std::queue<std::string> messages;

    std::thread ws_thread;
    std::atomic<bool> open{false};
    std::atomic<bool> closed{false};
    std::atomic<bool> stopped{false};
    Status failure = Status::ok_status();

    // One-shot async handlers — consumed on first matching event.
    WebSocketClient::ConnectHandler pending_connect;
    WebSocketClient::SendHandler    pending_send;
    WebSocketClient::MessageHandler pending_message;
    WebSocketClient::CloseHandler   pending_close;
};

} // namespace detail

namespace {

Status validate_url(std::string_view url) {
    bool is_secure = false;
    if (nexus::common::starts_with(url, "wss://")) {
#ifdef NEXUS_NET_HAS_TLS
        is_secure = true;
#else
        return Status::invalid_argument(
            "wss:// WebSocket URLs require TLS support; rebuild with OpenSSL");
#endif
    } else if (nexus::common::starts_with(url, "ws://")) {
        // plain WebSocket
    } else {
        return Status::invalid_argument("WebSocket URL must start with ws:// or wss://");
    }

    const auto authority = is_secure ? url.substr(6) : url.substr(5);
    if (authority.empty()) {
        return Status::invalid_argument("WebSocket URL must contain a host");
    }

    if (nexus::common::contains_space(authority)) {
        return Status::invalid_argument("WebSocket URL must not contain whitespace");
    }

    return Status::ok_status();
}

Status websocket_status(const std::string& message) {
    return Status(StatusCode::kUnavailable, message);
}

void join_ws_thread(std::thread& t) {
    if (t.joinable()) {
        t.join();
    }
}

void setup_handlers(std::shared_ptr<detail::WebSocketClientStorage> st) {
    std::weak_ptr<detail::WebSocketClientStorage> weak = st;

    st->client.set_open_handler([weak](websocketpp::connection_hdl hdl) {
        auto sp = weak.lock();
        if (!sp || sp->stopped.load()) return;

        std::unique_lock<std::mutex> lock(sp->mutex);
        sp->handle = hdl;
        sp->open.store(true);
        sp->closed.store(false);
        sp->cv.notify_all();
        auto handler = std::move(sp->pending_connect);
        lock.unlock();

        nexus::common::diagnostic_log(log::Level::info, "WebSocket connected");
        if (handler) handler(Status::ok_status());
    });

    st->client.set_fail_handler([weak](websocketpp::connection_hdl hdl) {
        auto sp = weak.lock();
        if (!sp || sp->stopped.load()) return;

        auto conn = sp->client.get_con_from_hdl(hdl);
        const auto msg = conn ? conn->get_ec().message()
                              : std::string("connect failed");

        std::unique_lock<std::mutex> lock(sp->mutex);
        sp->open.store(false);
        sp->closed.store(true);
        sp->failure = websocket_status(msg);
        sp->cv.notify_all();
        auto handler = std::move(sp->pending_connect);
        lock.unlock();

        nexus::common::diagnostic_log(log::Level::warn, "WebSocket failed: " + msg);
        if (handler) handler(sp->failure);
    });

    st->client.set_message_handler(
        [weak](websocketpp::connection_hdl,
               detail::WebSocketClientStorage::Client::message_ptr msg) {
            auto sp = weak.lock();
            if (!sp || sp->stopped.load()) return;

            auto payload = msg->get_payload();

            std::unique_lock<std::mutex> lock(sp->mutex);
            // Deliver to one-shot async handler if present; otherwise queue.
            auto handler = std::move(sp->pending_message);
            if (!handler) {
                sp->messages.push(std::move(payload));
            }
            sp->cv.notify_all();
            lock.unlock();

            nexus::common::diagnostic_log(
                log::Level::debug,
                "WebSocket receive bytes=" + std::to_string(payload.size()));

            if (handler) handler(std::move(payload));
        });

    st->client.set_close_handler([weak](websocketpp::connection_hdl) {
        auto sp = weak.lock();
        if (!sp || sp->stopped.load()) return;

        std::unique_lock<std::mutex> lock(sp->mutex);
        sp->open.store(false);
        sp->closed.store(true);
        sp->cv.notify_all();
        auto handler = std::move(sp->pending_close);
        lock.unlock();

        nexus::common::diagnostic_log(log::Level::debug, "WebSocket closed");
        if (handler) handler(Status::ok_status());
    });
}

} // namespace

// --- construct / destruct ---

WebSocketClient::WebSocketClient(
    std::shared_ptr<detail::WebSocketClientStorage> storage)
    : storage_(std::move(storage)) {}

WebSocketClient::WebSocketClient(WebSocketClient&& other) noexcept
    : storage_(std::move(other.storage_)) {}

WebSocketClient& WebSocketClient::operator=(WebSocketClient&& other) noexcept {
    if (this != &other) {
        this->~WebSocketClient();
        storage_ = std::move(other.storage_);
    }
    return *this;
}

WebSocketClient::~WebSocketClient() {
    close();
}

// --- factory methods ---

Result<WebSocketClient> WebSocketClient::create(WebSocketClientOptions options) {
    auto storage = std::make_shared<detail::WebSocketClientStorage>(options);
    setup_handlers(storage);

    storage->client.start_perpetual();

    auto* raw = storage.get();
    raw->ws_thread = std::thread([raw]() { raw->client.run(); });

    return WebSocketClient(std::move(storage));
}

Result<WebSocketClient> WebSocketClient::connect(
    std::string url,
    WebSocketClientOptions options) {
    return connect_impl(std::move(url), options);
}

Result<WebSocketClient> WebSocketClient::connect_impl(
    std::string url,
    WebSocketClientOptions options) {
    const auto status = validate_url(url);
    if (!status.ok()) return status;

    auto storage = std::make_shared<detail::WebSocketClientStorage>(options);
    setup_handlers(storage);

    websocketpp::lib::error_code error;
    auto connection = storage->client.get_connection(url, error);
    if (error) return websocket_status(error.message());

    storage->handle = connection->get_handle();

    nexus::common::diagnostic_log(log::Level::debug, "WebSocket connect " + url);

    std::unique_lock<std::mutex> lock(storage->mutex);
    storage->client.connect(connection);
    storage->ws_thread = std::thread([raw = storage.get()]() {
        raw->client.run();
    });

    const auto connected = storage->cv.wait_for(
        lock,
        options.connect_timeout,
        [&storage]() {
            return storage->open.load() || storage->closed.load();
        });

    if (!connected) {
        lock.unlock();
        storage->client.stop();
        join_ws_thread(storage->ws_thread);
        return websocket_status("WebSocket connect timed out");
    }

    if (!storage->open.load()) {
        const auto failure = storage->failure.ok()
            ? websocket_status("WebSocket connect failed")
            : storage->failure;
        lock.unlock();
        storage->client.stop();
        join_ws_thread(storage->ws_thread);
        return failure;
    }

    return WebSocketClient(std::move(storage));
}

// --- state ---

bool WebSocketClient::is_open() const {
    return storage_ && storage_->open.load() && !storage_->stopped.load();
}

// --- async connect ---

void WebSocketClient::async_connect(std::string url, ConnectHandler h) {
    const auto status = validate_url(url);
    if (!status.ok()) {
        if (h) h(status);
        return;
    }

    if (!storage_ || storage_->stopped.load()) {
        if (h) h(Status(StatusCode::kFailedPrecondition, "WebSocket client is closed"));
        return;
    }

    websocketpp::lib::error_code error;
    auto connection = storage_->client.get_connection(url, error);
    if (error) {
        if (h) h(websocket_status(error.message()));
        return;
    }

    storage_->handle = connection->get_handle();

    nexus::common::diagnostic_log(log::Level::debug, "WebSocket async_connect " + url);

    std::unique_lock<std::mutex> lock(storage_->mutex);
    storage_->pending_connect = std::move(h);
    storage_->client.connect(connection);
    lock.unlock();
}

// --- sync send ---

Status WebSocketClient::send_text(std::string_view message) {
    if (!is_open()) {
        return Status(StatusCode::kFailedPrecondition, "WebSocket client is not open");
    }

    websocketpp::lib::error_code error;
    storage_->client.send(
        storage_->handle,
        std::string(message),
        websocketpp::frame::opcode::text,
        error);
    if (error) {
        nexus::common::diagnostic_log(
            log::Level::warn, "WebSocket send failed: " + error.message());
        return websocket_status(error.message());
    }

    nexus::common::diagnostic_log(
        log::Level::debug, "WebSocket send bytes=" + std::to_string(message.size()));
    return Status::ok_status();
}

// --- async send ---

void WebSocketClient::async_send_text(std::string_view message, SendHandler h) {
    if (!is_open()) {
        if (h) h(Status(StatusCode::kFailedPrecondition, "WebSocket client is not open"));
        return;
    }

    websocketpp::lib::error_code error;
    storage_->client.send(
        storage_->handle,
        std::string(message),
        websocketpp::frame::opcode::text,
        error);

    nexus::common::diagnostic_log(
        log::Level::debug, "WebSocket async_send bytes=" + std::to_string(message.size()));

    if (error) {
        if (h) h(websocket_status(error.message()));
    } else {
        if (h) h(Status::ok_status());
    }
}

// --- sync receive ---

Result<std::string> WebSocketClient::receive_text() {
    if (!storage_) {
        return Status(StatusCode::kFailedPrecondition, "WebSocket client is not open");
    }

    std::unique_lock<std::mutex> lock(storage_->mutex);
    const auto received = storage_->cv.wait_for(
        lock,
        storage_->options.receive_timeout,
        [this]() {
            return !storage_->messages.empty() || storage_->closed.load();
        });

    if (!received) {
        return websocket_status("WebSocket receive timed out");
    }

    if (storage_->messages.empty()) {
        return websocket_status("WebSocket closed before a message was received");
    }

    auto message = std::move(storage_->messages.front());
    storage_->messages.pop();
    return message;
}

// --- async receive ---

void WebSocketClient::async_receive_text(MessageHandler h) {
    if (!storage_ || storage_->stopped.load()) {
        if (h) h(Status(StatusCode::kFailedPrecondition, "WebSocket client is not open"));
        return;
    }

    std::unique_lock<std::mutex> lock(storage_->mutex);

    // If there's already a queued message, deliver it immediately.
    if (!storage_->messages.empty()) {
        auto message = std::move(storage_->messages.front());
        storage_->messages.pop();
        lock.unlock();
        if (h) h(std::move(message));
        return;
    }

    // If the connection is already closed, report it.
    if (storage_->closed.load()) {
        lock.unlock();
        if (h) h(websocket_status("WebSocket is closed"));
        return;
    }

    storage_->pending_message = std::move(h);
    lock.unlock();
}

// --- sync close ---

Status WebSocketClient::close() {
    if (!storage_ || storage_->stopped.load()) {
        return Status::ok_status();
    }

    storage_->stopped.store(true);

    const auto was_open = is_open();
    if (was_open) {
        websocketpp::lib::error_code error;
        storage_->client.close(
            storage_->handle,
            websocketpp::close::status::normal,
            "",
            error);
        if (error) {
            nexus::common::diagnostic_log(
                log::Level::warn, "WebSocket close failed: " + error.message());
            storage_->client.stop_perpetual();
            storage_->client.stop();
            join_ws_thread(storage_->ws_thread);
            storage_.reset();
            return websocket_status(error.message());
        }

        std::unique_lock<std::mutex> lock(storage_->mutex);
        storage_->cv.wait_for(
            lock,
            storage_->options.close_timeout,
            [this]() {
                return storage_->closed.load();
            });
    }

    storage_->client.stop_perpetual();
    storage_->client.stop();
    join_ws_thread(storage_->ws_thread);
    storage_.reset();
    return Status::ok_status();
}

// --- async close ---

void WebSocketClient::async_close(CloseHandler h) {
    if (!storage_ || storage_->stopped.load()) {
        if (h) h(Status::ok_status());
        return;
    }

    storage_->stopped.store(true);

    if (!is_open()) {
        storage_->client.stop_perpetual();
        storage_->client.stop();
        join_ws_thread(storage_->ws_thread);
        storage_.reset();
        if (h) h(Status::ok_status());
        return;
    }

    websocketpp::lib::error_code error;
    storage_->client.close(
        storage_->handle,
        websocketpp::close::status::normal,
        "",
        error);

    if (error) {
        nexus::common::diagnostic_log(
            log::Level::warn, "WebSocket async_close failed: " + error.message());
        storage_->client.stop_perpetual();
        storage_->client.stop();
        join_ws_thread(storage_->ws_thread);
        storage_.reset();
        if (h) h(websocket_status(error.message()));
        return;
    }

    // Store handler — close_handler in setup_handlers will invoke it.
    std::unique_lock<std::mutex> lock(storage_->mutex);
    storage_->pending_close = std::move(h);
    lock.unlock();
}

} // namespace nexus::net
