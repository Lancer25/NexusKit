#include <nexus/net/websocket_server.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>

#include <websocketpp/server.hpp>
#ifdef NEXUS_NET_HAS_TLS
#include <websocketpp/config/asio.hpp>
#else
#include <websocketpp/config/asio_no_tls.hpp>
#endif

#include <nexus/common/logging.h>
#include <nexus/common/string.h>
#include <nexus/common/thread.h>
#include <nexus/log/logger.h>

namespace nexus::net {

namespace detail {

class WebSocketServerStorage {
public:
#ifdef NEXUS_NET_HAS_TLS
    using WsppConfig = websocketpp::config::asio_tls;
#else
    using WsppConfig = websocketpp::config::asio;
#endif
    using Server = websocketpp::server<WsppConfig>;

    explicit WebSocketServerStorage(WebSocketServerOptions server_options)
        : options(server_options) {
        server.clear_access_channels(websocketpp::log::alevel::all);
        server.clear_error_channels(websocketpp::log::elevel::all);
        server.init_asio();
        server.set_reuse_addr(true);
    }

    Server server;
    WebSocketServerOptions options;

    mutable std::mutex clients_mutex;
    std::unordered_map<WebSocketClientId, websocketpp::connection_hdl> clients;
    std::atomic<WebSocketClientId> next_client_id{1};

    std::atomic<bool> listening{false};
    std::atomic<bool> stopped{false};
    std::thread ws_thread;

    // User callbacks, protected by GS (only accessed from event-loop thread).
    WebSocketServer::ConnectHandler on_connect;
    WebSocketServer::DisconnectHandler on_disconnect;
    WebSocketServer::MessageHandler on_message;

    // Pending listen/close handlers (one-shot).
    WebSocketServer::ListenHandler pending_listen;
    WebSocketServer::CloseHandler pending_close;

