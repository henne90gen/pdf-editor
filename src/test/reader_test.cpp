#include <gtest/gtest.h>

#include <pdf/document.h>
#include <pdf/page.h>

TEST(Reader, Blank) {
    auto allocatorResult = pdf::Allocator::create();
    ASSERT_FALSE(allocatorResult.has_error());
    auto result = pdf::Document::read_from_file(allocatorResult.value(), "../../../test-files/blank.pdf");
    ASSERT_FALSE(result.has_error()) << result.message();

    auto &document = result.value();
    auto objects   = document.objects();
    ASSERT_EQ(objects.size(), 8);

    auto root = document.catalog();
    ASSERT_NE(root, nullptr);

    auto pages = document.pages();
    ASSERT_EQ(pages.size(), 1);

    auto page        = pages[0];
    auto contentsOpt = page->attr_contents();
    ASSERT_TRUE(contentsOpt.has_value());
    auto contents = contentsOpt.value();
    ASSERT_TRUE(contents->is<pdf::Stream>());
    auto stream = contents->as<pdf::Stream>();
    auto str    = stream->decode(document.allocator);
    ASSERT_EQ(str.size(), 42);
}

TEST(Reader, HelloWorldGeneral) {
    auto allocatorResult = pdf::Allocator::create();
    ASSERT_FALSE(allocatorResult.has_error());
    auto result = pdf::Document::read_from_file(allocatorResult.value(), "../../../test-files/hello-world.pdf");
    ASSERT_FALSE(result.has_error()) << result.message();

    auto &document = result.value();
    auto objects   = document.objects();
    ASSERT_EQ(objects.size(), 13);

    auto root = document.catalog();
    ASSERT_NE(root, nullptr);

    auto pages = document.pages();
    ASSERT_EQ(pages.size(), 1);

    auto page        = pages[0];
    auto contentsOpt = page->attr_contents();
    ASSERT_TRUE(contentsOpt.has_value());
    auto contents = contentsOpt.value();
    ASSERT_TRUE(contents->is<pdf::Stream>());
    auto stream = contents->as<pdf::Stream>();
    auto str    = stream->decode(document.allocator);
    ASSERT_EQ(str.size(), 139);
}

TEST(Reader, HelloWorldFont) {
    auto allocatorResult = pdf::Allocator::create();
    ASSERT_FALSE(allocatorResult.has_error());
    auto result = pdf::Document::read_from_file(allocatorResult.value(), "../../../test-files/hello-world.pdf");
    ASSERT_FALSE(result.has_error()) << result.message();

    auto &document = result.value();
    auto pages     = document.pages();
    ASSERT_EQ(pages.size(), 1);
    auto page = pages[0];

    auto fontMapOpt = page->attr_resources()->fonts(page->document);
    ASSERT_TRUE(fontMapOpt.has_value());
    auto fontMap = fontMapOpt.value();
    ASSERT_NE(fontMap, nullptr);

    auto fontOpt = fontMap->get(page->document, "F1");
    ASSERT_TRUE(fontOpt.has_value());

    auto font = fontOpt.value();
    ASSERT_NE(font, nullptr);
    ASSERT_EQ(font->type(), "TrueType");
    ASSERT_TRUE(font->is_true_type());
    ASSERT_EQ(font->base_font()->value, "BAAAAA+LiberationSerif");

    auto fontDescriptor = font->font_descriptor(document);
    ASSERT_TRUE(fontDescriptor.has_value());
    ASSERT_EQ(fontDescriptor.value()->font_name()->value, "BAAAAA+LiberationSerif");

    auto flags = fontDescriptor.value()->flags();
    ASSERT_TRUE(flags->symbolic());

    auto encodingOpt = font->encoding(document);
    ASSERT_FALSE(encodingOpt.has_value());

    auto toUnicodeOpt = font->to_unicode(document);
    ASSERT_TRUE(toUnicodeOpt.has_value());
}

TEST(Reader, HelloWorldCmap) {
    auto allocatorResult = pdf::Allocator::create();
    ASSERT_FALSE(allocatorResult.has_error());
    auto result = pdf::Document::read_from_file(allocatorResult.value(), "../../../test-files/hello-world.pdf");
    ASSERT_FALSE(result.has_error()) << result.message();

    auto &document = result.value();
    auto pages     = document.pages();
    ASSERT_EQ(pages.size(), 1);
    auto page = pages[0];

    auto fontMapOpt = page->attr_resources()->fonts(page->document);
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
    auto i = new pdf::Integer(262178);
    auto f = i->as<pdf::FontFlags>();
    ASSERT_TRUE(f->serif());
    ASSERT_TRUE(f->non_symbolic());
    ASSERT_TRUE(f->force_bold());
    delete i;
}

TEST(Reader, ObjectStream) {
    auto allocatorResult = pdf::Allocator::create();
    ASSERT_FALSE(allocatorResult.has_error());
    auto result = pdf::Document::read_from_file(allocatorResult.value(), "../../../test-files/object-stream.pdf");
    ASSERT_FALSE(result.has_error()) << result.message();

    auto &document = result.value();
    auto objects   = document.objects();
    ASSERT_EQ(objects.size(), 16);

    auto root = document.catalog();
    ASSERT_NE(root, nullptr);

    auto pages = document.pages();
    ASSERT_EQ(pages.size(), 1);

    auto page        = pages[0];
    auto contentsOpt = page->attr_contents();
    ASSERT_TRUE(contentsOpt.has_value());
    auto contents = contentsOpt.value();
    ASSERT_TRUE(contents->is<pdf::Stream>());
    auto stream = contents->as<pdf::Stream>();
    auto str    = stream->decode(document.allocator);
    ASSERT_EQ(str.size(), 117);
}
