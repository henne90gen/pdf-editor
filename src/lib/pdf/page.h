#pragma once

#include <utility>

#include "pdf/document.h"
#include "pdf/objects.h"
#include "pdf/operator_parser.h"
#include "pdf/operator_traverser.h"
#include "pdf/util/debug.h"

namespace pdf {

struct OperatorTraverser;

struct PageTreeNode : public Dictionary {
    Name *type() { return must_find<Name>("Type"); }
    bool is_page() { return type()->value == "Page"; }
    PageTreeNode *parent(Document &document);
    Array *kids() { return must_find<Array>("Kids"); }
    Integer *count() { return must_find<Integer>("Count"); }

    template <typename T>
    std::optional<T *> attribute(Document &document, const std::string &attributeName, bool inheritable) {
        auto itr = values.find(attributeName);
        if (itr != values.end()) {
            return document.get<T>(itr->second);
        }

        if (!inheritable) {
            return {};
        }

        auto p = parent(document);
        if (p == nullptr) {
            return {};
        }

        const std::optional<T *> &attrib = p->attribute<T>(document, attributeName, inheritable);
        if (!attrib.has_value()) {
            return {};
        }

        return attrib.value()->template as<T>();
    }
};

struct XObjectMap : public Dictionary {};

struct Resources : public Dictionary {
    std::optional<XObjectMap *> x_objects(Document &document) {
        return document.get<XObjectMap>(find<Object>("XObject"));
    }
    std::optional<FontMap *> fonts(Document &document) { return document.get<FontMap>(find<Object>("Font")); }
};

struct ContentStream : public Stream {
    void for_each_operator(Allocator &allocator, const std::function<ForEachResult(Operator *)> &func);
};

struct Page {
    Document &document;
    PageTreeNode *node;
    OperatorTraverser traverser = OperatorTraverser(*this);

    explicit Page(Document &_document, PageTreeNode *_node);

    Resources *attr_resources() { return node->attribute<Resources>(document, "Resources", true).value(); }
    Rectangle *attr_media_box() { return node->attribute<Rectangle>(document, "MediaBox", true).value(); }

    // TODO make value_or more efficient (currently mediaBox is fetched even if it is not being used)
    Rectangle *attr_crop_box() {
        return node->attribute<Rectangle>(document, "CropBox", true).value_or(attr_media_box());
    }
    Rectangle *attr_bleed_box() {
        return node->attribute<Rectangle>(document, "BleedBox", true).value_or(attr_media_box());
    }
    Rectangle *attr_trim_box() {
        return node->attribute<Rectangle>(document, "TrimBox", true).value_or(attr_media_box());
    }
    Rectangle *attr_art_box() {
        return node->attribute<Rectangle>(document, "ArtBox", true).value_or(attr_media_box());
    }
    std::optional<Dictionary *> attr_box_color_info() {
        return node->attribute<Dictionary>(document, "BoxColorInfo", false);
    }
    std::optional<Object *> attr_contents() { return node->attribute<Object>(document, "Contents", false); }
    std::vector<ContentStream *> content_streams();
    std::vector<TextBlock> text_blocks();
    std::vector<PageImage> images();
    void for_each_image(const std::function<ForEachResult(PageImage &)> &func);

    int64_t rotate();
    double attr_width();
    double attr_height();
    size_t character_count();
    std::optional<Font *> get_font(const Tf_SetTextFontSize &data);

    void render(const Cairo::RefPtr<Cairo::Context> &cr);
};

} // namespace pdf
