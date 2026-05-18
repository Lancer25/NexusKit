#include <nexus/net/http.h>

#include <cctype>
#include <exception>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <utility>


#include <asio.hpp>
#include <httplib.h>

#include <nexus/common/logging.h>
#include <nexus/common/string.h>
#include <nexus/common/thread.h>
#include <nexus/log/logger.h>

namespace nexus::net {

namespace detail {

class HttpClientStorage {
public:
    HttpClientStorage(std::string url, HttpClientOptions opts)
        : work(asio::make_work_guard(io)),
          base_url(std::move(url)),
          options(opts) {
#ifdef NEXUS_NET_HAS_TLS
        if (nexus::common::starts_with(base_url, "https://")) {
            is_tls_ = true;
            tls_client_ = std::make_unique<httplib::SSLClient>(base_url);
        } else {
            client_ = std::make_unique<httplib::Client>(base_url);
        }
#else
        client_ = std::make_unique<httplib::Client>(base_url);
#endif
        with_client([&](auto& c) {
            c.set_connection_timeout(options.connect_timeout);
            c.set_read_timeout(options.read_timeout);
            c.set_write_timeout(options.write_timeout);
            c.set_follow_location(options.follow_redirects);
        });
    }

    template <typename Func>
    auto with_client(Func&& f) -> decltype(auto) {
#ifdef NEXUS_NET_HAS_TLS
        if (is_tls_) {
            return f(*tls_client_);
        }
#endif
        return f(*client_);
    }

    template <typename Func>
    auto with_client(Func&& f) const -> decltype(auto) {
#ifdef NEXUS_NET_HAS_TLS
        if (is_tls_) {
            return f(*tls_client_);
        }
#endif
        return f(*client_);
    }

    asio::io_context io;
    asio::executor_work_guard<asio::io_context::executor_type> work;
    nexus::common::Thread worker;
    std::atomic<bool> open{false};
    std::atomic<bool> stopped{false};

