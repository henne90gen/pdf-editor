#include "document.h"

#include <fstream>
#include <spdlog/spdlog.h>
#include <sstream>

namespace pdf {

void ignoreNewLines(uint8_t *&ptr) {
    if (*ptr == '\r') {
        ptr++;
    }
    if (*ptr == '\n') {
        ptr++;
    }
}

// TODO replace "start + length" with a string_view
ValueResult<Dictionary *> parse_dict(Arena &arena, uint8_t *start, size_t length) {
    ASSERT(start != nullptr);
    ASSERT(length > 0);
    auto input  = std::string_view((char *)start, length);
    auto text   = StringTextProvider(input);
    auto lexer  = TextLexer(text);
    auto parser = Parser(lexer, arena);
    auto result = parser.parse();

    // TODO make error messages more descriptive
    if (result == nullptr) {
        return ValueResult<Dictionary *>::error("Failed to parse dictionary");
    }
    if (!result->is<Dictionary>()) {
        return ValueResult<Dictionary *>::error("Failed to parse dictionary");
    }

    return ValueResult<Dictionary *>::ok(result->as<Dictionary>());
}

// TODO replace "start + length" with a string_view
ValueResult<IndirectObject *> parse_stream(Allocator &allocator, uint8_t *start, size_t length) {
    ASSERT(start != nullptr);
    ASSERT(length > 0);
    auto input  = std::string_view((char *)start, length);
    auto text   = StringTextProvider(input);
    auto lexer  = TextLexer(text);
    auto parser = Parser(lexer, allocator.arena());
    auto result = parser.parse();

    // TODO make error messages more descriptive
    if (result == nullptr) {
        return ValueResult<IndirectObject *>::error("Failed to parse stream");
    }
    if (!result->is<IndirectObject>()) {
        return ValueResult<IndirectObject *>::error("Failed to parse stream");
    }

    return ValueResult<IndirectObject *>::ok(result->as<IndirectObject>());
}

Result read_trailers(Document &document, uint8_t *crossRefStartPtr, Trailer *currentTrailer);
Result read_cross_reference_stream(Document &document, IndirectObject *streamObject, Trailer *currentTrailer) {
    if (!streamObject->object->is<Stream>()) {
        return Result::error("Expected STREAM object but got {}", streamObject->object->type_string());
    }

    auto stream            = streamObject->object->as<Stream>();
    std::string &typeValue = stream->dictionary->must_find<Name>("Type")->value;
    if (typeValue != "XRef") {
        return Result::error("Expected stream of type XRef but got {}", typeValue);
    }

    auto W = stream->dictionary->must_find<Array>("W");
    if (W->values.size() != 3) {
        spdlog::warn("Cross reference stream should have W with 3 entries, not {}", W->values.size());
    }
    auto sizeField0 = W->values[0]->as<Integer>()->value;
    auto sizeField1 = W->values[1]->as<Integer>()->value;
    auto sizeField2 = W->values[2]->as<Integer>()->value;
    auto content    = stream->decode(document.allocator);

    // verify that the content of the stream matches the size in the dictionary
    size_t countInDict        = stream->dictionary->must_find<Integer>("Size")->value - 1;
    size_t crossRefEntryCount = content.size() / (sizeField0 + sizeField1 + sizeField2);
    if (countInDict != crossRefEntryCount) {
        spdlog::warn(
              "Cross reference stream has mismatched entry counts: {} (count in dictionary) vs {} (actual count)",
              countInDict, crossRefEntryCount);
    }

    auto indexOpt = stream->dictionary->find<Array>("Index");
    if (indexOpt.has_value()) {
        auto index = indexOpt.value();
        if (index->values.size() == 2) {
            currentTrailer->crossReferenceTable.firstObjectNumber = index->values[0]->as<Integer>()->value;
            // TODO the second value is the number of entries and not necessarily the number of objects
            currentTrailer->crossReferenceTable.objectCount = index->values[1]->as<Integer>()->value;
        } else {
            // TODO streams can define subsections of entries
            spdlog::warn("Stream subsections are not implemented yet");
        }
    } else {
        currentTrailer->crossReferenceTable.firstObjectNumber = 0;
        currentTrailer->crossReferenceTable.objectCount       = static_cast<int64_t>(countInDict);
    }

    if (currentTrailer->crossReferenceTable.objectCount < 0) {
        return Result::error("Object count in cross reference table cannot be negative");
    }
    if (currentTrailer->crossReferenceTable.objectCount > 1000000) {
        return Result::error("Too many objects in cross reference table: {}",
                             currentTrailer->crossReferenceTable.objectCount);
    }

    auto unknownEntryCount = 0;
    for (auto contentPtr = content.data(); contentPtr < content.data() + content.size();
         contentPtr += sizeField0 + sizeField1 + sizeField2) {
        uint64_t type = 0;
        if (sizeField0 == 0) {
            type = 1; // default value for type
        } else {
            for (int j = 0; j < sizeField0; j++) {
                uint8_t c = *(contentPtr + j);
                type      = (type << 8) + c;
            }
        }

        uint64_t field1 = 0;
        for (int j = 0; j < sizeField1; j++) {
            uint8_t c = *(contentPtr + j + sizeField0);
            field1    = (field1 << 8) + c;
        }

        uint64_t field2 = 0;
        for (int j = 0; j < sizeField2; j++) {
            uint8_t c = *(contentPtr + j + sizeField0 + sizeField1);
            field2    = (field2 << 8) + c;
        }

        switch (type) {
        case 0: {
            CrossReferenceEntry entry                 = {};
            entry.type                                = CrossReferenceEntryType::FREE;
            entry.free.nextFreeObjectNumber           = field1;
            entry.free.nextFreeObjectGenerationNumber = field2;
            currentTrailer->crossReferenceTable.entries.push_back(entry);
        } break;
        case 1: {
            CrossReferenceEntry entry     = {};
            entry.type                    = CrossReferenceEntryType::NORMAL;
            entry.normal.byteOffset       = field1;
            entry.normal.generationNumber = field2;
            currentTrailer->crossReferenceTable.entries.push_back(entry);
        } break;
        case 2: {
            CrossReferenceEntry entry             = {};
            entry.type                            = CrossReferenceEntryType::COMPRESSED;
            entry.compressed.objectNumberOfStream = field1;
            entry.compressed.indexInStream        = field2;
            currentTrailer->crossReferenceTable.entries.push_back(entry);
        } break;
        default:
            spdlog::trace("Encountered unknown cross reference stream entry field type: {}", type);
            unknownEntryCount++;
            break;
        }
    }
    spdlog::warn("Encountered {} unknown cross reference stream entries", unknownEntryCount);

    auto opt = stream->dictionary->find<Integer>("Prev");
    if (!opt.has_value()) {
        return Result::ok();
    }

    currentTrailer->prev = document.allocator.arena().push<Trailer>(document.allocator);
    return read_trailers(document, document.file.data + opt.value()->as<Integer>()->value, currentTrailer->prev);
}

Result read_trailers(Document &document, uint8_t *crossRefStartPtr, Trailer *currentTrailer) {
    // decide whether xref stream or table
    const auto xrefKeyword = std::string_view((char *)crossRefStartPtr, 4);
    if (document.file.is_out_of_range(xrefKeyword)) {
        return Result::error("Unexpectedly reached end of file");
    }

    if (xrefKeyword != "xref") {
        //  stream -> parse stream
        // TODO how long is the stream? (just using the end of the file for parsing purposes)
        auto startxrefPtr  = document.file.data + document.file.sizeInBytes;
        auto startOfStream = crossRefStartPtr;
        if (startxrefPtr <= startOfStream) {
            return Result::error("Failed to parse cross reference stream");
        }

        size_t lengthOfStream = startxrefPtr - startOfStream;
        auto result           = parse_stream(document.allocator, startOfStream, lengthOfStream);
        if (result.has_error()) {
            return result.drop_value();
        }

        currentTrailer->streamObject                    = result.value();
        document.file.metadata.trailers[currentTrailer] = std::string_view((char *)startOfStream, lengthOfStream);
        return read_cross_reference_stream(document, currentTrailer->streamObject, currentTrailer);
    }

    //  table -> parse table and parse trailer dict
    auto crossRefPtr = crossRefStartPtr + 4;
    ignoreNewLines(crossRefPtr);

    int64_t spaceLocation   = -1;
    uint8_t *currentReadPtr = crossRefPtr;
    while (*currentReadPtr != '\n' && *currentReadPtr != '\r') {
        if (*currentReadPtr == ' ') {
            spaceLocation = currentReadPtr - crossRefPtr;
        }
        currentReadPtr++;
    }

    auto metaData = std::string((char *)crossRefPtr, currentReadPtr - crossRefPtr);
    // TODO parse other cross-reference sections

    try {
        currentTrailer->crossReferenceTable.firstObjectNumber = std::stoll(metaData.substr(0, spaceLocation));
    } catch (std::invalid_argument &err) {
        return Result::error("Failed to parse first object number of cross reference table (std::invalid_argument): {}",
                             err.what());
    } catch (std::out_of_range &err) {
        return Result::error("Failed to parse first object number of cross reference table (std::out_of_range): {}",
                             err.what());
    }

    try {
        currentTrailer->crossReferenceTable.objectCount = std::stoll(metaData.substr(spaceLocation));
    } catch (std::invalid_argument &err) {
        return Result::error("Failed to parse object count of cross reference table (std::invalid_argument): {}",
                             err.what());
    } catch (std::out_of_range &err) {
        return Result::error("Failed to parse object count of cross reference table (std::out_of_range): {}",
                             err.what());
    }

    if (currentTrailer->crossReferenceTable.objectCount < 0) {
        return Result::error("Object count in cross reference table cannot be negative");
    }
    if (currentTrailer->crossReferenceTable.objectCount > 1000000) {
        return Result::error("Too many objects in cross reference table: {}",
                             currentTrailer->crossReferenceTable.objectCount);
    }

    for (int64_t objectNumber = currentTrailer->crossReferenceTable.firstObjectNumber;
         objectNumber <
         currentTrailer->crossReferenceTable.firstObjectNumber + currentTrailer->crossReferenceTable.objectCount;
         objectNumber++) {
        document.objectList[objectNumber] = nullptr;
    }

    ignoreNewLines(currentReadPtr);

    for (int i = 0; i < currentTrailer->crossReferenceTable.objectCount; i++) {
        if (document.file.is_out_of_range(currentReadPtr, 20)) {
            return Result::error("Invalid cross reference table");
        }

        // nnnnnnnnnn ggggg f__
        auto s = std::string((char *)currentReadPtr, 20);

        // TODO improve error messages (switch on entry type and add separate try/catch for each num)
        uint64_t num0, num1;
        try {
            num0 = std::stoll(s.substr(0, 10));
            num1 = std::stoll(s.substr(11, 16));
        } catch (std::invalid_argument &err) {
            return Result::error("Failed to parse number in cross reference table (std::invalid_argument): {}",
                                 err.what());
        } catch (std::out_of_range &err) {
            return Result::error("Failed to parse number in cross reference table (std::out_of_range): {}", err.what());
        }

        CrossReferenceEntry entry = {};
        if (s[17] == 'f') {
            entry.type                                = CrossReferenceEntryType ::FREE;
            entry.free.nextFreeObjectNumber           = num0;
            entry.free.nextFreeObjectGenerationNumber = num1;
        } else {
            entry.type                    = CrossReferenceEntryType::NORMAL;
            entry.normal.byteOffset       = num0;
            entry.normal.generationNumber = num1;
        }

        currentTrailer->crossReferenceTable.entries.push_back(entry);
        currentReadPtr += 20;
    }

    ignoreNewLines(currentReadPtr);

    // parse trailer dict
    auto view = std::string_view((char *)currentReadPtr, 7);
    while (view != "trailer") {
        currentReadPtr++;
        view = std::string_view((char *)currentReadPtr, 7);
        if (document.file.is_out_of_range(view)) {
            return Result::error("Unexpectedly reached end of file");
        }
    }

    currentReadPtr += 7;
    ignoreNewLines(currentReadPtr);

    size_t lengthOfTrailerDict = 1;
    view                       = std::string_view((char *)currentReadPtr + lengthOfTrailerDict, 9);
    while (view != "startxref") {
        lengthOfTrailerDict++;
        view = std::string_view((char *)currentReadPtr + lengthOfTrailerDict, 9);
        if (document.file.is_out_of_range(view)) {
            return Result::error("Unexpectedly reached end of file");
        }
    }

    document.file.metadata.trailers[currentTrailer] =
          std::string_view((char *)crossRefStartPtr, (currentReadPtr - crossRefStartPtr) + lengthOfTrailerDict);
    auto result = parse_dict(document.allocator.arena(), currentReadPtr, lengthOfTrailerDict);
    if (result.has_error()) {
        return result.drop_value();
    }

    currentTrailer->dict = result.value();
    auto opt             = currentTrailer->dict->find<Integer>("Prev");
    if (!opt.has_value()) {
        return Result::ok();
    }

    currentTrailer->prev = document.allocator.arena().push<Trailer>(document.allocator);
    return read_trailers(document, document.file.data + opt.value()->value, currentTrailer->prev);
}

using LoadObjectResult = ValueResult<std::pair<IndirectObject *, std::string_view>>;

LoadObjectResult load_object(Document &document, CrossReferenceEntry &entry) {
    if (entry.type == CrossReferenceEntryType::FREE) {
        return LoadObjectResult::ok({nullptr, ""});
    }

    if (entry.type == CrossReferenceEntryType::NORMAL) {
        if (entry.normal.byteOffset > document.file.sizeInBytes) {
            return LoadObjectResult ::error("Malformed cross reference table entry");
        }

        auto *start   = document.file.data + entry.normal.byteOffset;
        auto *fileEnd = document.file.end_ptr();
        if (start + 6 >= fileEnd) {
            return LoadObjectResult::error("Malformed cross reference table entry");
        }

        size_t length = 0;
        while (std::string_view((char *)start + length, 6) != "endobj") {
            length++;
            if (start + length + 6 >= fileEnd) {
                return LoadObjectResult::error("Unexpectedly reached end of file");
            }
        }
        length += 6;

        if (start + length >= fileEnd) {
            return LoadObjectResult::error("Unexpectedly reached end of file");
        }

        auto input  = std::string_view((char *)start, length);
        auto text   = StringTextProvider(input);
        auto lexer  = TextLexer(text);
        auto parser = Parser(lexer, document.allocator.arena(), &document);
        auto object = parser.parse();
        if (object == nullptr) {
            // TODO make this error more descriptive
            return LoadObjectResult::error("Failed to load object (parsing failed)");
        }
        if (!object->is<IndirectObject>()) {
            return LoadObjectResult::error("Expected INDIRECT_OBJECT, but got {} instead", object->type_string());
        }

        return LoadObjectResult::ok({object->as<IndirectObject>(), input});
    }

    if (entry.type == CrossReferenceEntryType::COMPRESSED) {
        auto streamObject = document.objectList[entry.compressed.objectNumberOfStream];
        ASSERT(streamObject != nullptr);
        auto stream = streamObject->object->as<Stream>();
        ASSERT(stream->dictionary->must_find<Name>("Type")->value == "ObjStm");

        auto content      = stream->decode(document.allocator);
        auto textProvider = StringTextProvider(content);
        auto lexer        = TextLexer(textProvider);
        auto parser       = Parser(lexer, document.allocator.arena(), &document);
        int64_t N         = stream->dictionary->must_find<Integer>("N")->value;

        auto temp          = document.allocator.temporary();
        auto objectNumbers = Vector<int64_t>(N, temp);
        for (int i = 0; i < N; i++) {
            auto objNum      = parser.parse()->as<Integer>();
            objectNumbers[i] = objNum->value;
            // parse the byteOffset as well
            parser.parse();
        }

        auto objs = Vector<Object *>(N, temp);
        for (int i = 0; i < N; i++) {
            auto obj = parser.parse();
            objs[i]  = obj;
        }

        auto object = document.allocator.arena().push<IndirectObject>( //
              objectNumbers[entry.compressed.indexInStream],           //
              0,                                                       //
              objs[entry.compressed.indexInStream]                     //
        );
        // TODO the content does not refer to the original PDF document, but instead to a decoded stream
        return LoadObjectResult::ok({object, content});
    }
    ASSERT(false);
}

Result load_all_objects(Document &document, Trailer *trailer) {
    if (trailer == nullptr) {
        return Result::error("Cannot load objects without trailer information (trailer=nullptr)");
    }

    struct NumberedCrossReferenceEntry {
        CrossReferenceEntry entry;
        uint64_t objectNumber;
    };
    auto temp              = document.allocator.temporary();
    auto compressedEntries = Vector<NumberedCrossReferenceEntry>(temp);
    auto &crt              = trailer->crossReferenceTable;

    for (uint64_t objectNumber = crt.firstObjectNumber;
         objectNumber < static_cast<uint64_t>(crt.firstObjectNumber + crt.objectCount); objectNumber++) {
        auto itr = document.objectList.find(objectNumber);
        if (itr != document.objectList.end() && itr->second != nullptr) {
            continue;
        }
        CrossReferenceEntry &entry = crt.entries[objectNumber - crt.firstObjectNumber];
        if (entry.type == CrossReferenceEntryType::COMPRESSED) {
            compressedEntries.push_back({.entry = entry, .objectNumber = objectNumber});
            continue;
        }

        auto result = load_object(document, entry);
        if (result.has_error()) {
            return result.drop_value();
        }

        const auto &object                           = result.value();
        document.objectList[objectNumber]            = object.first;
        document.file.metadata.objects[object.first] = {.data = object.second, .isInObjectStream = false};
    }

    for (auto &compressedEntry : compressedEntries) {
        auto result = load_object(document, compressedEntry.entry);
        if (result.has_error()) {
            return result.drop_value();
        }

        const auto &object                                = result.value();
        document.objectList[compressedEntry.objectNumber] = object.first;
        document.file.metadata.objects[object.first]      = {.data = object.second, .isInObjectStream = true};
    }

    return Result::ok();
}

Result read_data(Document &document, bool loadAllObjects) {
    if (document.file.sizeInBytes < 12) {
        return Result::error("File is too short: {} bytes", document.file.sizeInBytes);
    }

    const auto header = std::string_view((char *)document.file.data, 7);
    if (header != "%PDF-1." && header != "%PDF-2.") {
        return Result::error("Missing PDF header");
    }

    // parse eof
    size_t eofMarkerLength = 5;
    auto eofMarkerStart    = document.file.data + (document.file.sizeInBytes - eofMarkerLength);
    if (document.file.data[document.file.sizeInBytes - 1] == '\n' ||
        document.file.data[document.file.sizeInBytes - 1] == '\r') {
        eofMarkerStart--;
    }
    if (document.file.data[document.file.sizeInBytes - 2] == '\r') {
        eofMarkerStart--;
    }
    if (std::string_view((char *)eofMarkerStart, eofMarkerLength) != "%%EOF") {
        return Result::error("Last line did not have '%%EOF'");
    }

    auto *lastCrossRefStartPtr = eofMarkerStart - 3;
    while ((*lastCrossRefStartPtr != '\n' && *lastCrossRefStartPtr != '\r') &&
           document.file.data < lastCrossRefStartPtr) {
        lastCrossRefStartPtr--;
    }
    if (document.file.data == lastCrossRefStartPtr) {
        return Result::error("Unexpectedly reached start of file");
    }
    lastCrossRefStartPtr++;

    // parse startxref
    try {
        const auto str = std::string((char *)lastCrossRefStartPtr, eofMarkerStart - 1 - lastCrossRefStartPtr);
        document.file.lastCrossRefStart = std::stoll(str);
    } catch (std::invalid_argument &err) {
        return Result::error("Failed to parse byte offset of cross reference table (std::invalid_argument): {}",
                             err.what());
    } catch (std::out_of_range &err) {
        return Result::error("Failed to parse byte offset of cross reference table (std::out_of_range): {}",
                             err.what());
    }

    auto crossRefStartPtr = document.file.data + document.file.lastCrossRefStart;
    if (crossRefStartPtr >= document.file.data + document.file.sizeInBytes) {
        return Result::error("Invalid cross reference start");
    }

    auto result = read_trailers(document, crossRefStartPtr, &document.file.trailer);
    if (result.has_error()) {
        return result;
    }
    if (!loadAllObjects) {
        return Result::ok();
    }

    return load_all_objects(document, &document.file.trailer);
}

ValueResult<Document> Document::read_from_file(const std::string &filePath, bool loadAllObjects) {
    auto allocatorResult = Allocator::create();
    if (allocatorResult.has_error()) {
        return ValueResult<Document>::error("failed to create memory arena: {}", allocatorResult.message());
    }

    auto is = std::ifstream(filePath, std::ios::in | std::ifstream::ate | std::ios::binary);
    if (!is.is_open()) {
        return ValueResult<Document>::error("failed to open pdf file for reading: '{}'", filePath);
    }

    auto document             = Document(allocatorResult.value());
    document.file.path        = filePath;
    document.file.sizeInBytes = is.tellg();
    document.file.data        = document.allocator.arena().push(document.file.sizeInBytes);

    is.seekg(0);
    is.read((char *)document.file.data, static_cast<std::streamsize>(document.file.sizeInBytes));
    is.close();

    const auto readResult = read_data(document, loadAllObjects);
    if (readResult.has_error()) {
        return ValueResult<Document>::error("failed to read document: {}", readResult.message());
    }

    return ValueResult<Document>::ok(std::move(document));
}

ValueResult<Document> Document::read_from_memory(const uint8_t *buffer, size_t size, bool loadAllObjects) {
    auto allocatorResult = Allocator::create();
    if (allocatorResult.has_error()) {
        return ValueResult<Document>::error("failed to create memory arena: {}", allocatorResult.message());
    }

    auto document             = Document(allocatorResult.value());
    document.file.data        = document.allocator.arena().push(size);
    document.file.sizeInBytes = size;

    memcpy(document.file.data, buffer, size);

    const auto readResult = read_data(document, loadAllObjects);
    if (readResult.has_error()) {
        return ValueResult<Document>::error("failed to read document: {}", readResult.message());
    }

    return ValueResult<Document>::ok(std::move(document));
}

} // namespace pdf