    // Per-client close events for synchronous close_client timeout.
    std::mutex close_events_mutex;
    std::unordered_map<WebSocketClientId,
                       std::shared_ptr<nexus::common::Event>> close_events;
};

} // namespace detail

namespace {

Status websocket_status(const std::string& message) {
    return Status(StatusCode::kUnavailable, message);
}

WebSocketClientId assign_client_id(
    std::shared_ptr<detail::WebSocketServerStorage> st,
    websocketpp::connection_hdl hdl) {
    auto id = st->next_client_id.fetch_add(1);
    std::unique_lock<std::mutex> lock(st->clients_mutex);
    st->clients[id] = hdl;
    return id;
}

bool lookup_hdl(
    const std::shared_ptr<detail::WebSocketServerStorage>& st,
    WebSocketClientId client,
    websocketpp::connection_hdl& hdl) {
    std::unique_lock<std::mutex> lock(st->clients_mutex);
    auto it = st->clients.find(client);
    if (it == st->clients.end()) return false;
    hdl = it->second;
    return true;
}

void remove_client(
    std::shared_ptr<detail::WebSocketServerStorage> st,
    WebSocketClientId client) {
    std::unique_lock<std::mutex> lock(st->clients_mutex);
    st->clients.erase(client);
}

void join_ws_thread(std::thread& t) {
    if (t.joinable()) t.join();
}

void setup_handlers(std::shared_ptr<detail::WebSocketServerStorage> st) {
    std::weak_ptr<detail::WebSocketServerStorage> weak = st;

    st->server.set_open_handler([weak](websocketpp::connection_hdl hdl) {
        auto sp = weak.lock();
        if (!sp || sp->stopped.load()) return;

        auto id = assign_client_id(sp, hdl);

        nexus::common::diagnostic_log(
            log::Level::info,
            "WebSocket server client connected id=" + std::to_string(id));

        if (sp->on_connect) sp->on_connect(id);
    });

    st->server.set_fail_handler([weak](websocketpp::connection_hdl /*hdl*/) {
        // Connection handshake failed before open — nothing to clean up.
        auto sp = weak.lock();
        if (!sp) return;
        nexus::common::diagnostic_log(
            log::Level::warn, "WebSocket server connection failed");
    });

    st->server.set_message_handler(
        [weak](websocketpp::connection_hdl hdl,
               detail::WebSocketServerStorage::Server::message_ptr msg) {
            auto sp = weak.lock();
            if (!sp || sp->stopped.load()) return;

            auto payload = msg->get_payload();

            // Find client ID for this connection_hdl.
            WebSocketClientId cid = 0;
            {
                std::unique_lock<std::mutex> lock(sp->clients_mutex);
                for (const auto& pair : sp->clients) {
                    if (pair.second.lock() == hdl.lock()) {
                        cid = pair.first;
                        break;
                    }
                }
            }

            nexus::common::diagnostic_log(
                log::Level::debug,
                "WebSocket server receive id=" + std::to_string(cid) +
                    " bytes=" + std::to_string(payload.size()));

            if (cid != 0 && sp->on_message) {
                sp->on_message(cid, std::move(payload));
            }
        });

    st->server.set_close_handler([weak](websocketpp::connection_hdl hdl) {
        auto sp = weak.lock();
        if (!sp || sp->stopped.load()) return;

        WebSocketClientId cid = 0;
        {
            std::unique_lock<std::mutex> lock(sp->clients_mutex);
            for (auto it = sp->clients.begin(); it != sp->clients.end(); ++it) {
                if (it->second.lock() == hdl.lock()) {
                    cid = it->first;
                    sp->clients.erase(it);
                    break;
                }
            }
        }

        // Signal any pending sync close event.
        {
            std::unique_lock<std::mutex> lock(sp->close_events_mutex);
            auto it = sp->close_events.find(cid);
            if (it != sp->close_events.end()) {
                it->second->emit();
                sp->close_events.erase(it);
            }
        }

        nexus::common::diagnostic_log(
            log::Level::info,
            "WebSocket server client disconnected id=" + std::to_string(cid));

        if (cid != 0 && sp->on_disconnect) sp->on_disconnect(cid);
    });
}

} // namespace

// --- construct / destruct ---

Result<WebSocketServer> WebSocketServer::create(WebSocketServerOptions options) {
    auto storage = std::make_shared<detail::WebSocketServerStorage>(options);
    setup_handlers(storage);

    storage->server.start_perpetual();

    auto* raw = storage.get();
    raw->ws_thread = std::thread([raw]() { raw->server.run(); });

    return WebSocketServer(std::move(storage));
}

WebSocketServer::WebSocketServer(
    std::shared_ptr<detail::WebSocketServerStorage> storage)
    : storage_(std::move(storage)) {}

WebSocketServer::WebSocketServer(WebSocketServer&& other) noexcept
    : storage_(std::move(other.storage_)) {}

WebSocketServer& WebSocketServer::operator=(WebSocketServer&& other) noexcept {
    if (this != &other) {
        this->~WebSocketServer();
        storage_ = std::move(other.storage_);
    }
    return *this;
}

WebSocketServer::~WebSocketServer() {
    close();
    // If async_close was used from a callback, close() returned early
    // without joining the event-loop thread.  Ensure join happens here.
    if (storage_) {
        join_ws_thread(storage_->ws_thread);
    }
}

// --- callbacks ---

void WebSocketServer::set_on_connect(ConnectHandler h) {
    if (storage_) storage_->on_connect = std::move(h);
}

void WebSocketServer::set_on_disconnect(DisconnectHandler h) {
    if (storage_) storage_->on_disconnect = std::move(h);
}

void WebSocketServer::set_on_message(MessageHandler h) {
    if (storage_) storage_->on_message = std::move(h);
}

// --- state ---

bool WebSocketServer::is_listening() const {
    return storage_ && storage_->listening.load() && !storage_->stopped.load();
}

std::size_t WebSocketServer::client_count() const {
    if (!storage_) return 0;
    std::unique_lock<std::mutex> lock(storage_->clients_mutex);
    return storage_->clients.size();
}

// --- sync listen ---

Status WebSocketServer::listen(std::uint16_t port) {
    if (!storage_ || storage_->stopped.load()) {
        return Status(StatusCode::kFailedPrecondition, "WebSocket server is closed");
    }

    if (port == 0) {
        return Status::invalid_argument("WebSocket server port must not be zero");
    }

    nexus::common::Event done;
    Status result(StatusCode::kInternal, "unset");

    async_listen(port, [&](Status s) {
        result = s;
        done.emit();
    });

    done.wait(5000);

    if (!result.ok() && result.code() == StatusCode::kInternal) {
        return websocket_status("WebSocket server listen timed out");
    }

    return result;
}

// --- async listen ---

void WebSocketServer::async_listen(std::uint16_t port, ListenHandler h) {
    if (!storage_ || storage_->stopped.load()) {
        if (h) h(Status(StatusCode::kFailedPrecondition, "WebSocket server is closed"));
        return;
    }

    if (port == 0) {
        if (h) h(Status::invalid_argument("WebSocket server port must not be zero"));
        return;
    }

    // Post the listen to the event-loop thread.
    std::weak_ptr<detail::WebSocketServerStorage> weak = storage_;
    storage_->server.get_io_service().post([weak, port, h = std::move(h)]() mutable {
        auto sp = weak.lock();
        if (!sp || sp->stopped.load()) {
            if (h) h(Status(StatusCode::kFailedPrecondition, "WebSocket server is closed"));
            return;
        }

        websocketpp::lib::error_code ec;
        sp->server.listen(asio::ip::tcp::endpoint(
            asio::ip::tcp::v4(), port), ec);
        if (ec) {
            nexus::common::diagnostic_log(
                log::Level::warn, "WebSocket server listen failed: " + ec.message());
            if (h) h(websocket_status(ec.message()));
            return;
        }

        sp->server.start_accept();
        sp->listening.store(true);

        nexus::common::diagnostic_log(
            log::Level::info, "WebSocket server listening on port " + std::to_string(port));

        if (h) h(Status::ok_status());
    });
}

// --- sync send ---

Status WebSocketServer::send_text(ClientId client, std::string_view message) {
    if (!is_listening()) {
        return Status(StatusCode::kFailedPrecondition, "WebSocket server is not listening");
    }

    websocketpp::connection_hdl hdl;
    if (!lookup_hdl(storage_, client, hdl)) {
        return Status::not_found("WebSocket client not found: " + std::to_string(client));
    }

    websocketpp::lib::error_code error;
    storage_->server.send(hdl, std::string(message),
                          websocketpp::frame::opcode::text, error);
    if (error) {
        nexus::common::diagnostic_log(
            log::Level::warn, "WebSocket server send failed: " + error.message());
        return websocket_status(error.message());
    }

    nexus::common::diagnostic_log(
        log::Level::debug,
        "WebSocket server send id=" + std::to_string(client) +
            " bytes=" + std::to_string(message.size()));
    return Status::ok_status();
}

// --- async send ---

void WebSocketServer::async_send_text(ClientId client,
                                       std::string_view message,
                                       SendHandler h) {
    if (!is_listening()) {
        if (h) h(Status(StatusCode::kFailedPrecondition, "WebSocket server is not listening"));
        return;
    }

    websocketpp::connection_hdl hdl;
    if (!lookup_hdl(storage_, client, hdl)) {
        if (h) h(Status::not_found("WebSocket client not found: " + std::to_string(client)));
        return;
    }

    websocketpp::lib::error_code error;
    storage_->server.send(hdl, std::string(message),
                          websocketpp::frame::opcode::text, error);

    nexus::common::diagnostic_log(
        log::Level::debug,
        "WebSocket server async_send id=" + std::to_string(client) +
            " bytes=" + std::to_string(message.size()));

    if (error) {
        if (h) h(websocket_status(error.message()));
    } else {
        if (h) h(Status::ok_status());
    }
}

// --- sync close client ---

Status WebSocketServer::close_client(ClientId client) {
    if (!storage_ || storage_->stopped.load()) {
        return Status::ok_status();
    }

    websocketpp::connection_hdl hdl;
    if (!lookup_hdl(storage_, client, hdl)) {
        return Status::not_found("WebSocket client not found: " + std::to_string(client));
    }

    auto event = std::make_shared<nexus::common::Event>();
    {
        std::unique_lock<std::mutex> lock(storage_->close_events_mutex);
        storage_->close_events[client] = event;
    }

    websocketpp::lib::error_code error;
    storage_->server.close(hdl, websocketpp::close::status::normal, "", error);
    if (error) {
        std::unique_lock<std::mutex> lock(storage_->close_events_mutex);
        storage_->close_events.erase(client);
        nexus::common::diagnostic_log(
            log::Level::warn, "WebSocket server close_client failed: " + error.message());
        return websocket_status(error.message());
    }

    // Wait for close handshake.
    auto timeout_ms = static_cast<int>(storage_->options.close_timeout.count());
    if (timeout_ms <= 0) timeout_ms = 1000;
    if (!event->wait(timeout_ms)) {
        // Timeout — force remove.
        std::unique_lock<std::mutex> lock(storage_->close_events_mutex);
        storage_->close_events.erase(client);
        remove_client(storage_, client);
        return websocket_status("WebSocket server close_client timed out");
    }

    return Status::ok_status();
}

// --- async close client ---

void WebSocketServer::async_close_client(ClientId client, CloseHandler h) {
    if (!storage_ || storage_->stopped.load()) {
        if (h) h(Status::ok_status());
        return;
    }

    websocketpp::connection_hdl hdl;
    if (!lookup_hdl(storage_, client, hdl)) {
        if (h) h(Status::ok_status());
        return;
    }

    websocketpp::lib::error_code error;
    storage_->server.close(hdl, websocketpp::close::status::normal, "", error);

    remove_client(storage_, client);

    if (error) {
        if (h) h(websocket_status(error.message()));
    } else {
        if (h) h(Status::ok_status());
    }
}

// --- sync close ---

Status WebSocketServer::close() {
    if (!storage_ || storage_->stopped.load()) {
        return Status::ok_status();
    }

    storage_->stopped.store(true);
    storage_->listening.store(false);

    // Close all known clients.
    std::vector<websocketpp::connection_hdl> to_close;
    {
        std::unique_lock<std::mutex> lock(storage_->clients_mutex);
        to_close.reserve(storage_->clients.size());
        for (const auto& pair : storage_->clients) {
            to_close.push_back(pair.second);
        }
    }

    websocketpp::lib::error_code ignored;
    for (const auto& hdl : to_close) {
        storage_->server.close(hdl, websocketpp::close::status::normal, "", ignored);
    }

    storage_->server.stop_listening(ignored);
    storage_->server.stop_perpetual();
    storage_->server.stop();

    join_ws_thread(storage_->ws_thread);

    {
        std::unique_lock<std::mutex> lock(storage_->clients_mutex);
        storage_->clients.clear();
    }

    storage_.reset();
    return Status::ok_status();
}

// --- async close ---

void WebSocketServer::async_close(CloseHandler h) {
    if (!storage_ || storage_->stopped.load()) {
        if (h) h(Status::ok_status());
        return;
    }

    storage_->stopped.store(true);
    storage_->listening.store(false);

    // Close all known clients.
    {
        std::unique_lock<std::mutex> lock(storage_->clients_mutex);
        websocketpp::lib::error_code ignored;
        for (const auto& pair : storage_->clients) {
            storage_->server.close(pair.second,
                                    websocketpp::close::status::normal,
                                    "", ignored);
        }
        storage_->clients.clear();
    }

    websocketpp::lib::error_code ignored;
    storage_->server.stop_listening(ignored);
    storage_->server.stop_perpetual();
    storage_->server.stop();
    // Don't join here — may be called from event-loop thread.
    // Destructor's close() path handles joining.

    if (h) h(Status::ok_status());
}

} // namespace nexus::net
