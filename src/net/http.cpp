#include <nexus/net/http.h>

#include <cctype>
#include <exception>
#include <memory>
#include <string>
#include <utility>

#include <httplib.h>

namespace nexus::net {

namespace detail {

class HttpClientStorage {
public:
    HttpClientStorage(std::string url, HttpClientOptions options)
        : base_url(std::move(url)), client(base_url), options(options) {
        client.set_connection_timeout(options.connect_timeout);
        client.set_read_timeout(options.read_timeout);
        client.set_write_timeout(options.write_timeout);
    }

    std::string base_url;
    httplib::Client client;
    HttpClientOptions options;
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

Status validate_base_url(std::string_view base_url) {
    if (!starts_with(base_url, "http://")) {
        return Status::invalid_argument("Only http:// base URLs are supported");
    }

    const auto authority = base_url.substr(7);
    if (authority.empty()) {
        return Status::invalid_argument("HTTP base URL must contain a host");
    }

    if (authority.find('/') != std::string_view::npos) {
        return Status::invalid_argument("HTTP base URL must not contain a path");
    }

    if (contains_space(authority)) {
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

std::shared_ptr<detail::HttpClientStorage> make_storage(
    std::string base_url,
    HttpClientOptions options) {
    return std::make_shared<detail::HttpClientStorage>(std::move(base_url), options);
}

} // namespace

HttpClient::HttpClient(const HttpClient& other)
    : storage_(make_storage(other.storage_->base_url, other.storage_->options)) {}

HttpClient::HttpClient(HttpClient&& other) noexcept = default;

HttpClient& HttpClient::operator=(const HttpClient& other) {
    if (this != &other) {
        storage_ = make_storage(other.storage_->base_url, other.storage_->options);
    }
    return *this;
}

HttpClient& HttpClient::operator=(HttpClient&& other) noexcept = default;

HttpClient::~HttpClient() = default;

Result<HttpClient> HttpClient::create(std::string base_url, HttpClientOptions options) {
    const auto status = validate_base_url(base_url);
    if (!status.ok()) {
        return status;
    }

    try {
        return HttpClient(make_storage(std::move(base_url), options));
    } catch (const std::exception& error) {
        return Status::internal(error.what());
    }
}

Result<HttpResponse> HttpClient::get(
    std::string_view path,
    const std::vector<HttpHeader>& headers) const {
    const auto result = storage_->client.Get(normalize_path(path), to_backend_headers(headers));
    if (!result) {
        return error_to_status(result.error());
    }

    return to_response(*result);
}

Result<HttpResponse> HttpClient::post(
    std::string_view path,
    std::string_view body,
    std::string_view content_type,
    const std::vector<HttpHeader>& headers) const {
    const auto result = storage_->client.Post(
        normalize_path(path),
        to_backend_headers(headers),
        std::string(body),
        std::string(content_type));

    if (!result) {
        return error_to_status(result.error());
    }

    return to_response(*result);
}

HttpClient::HttpClient(std::shared_ptr<detail::HttpClientStorage> storage)
    : storage_(std::move(storage)) {}

} // namespace nexus::net
