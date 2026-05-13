#pragma once

#include <memory>
#include <string>
#include <string_view>

#include <nexus/common/export.h>
#include <nexus/core/result.h>
#include <nexus/core/status.h>

namespace nexus::common {

namespace detail {
class XmlDocumentStorage;
class XmlNodeStorage;
}

class XmlNode;

class NEXUS_COMMON_API XmlDocument {
public:
    XmlDocument();
    XmlDocument(const XmlDocument& other);
    XmlDocument(XmlDocument&& other) noexcept;
    XmlDocument& operator=(const XmlDocument& other);
    XmlDocument& operator=(XmlDocument&& other) noexcept;
    ~XmlDocument();

    static XmlDocument create(std::string_view root_name);

    Result<XmlNode> root() const;
    std::string dump() const;

private:
    friend NEXUS_COMMON_API Result<XmlDocument> parse_xml(std::string_view text);
    friend class XmlNode;

    explicit XmlDocument(std::shared_ptr<detail::XmlDocumentStorage> storage);

    std::shared_ptr<detail::XmlDocumentStorage> storage_;
};

class NEXUS_COMMON_API XmlNode {
public:
    XmlNode();
    XmlNode(const XmlNode& other);
    XmlNode(XmlNode&& other) noexcept;
    XmlNode& operator=(const XmlNode& other);
    XmlNode& operator=(XmlNode&& other) noexcept;
    ~XmlNode();

    std::string name() const;
    Result<std::string> text() const;
    Result<std::string> attribute(std::string_view name) const;
    Result<XmlNode> child(std::string_view name) const;
    Result<XmlNode> append_child(std::string_view name);
    Status set_text(std::string_view text);
    Status set_attribute(std::string_view name, std::string_view value);

private:
    friend class XmlDocument;

    explicit XmlNode(std::shared_ptr<detail::XmlNodeStorage> storage);

    std::shared_ptr<detail::XmlNodeStorage> storage_;
};

NEXUS_COMMON_API Result<XmlDocument> parse_xml(std::string_view text);

} // namespace nexus::common
