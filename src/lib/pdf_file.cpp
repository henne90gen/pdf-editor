#include "pdf_file.h"

namespace pdf {

IndirectObject *File::loadObject(int64_t objectNumber) const {
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
    auto lexer  = TextLexer(text);
    auto parser = Parser(lexer, (ReferenceResolver *)this);
    auto result = parser.parse();
    return result->as<IndirectObject>();
}

IndirectObject *File::getObject(int64_t objectNumber) {
    if (objects[objectNumber] != nullptr) {
        return objects[objectNumber];
    }

    auto object           = loadObject(objectNumber);
    objects[objectNumber] = object;
    return object;
}

Dictionary *File::getRoot() {
    auto rootRef        = trailer.dict->values["Root"]->as<IndirectReference>();
    auto resolvedObj    = resolve(rootRef);
    auto indirectObject = resolvedObj->as<IndirectObject>();
    return indirectObject->object->as<Dictionary>();
}

PageTreeNode *File::getPageTree() {
    auto root           = getRoot();
    auto pagesRef       = root->values["Pages"]->as<IndirectReference>();
    auto resolvedObj    = resolve(pagesRef);
    auto indirectObject = resolvedObj->as<IndirectObject>();
    return indirectObject->object->as<PageTreeNode>();
}

IndirectObject *File::resolve(const IndirectReference *ref) { return getObject(ref->objectNumber); }

std::vector<IndirectObject *> File::getAllObjects() {
    std::vector<IndirectObject *> result = {};
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

std::vector<PageNode *> PageTreeNode::pages(File &file) {
    // TODO this is super inefficient, but works for now

    if (isPage()) {
        return {this->as<PageNode>()};
    }

    auto _this  = this->as<IntermediateNode>();
    auto result = std::vector<PageNode *>();

    auto count = _this->count()->value;
    result.reserve(count);

    for (auto kid : _this->kids()->values) {
        auto resolvedKid = file.resolve(kid->as<IndirectReference>());
        auto subPages    = resolvedKid->object->as<PageTreeNode>()->pages(file);
        for (auto page : subPages) {
            result.push_back(page);
        }
    }

    return result;
}

PageTreeNode *PageTreeNode::parent(File &file) {
    auto itr = values.find("Parent");
    if (itr == values.end()) {
        return nullptr;
    }
    auto reference         = itr->second->as<IndirectReference>();
    auto resolvedReference = file.resolve(reference);
    return resolvedReference->as<PageTreeNode>();
}

int64_t PageNode::rotate(File &file) {
    const std::optional<Integer *> &rot = attribute<Integer>(file, "Rotate", true);
    if (!rot.has_value()) {
        return 0;
    }
    return rot.value()->value;
}

} // namespace pdf
