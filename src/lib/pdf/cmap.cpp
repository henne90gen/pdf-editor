#include "cmap.h"

namespace pdf {

#pragma pack(push, 1)
struct CMap_Index {
    uint16_t version;
    uint16_t numberSubtables;
};

struct CMap_Subtable {
    uint16_t platformID;
    uint16_t platformSpecificID;
    uint32_t offset;
};
#pragma pack(pop)

/*
 "/CIDInit/ProcSet findresource begin
 12 dict begin
begincmap
/CIDSystemInfo<<
/Registry (Adobe)
/Ordering (UCS)
/Supplement 0
>> def
/CMapName/Adobe-Identity-UCS def
/CMapType 2 def
1 begincodespacerange
<00> <FF>
endcodespacerange
8 beginbfchar
<01> <0048>
<02> <0065>
<03> <006C>
<04> <006F>
<05> <0020>
<06> <0057>
<07> <0072>
<08> <0064>
endbfchar
endcmap
CMapName currentdict /CMap defineresource pop
end
end
"
 */

CMap* CMapStream::read_cmap() {
    auto data  = to_string();

    return new CMap(data);
}

} // namespace pdf
