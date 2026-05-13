#include <nexus/common/xml.h>

#include <sstream>
#include <utility>

#include <pugixml.hpp>

namespace nexus::common {

namespace detail {

class XmlDocumentStorage {
public:
    pugi::xml_document document;
};

class XmlNodeStorage {
public:
    XmlNodeStorage(std::shared_ptr<XmlDocumentStorage> owner, pugi::xml_node node)
        : owner(std::move(owner)), node(node) {}

    std::shared_ptr<XmlDocumentStorage> owner;
    pugi::xml_node node;
};

} // namespace detail

namespace {

Status failed_precondition(std::string message) {
    return Status(StatusCode::kFailedPrecondition, std::move(message));
}

std::shared_ptr<detail::XmlDocumentStorage> make_document_storage() {
    return std::make_shared<detail::XmlDocumentStorage>();
}

std::shared_ptr<detail::XmlNodeStorage> make_node_storage(
    std::shared_ptr<detail::XmlDocumentStorage> owner,
    pugi::xml_node node) {
    return std::make_shared<detail::XmlNodeStorage>(std::move(owner), node);
}

Status invalid_node_status() {
    return failed_precondition("XML node is empty");
}

} // namespace

XmlDocument::XmlDocument() : storage_(make_document_storage()) {}

XmlDocument::XmlDocument(const XmlDocument& other) : storage_(make_document_storage()) {
    const auto text = other.dump();
    storage_->document.load_buffer(text.data(), text.size());
}

XmlDocument::XmlDocument(XmlDocument&& other) noexcept
    : storage_(std::move(other.storage_)) {
    if (!other.storage_) {
        other.storage_ = make_document_storage();
    }
}

XmlDocument& XmlDocument::operator=(const XmlDocument& other) {
    if (this != &other) {
        auto replacement = make_document_storage();
        const auto text = other.dump();
        replacement->document.load_buffer(text.data(), text.size());
        storage_ = std::move(replacement);
    }
    return *this;
}

XmlDocument& XmlDocument::operator=(XmlDocument&& other) noexcept {
    if (this != &other) {
        storage_ = std::move(other.storage_);
        if (!other.storage_) {
            other.storage_ = make_document_storage();
        }
    }
    return *this;
}

XmlDocument::~XmlDocument() = default;

XmlDocument XmlDocument::create(std::string_view root_name) {
    XmlDocument document;
    document.storage_->document.append_child(std::string(root_name).c_str());
    return document;
}

Result<XmlNode> XmlDocument::root() const {
    const auto node = storage_->document.document_element();
    if (!node) {
        return Status::not_found("XML document does not contain a root element");
    }

    return XmlNode(make_node_storage(storage_, node));
}

std::string XmlDocument::dump() const {
    std::ostringstream stream;
    storage_->document.save(stream, "", pugi::format_raw | pugi::format_no_declaration);
    return stream.str();
}

XmlDocument::XmlDocument(std::shared_ptr<detail::XmlDocumentStorage> storage)
    : storage_(std::move(storage)) {}

XmlNode::XmlNode()
    : storage_(make_node_storage(make_document_storage(), pugi::xml_node())) {}

XmlNode::XmlNode(const XmlNode& other) = default;

XmlNode::XmlNode(XmlNode&& other) noexcept
    : storage_(std::move(other.storage_)) {
    if (!other.storage_) {
        other.storage_ = make_node_storage(make_document_storage(), pugi::xml_node());
    }
}

XmlNode& XmlNode::operator=(const XmlNode& other) = default;

XmlNode& XmlNode::operator=(XmlNode&& other) noexcept {
    if (this != &other) {
        storage_ = std::move(other.storage_);
        if (!other.storage_) {
            other.storage_ = make_node_storage(make_document_storage(), pugi::xml_node());
        }
    }
    return *this;
}

XmlNode::~XmlNode() = default;

std::string XmlNode::name() const {
    if (!storage_->node) {
        return {};
    }
    return storage_->node.name();
}

Result<std::string> XmlNode::text() const {
    if (!storage_->node) {
        return invalid_node_status();
    }
    return std::string(storage_->node.child_value());
}

Result<std::string> XmlNode::attribute(std::string_view name) const {
    if (!storage_->node) {
        return invalid_node_status();
    }

    const auto attribute = storage_->node.attribute(std::string(name).c_str());
    if (!attribute) {
        return Status::not_found("XML node does not contain attribute: " + std::string(name));
    }

    return std::string(attribute.value());
}

Result<XmlNode> XmlNode::child(std::string_view name) const {
    if (!storage_->node) {
        return invalid_node_status();
    }

    const auto child = storage_->node.child(std::string(name).c_str());
    if (!child) {
        return Status::not_found("XML node does not contain child: " + std::string(name));
    }

    return XmlNode(make_node_storage(storage_->owner, child));
}

Result<XmlNode> XmlNode::append_child(std::string_view name) {
    if (!storage_->node) {
        return invalid_node_status();
    }

    const auto child = storage_->node.append_child(std::string(name).c_str());
    if (!child) {
        return Status::internal("Failed to append XML child: " + std::string(name));
    }

    return XmlNode(make_node_storage(storage_->owner, child));
}

Status XmlNode::set_text(std::string_view text) {
    if (!storage_->node) {
        return invalid_node_status();
    }

    storage_->node.text().set(std::string(text).c_str());
    return Status::ok_status();
}

Status XmlNode::set_attribute(std::string_view name, std::string_view value) {
    if (!storage_->node) {
        return invalid_node_status();
    }

    auto attribute = storage_->node.attribute(std::string(name).c_str());
    if (!attribute) {
        attribute = storage_->node.append_attribute(std::string(name).c_str());
    }

    if (!attribute) {
        return Status::internal("Failed to set XML attribute: " + std::string(name));
    }

    attribute.set_value(std::string(value).c_str());
    return Status::ok_status();
}

XmlNode::XmlNode(std::shared_ptr<detail::XmlNodeStorage> storage)
    : storage_(std::move(storage)) {}

Result<XmlDocument> parse_xml(std::string_view text) {
    auto storage = make_document_storage();
    const auto result = storage->document.load_buffer(text.data(), text.size());
    if (!result) {
        return Status::invalid_argument(result.description());
    }

    return XmlDocument(std::move(storage));
}

} // namespace nexus::common
