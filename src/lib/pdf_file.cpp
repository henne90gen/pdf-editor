#include "pdf_file.h"

namespace pdf {

Object *pdf::File::loadObject(int64_t objectNumber) const {
    auto &entry = crossReferenceTable.entries[objectNumber];
    if (entry.isFree) {
        return nullptr;
    }

    auto start = data + entry.byteOffset;

    // TODO this is dangerous (it might read past the end of the stream)
    size_t length = 0;
    while (std::string(start + length, 6) != "endobj") {
        length++;
    }

    auto input  = std::string(start, length + 6);
    auto text   = StringTextProvider(input);
    auto lexer  = Lexer(text);
    auto parser = Parser(lexer, (ReferenceResolver *)this);
    auto result = parser.parse();
    return result;
}

Object *File::getObject(int64_t objectNumber) {
    if (objects[objectNumber] != nullptr) {
        return objects[objectNumber];
    }

    auto object           = loadObject(objectNumber);
    objects[objectNumber] = object;
    return object;
}

Dictionary *File::getRoot() {
    auto rootRef        = trailer.dict->values["Root"];
    auto resolvedObj    = resolve(rootRef->as<IndirectReference>());
    auto indirectObject = resolvedObj->as<IndirectObject>();
    return indirectObject->object->as<Dictionary>();
}

Dictionary *File::getPages() {
    auto root = getRoot();
    return root->values["Pages"]->as<Dictionary>();
}

Object *File::resolve(IndirectReference *ref) { return getObject(ref->objectNumber); }

std::vector<Object *> File::getAllObjects() {
    std::vector<Object *> result = {};
    for (int i = 0; i < crossReferenceTable.entries.size(); i++) {
        auto &entry = crossReferenceTable.entries[i];
        if (entry.isFree) {
            continue;
        }
        auto object = getObject(i);
        result.push_back(object);
    }
    return result;
}

} // namespace pdf