    std::string base_url;
    HttpClientOptions options;

private:
    bool is_tls_ = false;
    std::unique_ptr<httplib::Client> client_;
#ifdef NEXUS_NET_HAS_TLS
    std::unique_ptr<httplib::SSLClient> tls_client_;
#endif
};

} // namespace detail

namespace {

Status validate_base_url(std::string_view base_url) {
    bool is_https = false;
    if (nexus::common::starts_with(base_url, "https://")) {
#ifdef NEXUS_NET_HAS_TLS
        is_https = true;
#else
        return Status::invalid_argument(
            "https:// base URLs require TLS support; rebuild with OpenSSL");
#endif
    } else if (nexus::common::starts_with(base_url, "http://")) {
        // plain HTTP
    } else {
        return Status::invalid_argument("Only http:// or https:// base URLs are supported");
    }

    const auto authority = is_https ? base_url.substr(8) : base_url.substr(7);
    if (authority.empty()) {
        return Status::invalid_argument("HTTP base URL must contain a host");
    }

    if (authority.find('/') != std::string_view::npos) {
        return Status::invalid_argument("HTTP base URL must not contain a path");
    }

    if (nexus::common::contains_space(authority)) {
        return Status::invalid_argument("HTTP base URL must not contain whitespace");
    }

    return Status::ok_status();
}

std::string normalize_path(std::string_view path) {
    if (path.empty()) {
        return "/";
    }

    if (path.front() == '/') {
        return std::string(path);
    }

    return "/" + std::string(path);
}

Status error_to_status(httplib::Error error) {
    switch (error) {
    case httplib::Error::Connection:
        return Status(StatusCode::kUnavailable, "HTTP connection failed");
    case httplib::Error::Read:
        return Status(StatusCode::kUnavailable, "HTTP read failed");
    case httplib::Error::Write:
        return Status(StatusCode::kUnavailable, "HTTP write failed");
    case httplib::Error::ExceedRedirectCount:
        return Status(StatusCode::kFailedPrecondition, "HTTP redirect limit exceeded");
    case httplib::Error::Canceled:
        return Status(StatusCode::kCancelled, "HTTP request was cancelled");
    case httplib::Error::UnsupportedMultipartBoundaryChars:
    case httplib::Error::Compression:
    case httplib::Error::ConnectionTimeout:
        return Status(StatusCode::kUnavailable, "HTTP request failed");
    case httplib::Error::Success:
        return Status::internal("HTTP backend reported success without a response");
    case httplib::Error::Unknown:
    default:
        return Status(StatusCode::kUnknown, "HTTP request failed");
    }
}

std::string request_prefix(std::string_view method, std::string_view path) {
    return "HTTP " + std::string(method) + " " + normalize_path(path);
}

Status log_and_return_failure(std::string_view operation, httplib::Error error) {
    const auto status = error_to_status(error);
    nexus::common::diagnostic_log(log::Level::warn, std::string(operation) + " failed: " + status.message());
    return status;
}

httplib::Headers to_backend_headers(const std::vector<HttpHeader>& headers) {
    httplib::Headers backend_headers;
    for (const auto& header : headers) {
        backend_headers.emplace(header.name, header.value);
    }
    return backend_headers;
}

HttpResponse to_response(const httplib::Response& backend_response) {
    HttpResponse response;
    response.status_code = backend_response.status;
    response.body = backend_response.body;
    response.headers.reserve(backend_response.headers.size());

    for (const auto& header : backend_response.headers) {
        response.headers.push_back(HttpHeader{header.first, header.second});
    }

    return response;
}

HttpResponse log_and_return_response(std::string_view operation, const httplib::Response& backend_response) {
    auto response = to_response(backend_response);
    nexus::common::diagnostic_log(
        log::Level::info,
        std::string(operation) +
            " status=" + std::to_string(response.status_code) +
            " bytes=" + std::to_string(response.body.size()));
    return response;
}

std::shared_ptr<detail::HttpClientStorage> make_storage(
    std::string base_url,
    HttpClientOptions options) {
    auto storage = std::make_shared<detail::HttpClientStorage>(std::move(base_url), options);
    auto* raw = storage.get();
    raw->worker.start([raw] { raw->io.run(); });
    raw->open.store(true);
    return storage;
}

std::shared_ptr<detail::HttpClientStorage> copy_storage(
    const std::shared_ptr<detail::HttpClientStorage>& src) {
    if (!src) return nullptr;
    return make_storage(src->base_url, src->options);
}

void stop_storage(detail::HttpClientStorage& st) {
    st.stopped.store(true);
    st.open.store(false);
    st.io.stop();
    st.work.reset();
    st.worker.stop();
}

} // namespace

bool HttpResponse::ok() const {
    return status_code >= 200 && status_code < 300;
}

Result<std::string> HttpResponse::header(std::string_view name) const {
    for (const auto& item : headers) {
        if (nexus::common::equals_ignore_case(item.name, name)) {
            return item.value;
        }
    }

    return Status::not_found("HTTP response header not found: " + std::string(name));
}

HttpClient::HttpClient(const HttpClient& other)
    : storage_(copy_storage(other.storage_)) {}

HttpClient::HttpClient(HttpClient&& other) noexcept = default;

HttpClient& HttpClient::operator=(const HttpClient& other) {
    if (this != &other) {
        if (storage_) stop_storage(*storage_);
        storage_ = copy_storage(other.storage_);
    }
    return *this;
}

HttpClient& HttpClient::operator=(HttpClient&& other) noexcept = default;

HttpClient::~HttpClient() {
    if (storage_) {
        stop_storage(*storage_);
    }
}

Result<HttpClient> HttpClient::create(std::string base_url, HttpClientOptions options) {
    const auto status = validate_base_url(base_url);
    if (!status.ok()) {
        return status;
    }

    try {
        nexus::common::diagnostic_log(log::Level::debug, "HTTP client create base_url=" + base_url);
        return HttpClient(make_storage(std::move(base_url), options));
    } catch (const std::exception& error) {
        nexus::common::diagnostic_log(log::Level::warn, std::string("HTTP client create failed: ") + error.what());
        return Status::internal(error.what());
    }
}

bool HttpClient::is_open() const {
    return storage_ && storage_->open.load() && !storage_->stopped.load();
}

Result<HttpResponse> HttpClient::get(
    std::string_view path,
    const std::vector<HttpHeader>& headers) const {
    if (!storage_ || storage_->stopped.load()) {
        return Status(StatusCode::kFailedPrecondition, "HTTP client is closed");
    }
    const auto operation = request_prefix("GET", path);
    nexus::common::diagnostic_log(log::Level::debug, operation);

    const auto result = storage_->with_client([&](auto& cli) {
        return cli.Get(normalize_path(path), to_backend_headers(headers));
    });
    if (!result) {
        return log_and_return_failure(operation, result.error());
    }

    return log_and_return_response(operation, *result);
}

Result<HttpResponse> HttpClient::post(
    std::string_view path,
    std::string_view body,
    std::string_view content_type,
    const std::vector<HttpHeader>& headers) const {
    if (!storage_ || storage_->stopped.load()) {
        return Status(StatusCode::kFailedPrecondition, "HTTP client is closed");
    }
    const auto operation = request_prefix("POST", path);
    nexus::common::diagnostic_log(log::Level::debug, operation + " bytes=" + std::to_string(body.size()));

    const auto result = storage_->with_client([&](auto& cli) {
        return cli.Post(
            normalize_path(path),
            to_backend_headers(headers),
            std::string(body),
            std::string(content_type));
    });

    if (!result) {
        return log_and_return_failure(operation, result.error());
    }

    return log_and_return_response(operation, *result);
}

Result<HttpResponse> HttpClient::put(
    std::string_view path,
    std::string_view body,
    std::string_view content_type,
    const std::vector<HttpHeader>& headers) const {
    if (!storage_ || storage_->stopped.load()) {
        return Status(StatusCode::kFailedPrecondition, "HTTP client is closed");
    }
    const auto operation = request_prefix("PUT", path);
    nexus::common::diagnostic_log(log::Level::debug, operation + " bytes=" + std::to_string(body.size()));

    const auto result = storage_->with_client([&](auto& cli) {
        return cli.Put(
            normalize_path(path),
            to_backend_headers(headers),
            std::string(body),
            std::string(content_type));
    });

    if (!result) {
        return log_and_return_failure(operation, result.error());
    }

    return log_and_return_response(operation, *result);
}

Result<HttpResponse> HttpClient::del(
    std::string_view path,
    const std::vector<HttpHeader>& headers) const {
    if (!storage_ || storage_->stopped.load()) {
        return Status(StatusCode::kFailedPrecondition, "HTTP client is closed");
    }
    const auto operation = request_prefix("DELETE", path);
    nexus::common::diagnostic_log(log::Level::debug, operation);

    const auto result = storage_->with_client([&](auto& cli) {
        return cli.Delete(normalize_path(path), to_backend_headers(headers));
    });
    if (!result) {
        return log_and_return_failure(operation, result.error());
    }

    return log_and_return_response(operation, *result);
}

namespace {

template <typename RequestFunc>
void schedule_request(
    std::shared_ptr<detail::HttpClientStorage> storage,
    std::string_view path,
    std::string_view method_name,
    HttpClient::RequestHandler handler,
    RequestFunc do_request) {
    if (!storage || storage->stopped.load()) {
        handler(Status(StatusCode::kFailedPrecondition, "HTTP client is closed"));
        return;
    }

    std::weak_ptr<detail::HttpClientStorage> weak = storage;
    auto path_str = normalize_path(path);
    storage->io.post([weak, path_str, mn = std::string(method_name),
                      handler = std::move(handler),
                      do_request = std::move(do_request)]() mutable {
        auto st = weak.lock();
        if (!st || st->stopped.load()) {
            handler(Status(StatusCode::kFailedPrecondition, "HTTP client is closed"));
            return;
        }

        nexus::common::diagnostic_log(
            log::Level::debug,
            "HTTP " + mn + " " + path_str);

#ifdef NEXUS_NET_HAS_TLS
        if (nexus::common::starts_with(st->base_url, "https://")) {
            httplib::SSLClient client(st->base_url);
            client.set_connection_timeout(st->options.connect_timeout);
            client.set_read_timeout(st->options.read_timeout);
            client.set_write_timeout(st->options.write_timeout);
            client.set_follow_location(st->options.follow_redirects);
            auto hresult = do_request(client, path_str);
            if (!hresult) {
                handler(error_to_status(hresult.error()));
                return;
            }
            handler(to_response(*hresult));
            return;
        }
#endif
        httplib::Client client(st->base_url);
        client.set_connection_timeout(st->options.connect_timeout);
        client.set_read_timeout(st->options.read_timeout);
        client.set_write_timeout(st->options.write_timeout);
        client.set_follow_location(st->options.follow_redirects);

        auto hresult = do_request(client, path_str);
        if (!hresult) {
            handler(error_to_status(hresult.error()));
            return;
        }
        handler(to_response(*hresult));
    });
}

} // namespace

void HttpClient::async_get(
    std::string_view path,
    RequestHandler handler,
    const std::vector<HttpHeader>& headers) {
    schedule_request(storage_, path, "GET", std::move(handler),
        [headers](auto& c, const std::string& p) {
            return c.Get(p, to_backend_headers(headers));
        });
}

void HttpClient::async_post(
    std::string_view path,
    std::string_view body,
    std::string_view content_type,
    RequestHandler handler,
    const std::vector<HttpHeader>& headers) {
    schedule_request(storage_, path, "POST", std::move(handler),
        [body = std::string(body), ct = std::string(content_type),
         headers](auto& c, const std::string& p) {
            return c.Post(p, to_backend_headers(headers), body, ct);
        });
}

void HttpClient::async_put(
    std::string_view path,
    std::string_view body,
    std::string_view content_type,
    RequestHandler handler,
    const std::vector<HttpHeader>& headers) {
    schedule_request(storage_, path, "PUT", std::move(handler),
        [body = std::string(body), ct = std::string(content_type),
         headers](auto& c, const std::string& p) {
            return c.Put(p, to_backend_headers(headers), body, ct);
        });
}

void HttpClient::async_del(
    std::string_view path,
    RequestHandler handler,
    const std::vector<HttpHeader>& headers) {
    schedule_request(storage_, path, "DELETE", std::move(handler),
        [headers](auto& c, const std::string& p) {
            return c.Delete(p, to_backend_headers(headers));
        });
}

void HttpClient::async_close(CloseHandler handler) {
    if (!storage_ || storage_->stopped.load()) {
        if (handler) handler(Status::ok_status());
        return;
    }

    storage_->stopped.store(true);
    storage_->open.store(false);

    std::weak_ptr<detail::HttpClientStorage> weak = storage_;
    storage_->io.post([weak, handler = std::move(handler)]() mutable {
        auto st = weak.lock();
        if (st) {
            st->io.stop();
            st->work.reset();
        }
        if (handler) handler(Status::ok_status());
    });
}

HttpClient::HttpClient(std::shared_ptr<detail::HttpClientStorage> storage)
    : storage_(std::move(storage)) {}

std::string build_query_path(
    std::string_view path,
    const std::vector<HttpQueryParameter>& parameters) {
    auto result = normalize_path(path);
    if (parameters.empty()) {
        return result;
    }

    result += result.find('?') == std::string::npos ? '?' : '&';

    bool first = true;
    for (const auto& parameter : parameters) {
        if (!first) {
            result += '&';
        }
        first = false;
        result += nexus::common::percent_encode(parameter.name);
        result += '=';
        result += nexus::common::percent_encode(parameter.value);
    }

    return result;
}

} // namespace nexus::net
