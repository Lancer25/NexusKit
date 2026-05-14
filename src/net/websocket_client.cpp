#include <nexus/net/websocket_client.h>

#include <cctype>
#include <condition_variable>
#include <exception>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <utility>

#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>

#include <nexus/log/logger.h>

namespace nexus::net {

namespace detail {

class WebSocketClientStorage {
public:
    using Client = websocketpp::client<websocketpp::config::asio_client>;

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
    std::thread thread;
    bool open = false;
    bool closed = false;
    Status failure = Status::ok_status();
};

} // namespace detail

namespace {

bool starts_with(std::string_view value, std::string_view prefix) {
    return value.size() >= prefix.size() && value.substr(0, prefix.size()) == prefix;
}

bool contains_space(std::string_view value) {
    for (const auto character : value) {
        if (std::isspace(static_cast<unsigned char>(character)) != 0) {
            return true;
        }
    }
    return false;
}

Status validate_url(std::string_view url) {
    if (!starts_with(url, "ws://")) {
        return Status::invalid_argument("WebSocket URL must start with ws://");
    }

    const auto authority = url.substr(5);
    if (authority.empty()) {
        return Status::invalid_argument("WebSocket URL must contain a host");
    }

    if (contains_space(authority)) {
        return Status::invalid_argument("WebSocket URL must not contain whitespace");
    }

    return Status::ok_status();
}

Status websocket_status(const std::string& message) {
    return Status(StatusCode::kUnavailable, message);
}

void diagnostic_log(log::Level level, const std::string& message) {
    log::write(level, message);
}

void join_client_thread(detail::WebSocketClientStorage& storage) {
    if (storage.thread.joinable()) {
        storage.thread.join();
    }
}

} // namespace

WebSocketClient::WebSocketClient(WebSocketClient&& other) noexcept = default;

WebSocketClient& WebSocketClient::operator=(WebSocketClient&& other) noexcept = default;

WebSocketClient::~WebSocketClient() {
    close();
}

Result<WebSocketClient> WebSocketClient::connect(
    std::string url,
    WebSocketClientOptions options) {
    const auto status = validate_url(url);
    if (!status.ok()) {
        return status;
    }

    auto storage = std::make_unique<detail::WebSocketClientStorage>(options);
    auto* raw_storage = storage.get();

    raw_storage->client.set_open_handler([raw_storage](websocketpp::connection_hdl handle) {
        {
            std::lock_guard<std::mutex> lock(raw_storage->mutex);
            raw_storage->handle = handle;
            raw_storage->open = true;
            raw_storage->closed = false;
        }
        raw_storage->cv.notify_all();
        diagnostic_log(log::Level::info, "WebSocket connected");
    });

    raw_storage->client.set_message_handler(
        [raw_storage](
            websocketpp::connection_hdl,
            detail::WebSocketClientStorage::Client::message_ptr message) {
            {
                std::lock_guard<std::mutex> lock(raw_storage->mutex);
                raw_storage->messages.push(message->get_payload());
            }
            raw_storage->cv.notify_all();
            diagnostic_log(
                log::Level::debug,
                "WebSocket receive bytes=" + std::to_string(message->get_payload().size()));
        });

    raw_storage->client.set_fail_handler([raw_storage](websocketpp::connection_hdl handle) {
        auto connection = raw_storage->client.get_con_from_hdl(handle);
        const auto message = connection ? connection->get_ec().message() : std::string("connect failed");
        {
            std::lock_guard<std::mutex> lock(raw_storage->mutex);
            raw_storage->open = false;
            raw_storage->closed = true;
            raw_storage->failure = websocket_status(message);
        }
        raw_storage->cv.notify_all();
        diagnostic_log(log::Level::warn, "WebSocket failed: " + message);
    });

    raw_storage->client.set_close_handler([raw_storage](websocketpp::connection_hdl) {
        {
            std::lock_guard<std::mutex> lock(raw_storage->mutex);
            raw_storage->open = false;
            raw_storage->closed = true;
        }
        raw_storage->cv.notify_all();
        diagnostic_log(log::Level::debug, "WebSocket closed");
    });

    websocketpp::lib::error_code error;
    auto connection = raw_storage->client.get_connection(url, error);
    if (error) {
        return websocket_status(error.message());
    }

    raw_storage->handle = connection->get_handle();
    diagnostic_log(log::Level::debug, "WebSocket connect " + url);
    raw_storage->client.connect(connection);
    raw_storage->thread = std::thread([raw_storage]() {
        raw_storage->client.run();
    });

    std::unique_lock<std::mutex> lock(raw_storage->mutex);
    const auto connected = raw_storage->cv.wait_for(
        lock,
        options.connect_timeout,
        [raw_storage]() {
            return raw_storage->open || raw_storage->closed || !raw_storage->failure.ok();
        });

    if (!connected) {
        lock.unlock();
        raw_storage->client.stop();
        join_client_thread(*raw_storage);
        return websocket_status("WebSocket connect timed out");
    }

    if (!raw_storage->open) {
        const auto failure = raw_storage->failure.ok()
            ? websocket_status("WebSocket connect failed")
            : raw_storage->failure;
        lock.unlock();
        raw_storage->client.stop();
        join_client_thread(*raw_storage);
        return failure;
    }

    return WebSocketClient(std::move(storage));
}

bool WebSocketClient::is_open() const {
    if (!storage_) {
        return false;
    }

    std::lock_guard<std::mutex> lock(storage_->mutex);
    return storage_->open;
}

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
        diagnostic_log(log::Level::warn, "WebSocket send failed: " + error.message());
        return websocket_status(error.message());
    }

    diagnostic_log(log::Level::debug, "WebSocket send bytes=" + std::to_string(message.size()));
    return Status::ok_status();
}

Result<std::string> WebSocketClient::receive_text() {
    if (!storage_) {
        return Status(StatusCode::kFailedPrecondition, "WebSocket client is not open");
    }

    std::unique_lock<std::mutex> lock(storage_->mutex);
    const auto received = storage_->cv.wait_for(
        lock,
        storage_->options.receive_timeout,
        [this]() {
            return !storage_->messages.empty() || storage_->closed;
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

Status WebSocketClient::close() {
    if (!storage_) {
        return Status::ok_status();
    }

    const auto was_open = is_open();
    if (was_open) {
        websocketpp::lib::error_code error;
        storage_->client.close(
            storage_->handle,
            websocketpp::close::status::normal,
            "",
            error);
        if (error) {
            diagnostic_log(log::Level::warn, "WebSocket close failed: " + error.message());
            storage_->client.stop();
            join_client_thread(*storage_);
            storage_.reset();
            return websocket_status(error.message());
        }

        std::unique_lock<std::mutex> lock(storage_->mutex);
        storage_->cv.wait_for(
            lock,
            storage_->options.close_timeout,
            [this]() {
                return storage_->closed;
            });
    }

    storage_->client.stop();
    join_client_thread(*storage_);
    storage_.reset();
    return Status::ok_status();
}

WebSocketClient::WebSocketClient(std::unique_ptr<detail::WebSocketClientStorage> storage)
    : storage_(std::move(storage)) {}

} // namespace nexus::net
