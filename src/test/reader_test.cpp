#include <gtest/gtest.h>

#include <pdf/document.h>
#include <pdf/page.h>

TEST(Reader, Blank) {
    pdf::Document document;
    pdf::Document::loadFromFile("../../../test-files/blank.pdf", document);
    std::vector<pdf::IndirectObject *> objects = document.getAllObjects();
    ASSERT_EQ(objects.size(), 8);

    auto root = document.root();
    ASSERT_NE(root, nullptr);

    auto pages = document.pages();
    ASSERT_EQ(pages.size(), 1);

    auto page        = pages[0];
    auto contentsOpt = page->contents();
    ASSERT_TRUE(contentsOpt.has_value());
    auto contents = contentsOpt.value();
    ASSERT_TRUE(contents->is<pdf::Stream>());
    auto stream  = contents->as<pdf::Stream>();
    auto str     = stream->to_string();
    auto filters = stream->filters();
}

TEST(Reader, HelloWorldGeneral) {
    pdf::Document document;
    pdf::Document::loadFromFile("../../../test-files/hello-world.pdf", document);
    std::vector<pdf::IndirectObject *> objects = document.getAllObjects();
    ASSERT_EQ(objects.size(), 13);

    auto root = document.root();
    ASSERT_NE(root, nullptr);

    auto pages = document.pages();
    ASSERT_EQ(pages.size(), 1);

    auto page        = pages[0];
    auto contentsOpt = page->contents();
    ASSERT_TRUE(contentsOpt.has_value());
    auto contents = contentsOpt.value();
    ASSERT_TRUE(contents->is<pdf::Stream>());
    auto stream  = contents->as<pdf::Stream>();
    auto str     = stream->to_string();
    auto filters = stream->filters();
}

TEST(Reader, HelloWorldFont) {
    pdf::Document document;
    pdf::Document::loadFromFile("../../../test-files/hello-world.pdf", document);

    auto pages = document.pages();
    ASSERT_EQ(pages.size(), 1);
    auto page = pages[0];

    auto fontMapOpt = page->resources()->fonts(page->document);
    ASSERT_TRUE(fontMapOpt.has_value());
    auto fontMap = fontMapOpt.value();
    ASSERT_NE(fontMap, nullptr);

    auto fontOpt = fontMap->get(page->document, "F1");
    ASSERT_TRUE(fontOpt.has_value());

    auto font = fontOpt.value();
    ASSERT_NE(font, nullptr);
    ASSERT_EQ(font->type()->value, "TrueType");
    ASSERT_TRUE(font->isTrueType());
    auto ttFont = font->as<pdf::TrueTypeFont>();
    ASSERT_EQ(ttFont->baseFont()->value, "BAAAAA+LiberationSerif");

    auto fontDescriptor = ttFont->fontDescriptor(document);
    ASSERT_NE(fontDescriptor, nullptr);
    ASSERT_EQ(fontDescriptor->fontName()->value, "BAAAAA+LiberationSerif");

    auto flags = fontDescriptor->flags();
    ASSERT_TRUE(flags->symbolic());

    auto encodingOpt = ttFont->encoding(document);
    ASSERT_FALSE(encodingOpt.has_value());

    auto toUnicodeOpt = ttFont->toUnicode(document);
    ASSERT_TRUE(toUnicodeOpt.has_value());
}

TEST(Reader, HelloWorldCmap) {
    pdf::Document document;
    pdf::Document::loadFromFile("../../../test-files/hello-world.pdf", document);

    auto pages = document.pages();
    ASSERT_EQ(pages.size(), 1);
    auto page = pages[0];

    auto fontMapOpt = page->resources()->fonts(page->document);
    ASSERT_TRUE(fontMapOpt.has_value());
    auto fontMap = fontMapOpt.value();
    ASSERT_NE(fontMap, nullptr);

    auto fontOpt = fontMap->get(page->document, "F1");
    ASSERT_TRUE(fontOpt.has_value());

    auto font = fontOpt.value();
    ASSERT_NE(font, nullptr);
    auto trueTypeFont = font->as<pdf::TrueTypeFont>();
    auto toUnicodeOpt = trueTypeFont->toUnicode(document);
    ASSERT_TRUE(toUnicodeOpt.has_value());
    auto toUnicode = toUnicodeOpt.value();
    auto cmap      = toUnicode->read_cmap();
}

TEST(Reader, FontFlags) {
    auto i = new pdf::Integer(262178);
    auto f = i->as<pdf::FontFlags>();
    ASSERT_TRUE(f->serif());
    ASSERT_TRUE(f->nonsymbolic());
    ASSERT_TRUE(f->forceBold());
}
