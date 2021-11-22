#include <gtest/gtest.h>

#include <pdf/document.h>
#include <pdf/page.h>

TEST(Reader, Blank) {
    pdf::Document document;
    pdf::Document::read_from_file("../../../test-files/blank.pdf", document);
    std::vector<pdf::IndirectObject *> objects = document.objects();
    ASSERT_EQ(objects.size(), 8);

    auto root = document.catalog();
    ASSERT_NE(root, nullptr);

    auto pages = document.pages();
    ASSERT_EQ(pages.size(), 1);

    auto page        = pages[0];
    auto contentsOpt = page->contents();
    ASSERT_TRUE(contentsOpt.has_value());
    auto contents = contentsOpt.value();
    ASSERT_TRUE(contents->is<pdf::Stream>());
    auto stream = contents->as<pdf::Stream>();
    auto str    = stream->decode(document.allocator);
    ASSERT_EQ(str.size(), 42);
}

TEST(Reader, HelloWorldGeneral) {
    pdf::Document document;
    pdf::Document::read_from_file("../../../test-files/hello-world.pdf", document);
    std::vector<pdf::IndirectObject *> objects = document.objects();
    ASSERT_EQ(objects.size(), 13);

    auto root = document.catalog();
    ASSERT_NE(root, nullptr);

    auto pages = document.pages();
    ASSERT_EQ(pages.size(), 1);

    auto page        = pages[0];
    auto contentsOpt = page->contents();
    ASSERT_TRUE(contentsOpt.has_value());
    auto contents = contentsOpt.value();
    ASSERT_TRUE(contents->is<pdf::Stream>());
    auto stream = contents->as<pdf::Stream>();
    auto str    = stream->decode(document.allocator);
    ASSERT_EQ(str.size(), 139);
}

TEST(Reader, HelloWorldFont) {
    pdf::Document document;
    pdf::Document::read_from_file("../../../test-files/hello-world.pdf", document);

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
    ASSERT_EQ(font->type(), "TrueType");
    ASSERT_TRUE(font->is_true_type());
    ASSERT_EQ(font->base_font()->value(), "BAAAAA+LiberationSerif");

    auto fontDescriptor = font->font_descriptor(document);
    ASSERT_NE(fontDescriptor, nullptr);
    ASSERT_EQ(fontDescriptor->font_name()->value(), "BAAAAA+LiberationSerif");

    auto flags = fontDescriptor->flags();
    ASSERT_TRUE(flags->symbolic());

    auto encodingOpt = font->encoding(document);
    ASSERT_FALSE(encodingOpt.has_value());

    auto toUnicodeOpt = font->to_unicode(document);
    ASSERT_TRUE(toUnicodeOpt.has_value());
}

TEST(Reader, HelloWorldCmap) {
    pdf::Document document;
    pdf::Document::read_from_file("../../../test-files/hello-world.pdf", document);

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
    auto toUnicodeOpt = font->to_unicode(document);
    ASSERT_TRUE(toUnicodeOpt.has_value());
    //    auto toUnicode = toUnicodeOpt.value();
    //    auto cmap      = toUnicode->read_cmap();
}

TEST(Reader, FontFlags) {
    auto i = new pdf::Integer("262178", 262178);
    auto f = i->as<pdf::FontFlags>();
    ASSERT_TRUE(f->serif());
    ASSERT_TRUE(f->non_symbolic());
    ASSERT_TRUE(f->force_bold());
    delete i;
}

TEST(Reader, ObjectStream) {
    pdf::Document document;
    pdf::Document::read_from_file("../../../test-files/object-stream.pdf", document);
    std::vector<pdf::IndirectObject *> objects = document.objects();
    ASSERT_EQ(objects.size(), 16);

    auto root = document.catalog();
    ASSERT_NE(root, nullptr);

    auto pages = document.pages();
    ASSERT_EQ(pages.size(), 1);

    auto page        = pages[0];
    auto contentsOpt = page->contents();
    ASSERT_TRUE(contentsOpt.has_value());
    auto contents = contentsOpt.value();
    ASSERT_TRUE(contents->is<pdf::Stream>());
    auto stream = contents->as<pdf::Stream>();
    auto str    = stream->decode(document.allocator);
    ASSERT_EQ(str.size(), 117);
}

TEST(Reader, HelloWorldTextBlocks) {
    pdf::Document document;
    pdf::Document::read_from_file("../../../test-files/hello-world.pdf", document);

    auto pages = document.pages();
    ASSERT_EQ(pages.size(), 1);

    auto page      = pages[0];
    auto textBlock = page->text_blocks();
    ASSERT_EQ(textBlock.size(), 1);

    // TODO   ASSERT_EQ(textBlock[0].x, 100);
}
