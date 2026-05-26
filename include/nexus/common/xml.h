#pragma once

#include <memory>
#include <string>
#include <string_view>

#include <nexus/common/export.h>
#include <nexus/core/result.h>
#include <nexus/core/status.h>

/// XML parsing and document manipulation.
///
/// The current backend is pugixml, included only from the implementation file.
/// Users of `nexus_common` do not need to link against pugixml.
namespace nexus::common {

namespace detail {
class XmlDocumentStorage;
class XmlNodeStorage;
}

class XmlNode;

/// A copyable XML document handle backed by a shared storage.
///
/// Use `create()` to build a new document programmatically or `parse_xml()` to
/// load from text.  Access the root node via `root()` and serialize the whole
/// document with `dump()`.
///
/// Copy and move are shallow and cheap (shared pointer semantics).  A moved-from
/// document is empty; `root()` returns `kFailedPrecondition`.
class NEXUS_COMMON_API XmlDocument {
public:
    /// Constructs an empty document (no root element).
    XmlDocument();
    /// Copies an XML document handle.
    XmlDocument(const XmlDocument& other);
    /// Moves an XML document handle.
    XmlDocument(XmlDocument&& other) noexcept;
    /// Copies an XML document handle.
    XmlDocument& operator=(const XmlDocument& other);
    /// Moves an XML document handle.
    XmlDocument& operator=(XmlDocument&& other) noexcept;
    /// Releases this XML document handle.
    ~XmlDocument();

    /// Creates a new document with the given root element.
    static XmlDocument create(std::string_view root_name);

    /// Returns the root element of this document.
    ///
    /// @return The root node, or `kFailedPrecondition` when the document is
    /// empty (default-constructed or moved-from).
    Result<XmlNode> root() const;

    /// Serializes the document as compact XML text.
    ///
    /// Uses pugixml's built-in writer with default (compact) formatting.
    /// Returns an empty string for an empty document.
    std::string dump() const;

private:
    friend NEXUS_COMMON_API Result<XmlDocument> parse_xml(std::string_view text);
    friend class XmlNode;

    explicit XmlDocument(std::shared_ptr<detail::XmlDocumentStorage> storage);

    std::shared_ptr<detail::XmlDocumentStorage> storage_;
};

/// A copyable XML element node handle.
///
/// Provides access to the node name, text content, attributes, and children.
/// Mutation helpers `set_text()`, `set_attribute()`, and `append_child()`
/// modify the underlying shared document.
///
/// A default-constructed or moved-from node is "empty".  Operations on an empty
/// node (except `name()`, copy, and assignment) return `kFailedPrecondition`.
///
/// Copy and move are shallow and cheap (shared pointer semantics).
class NEXUS_COMMON_API XmlNode {
public:
    /// Constructs an empty node.
    XmlNode();
    /// Copies an XML node handle.
    XmlNode(const XmlNode& other);
    /// Moves an XML node handle.
    XmlNode(XmlNode&& other) noexcept;
    /// Copies an XML node handle.
    XmlNode& operator=(const XmlNode& other);
    /// Moves an XML node handle.
    XmlNode& operator=(XmlNode&& other) noexcept;
    /// Releases this XML node handle.
    ~XmlNode();

    /// The element tag name.  Returns an empty string for an empty node.
    std::string name() const;

    /// The text content of this element.
    ///
    /// @return The text, or `kFailedPrecondition` when the node is empty.
    Result<std::string> text() const;

    /// Reads an attribute by name.
    ///
    /// @return The attribute value, or `kFailedPrecondition` when the node is
    /// empty, or `kNotFound` when the attribute is absent.
    Result<std::string> attribute(std::string_view name) const;

    /// Finds a direct child element by tag name.
    ///
    /// @return The child node, or `kFailedPrecondition` when the node is empty,
    /// or `kNotFound` when no such child exists.
    Result<XmlNode> child(std::string_view name) const;

    /// Appends a new child element with the given tag name.
    ///
    /// @return The new child, or `kFailedPrecondition` when the node is empty.
    Result<XmlNode> append_child(std::string_view name);

    /// Sets the text content of this element.
    ///
    /// @return `kFailedPrecondition` when the node is empty.
    Status set_text(std::string_view text);

    /// Sets an attribute value.
    ///
    /// @return `kFailedPrecondition` when the node is empty.
    Status set_attribute(std::string_view name, std::string_view value);

private:
    friend class XmlDocument;

    explicit XmlNode(std::shared_ptr<detail::XmlNodeStorage> storage);

    std::shared_ptr<detail::XmlNodeStorage> storage_;
};

/// Parses XML text into an `XmlDocument`.
///
/// @return The parsed document on success, or `StatusCode::kInvalidArgument`
/// for malformed input.
NEXUS_COMMON_API Result<XmlDocument> parse_xml(std::string_view text);

} // namespace nexus::common
