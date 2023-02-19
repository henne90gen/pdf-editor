#pragma once

#include <utility>

#include "pdf/document.h"
#include "pdf/objects.h"
#include "pdf/operator_parser.h"
#include "pdf/util/debug.h"

namespace pdf {

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
    void for_each_operator(Arena &arena, const std::function<ForEachResult(Operator *)> &func);
};

struct TextBlock {
    std::string text;
    double x      = 0.0;
    double y      = 0.0;
    double width  = 0.0;
    double height = 0.0;

    Operator *op      = nullptr;
    ContentStream *cs = nullptr;

    /// moves the text block on the page by the specified offset
    void move(Document &document, double xOffset, double yOffset) const;
};

struct XObjectImage : public Stream {
    int64_t width() { return dictionary->must_find<Integer>("Width")->value; }
    int64_t height() { return dictionary->must_find<Integer>("Height")->value; }
    std::optional<Object *> color_space() { return dictionary->find<Object>("ColorSpace"); }
    std::optional<Integer *> bits_per_component() { return dictionary->find<Integer>("BitsPerComponent"); }
    bool image_mask() {
        const auto opt = dictionary->find<Boolean>("ImageMask");
        if (!opt.has_value()) {
            return false;
        }
        return opt.value()->value;
    }
};

struct PageImage {
    std::string name;
    double xOffset      = 0.0;
    double yOffset      = 0.0;
    XObjectImage *image = nullptr;

    Operator *op      = nullptr;
    ContentStream *cs = nullptr;

    PageImage(std::string _name, double _xOffset, double _yOffset, XObjectImage *_image, Operator *_op,
              ContentStream *_cs)
        : name(std::move(_name)), xOffset(_xOffset), yOffset(_yOffset), image(_image), op(_op), cs(_cs) {}

    /// Moves the image on the page by the specified offset
    void move(Document &document, double xOffset, double yOffset) const;
};

struct Page {
    Document &document;
    PageTreeNode *node;

    explicit Page(Document &_document, PageTreeNode *_node) : document(_document), node(_node) {}

    Resources *resources() { return node->attribute<Resources>(document, "Resources", true).value(); }
    Rectangle *media_box() { return node->attribute<Rectangle>(document, "MediaBox", true).value(); }

    // TODO make value_or more efficient (currently mediaBox is fetched even if it is not being used)
    Rectangle *crop_box() { return node->attribute<Rectangle>(document, "CropBox", true).value_or(media_box()); }
    Rectangle *bleed_box() { return node->attribute<Rectangle>(document, "BleedBox", true).value_or(media_box()); }
    Rectangle *trim_box() { return node->attribute<Rectangle>(document, "TrimBox", true).value_or(media_box()); }
    Rectangle *art_box() { return node->attribute<Rectangle>(document, "ArtBox", true).value_or(media_box()); }
    std::optional<Dictionary *> box_color_info() {
        return node->attribute<Dictionary>(document, "BoxColorInfo", false);
    }
    std::optional<Object *> contents() { return node->attribute<Object>(document, "Contents", false); }
    std::vector<ContentStream *> content_streams();
    std::vector<TextBlock> text_blocks();
    std::vector<PageImage> images();
    void for_each_image(const std::function<ForEachResult(PageImage)> &func);

    int64_t rotate();
    double width();
    double height();
    size_t character_count();
    std::optional<Font *> get_font(const Tf_SetTextFontSize &data);
};

} // namespace pdf
