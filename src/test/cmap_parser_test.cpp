#include <functional>
#include <gtest/gtest.h>

#include <pdf/cmap.h>
#include <pdf/lexer.h>

TEST(CMapParser, HelloWorld) {
    auto textProvider = pdf::StringTextProvider(
          "/CIDInit/ProcSet findresource begin\n12 dict begin\nbegincmap\n/CIDSystemInfo<<\n/Registry "
          "(Adobe)\n/Ordering (UCS)\n/Supplement 0\n>> def\n/CMapName/Adobe-Identity-UCS def\n/CMapType 2 def\n1 "
          "begincodespacerange\n<00> <FF>\nendcodespacerange\n8 beginbfchar\n<01> <0048>\n<02> <0065>\n<03> "
          "<006C>\n<04> <006F>\n<05> <0020>\n<06> <0057>\n<07> <0072>\n<08> <0064>\nendbfchar\nendcmap\nCMapName "
          "currentdict /CMap defineresource pop\nend\nend");
    auto lexer  = pdf::TextLexer(textProvider);
    auto parser = pdf::CMapParser(lexer);
    auto result = parser.parse();
//    ASSERT_TRUE(result != nullptr);
}
