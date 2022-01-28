#include <functional>
#include <gtest/gtest.h>

#include <pdf/cmap.h>
#include <pdf/lexer.h>

TEST(CMapParser, DISABLED_HelloWorld) {
    auto textProvider = pdf::StringTextProvider(
          "/CIDInit/ProcSet findresource begin\n12 dict begin\nbegincmap\n/CIDSystemInfo<<\n/Registry "
          "(Adobe)\n/Ordering (UCS)\n/Supplement 0\n>> def\n/CMapName/Adobe-Identity-UCS def\n/CMapType 2 def\n1 "
          "begincodespacerange\n<00> <FF>\nendcodespacerange\n8 beginbfchar\n<01> <0048>\n<02> <0065>\n<03> "
          "<006C>\n<04> <006F>\n<05> <0020>\n<06> <0057>\n<07> <0072>\n<08> <0064>\nendbfchar\nendcmap\nCMapName "
          "currentdict /CMap defineresource pop\nend\nend");
    auto lexer     = pdf::TextLexer(textProvider);
    auto allocator = pdf::Allocator();
    allocator.init(1000);
    auto parser = pdf::CMapParser(lexer, allocator);
    auto result = parser.parse();
    ASSERT_TRUE(result != nullptr);
    ASSERT_EQ(result->map_char_code(1), "H");
    ASSERT_EQ(result->map_char_code(2), "e");
    ASSERT_EQ(result->map_char_code(3), "l");
    ASSERT_EQ(result->map_char_code(4), "o");
    ASSERT_EQ(result->map_char_code(5), " ");
    ASSERT_EQ(result->map_char_code(6), "W");
    ASSERT_EQ(result->map_char_code(7), "r");
    ASSERT_EQ(result->map_char_code(8), "d");
}

TEST(CMapParser, StandardExample) {
    auto textProvider = pdf::StringTextProvider(
          "/CIDInit /ProcSet findresource begin\n12 dict begin\nbegincmap\n/CIDSystemInfo\n<< /Registry "
          "(Adobe)\n/Ordering (UCS)\n/Supplement 0\n>> def\n/CMapName /Adobe−Identity−UCS def\n/CMapType 2 def\n1 "
          "begincodespacerange\n<0000> <FFFF>\nendcodespacerange\n2 beginbfrange\n<0000> <005E> <0020>\n<005F> "
          "<0061> [<00660066> <00660069> <00660066006C>]\nendbfrange\n1 beginbfchar\n<3A51> "
          "<D840DC3E>\nendbfchar\nendcmap\nCMapName currentdict /CMap defineresource pop\nend\nend");
    auto lexer     = pdf::TextLexer(textProvider);
    auto allocator = pdf::Allocator();
    allocator.init(1000);
    auto parser = pdf::CMapParser(lexer, allocator);
    auto result = parser.parse();
    ASSERT_TRUE(result != nullptr);
    //    ASSERT_EQ(result->map_char_code(95), "ff");
    //    ASSERT_EQ(result->map_char_code(96), "fi");
    //    ASSERT_EQ(result->map_char_code(97), "ffl");
}
