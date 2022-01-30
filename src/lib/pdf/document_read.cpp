#include "document.h"

#include <fstream>
#include <spdlog/spdlog.h>
#include <sstream>

namespace pdf {

void ignoreNewLines(char *&ptr) {
    if (*ptr == '\r') {
        ptr++;
    }
    if (*ptr == '\n') {
        ptr++;
    }
}

Dictionary *parseDict(Allocator &allocator, char *start, size_t length) {
    ASSERT(start != nullptr);
    ASSERT(length > 0);
    const std::string_view input = std::string_view(start, length);
    auto text                    = StringTextProvider(input);
    auto lexer                   = TextLexer(text);
    auto parser                  = Parser(lexer, allocator);
    auto result                  = parser.parse();
    ASSERT(result != nullptr);
    return result->as<Dictionary>();
}

Stream *parseStream(Allocator &allocator, char *start, size_t length) {
    ASSERT(start != nullptr);
    ASSERT(length > 0);
    const std::string_view input = std::string_view(start, length);
    auto text                    = StringTextProvider(input);
    auto lexer                   = TextLexer(text);
    auto parser                  = Parser(lexer, allocator);
    auto result                  = parser.parse();
    ASSERT(result != nullptr);
    return result->as<IndirectObject>()->object->as<Stream>();
}

Result read_trailers(Document &document, char *crossRefStartPtr, Trailer *currentTrailer);
Result read_cross_reference_stream(Document &document, Stream *stream, Trailer *currentTrailer) {
    auto W          = stream->dictionary->must_find<Array>("W");
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
        }
    } else {
        currentTrailer->crossReferenceTable.firstObjectNumber = 0;
        currentTrailer->crossReferenceTable.objectCount       = static_cast<int64_t>(countInDict);
    }

    for (auto contentPtr = content.data(); contentPtr < content.data() + content.size();
         contentPtr += sizeField0 + sizeField1 + sizeField2) {
        uint64_t type = 0;
        if (sizeField0 == 0) {
            type = 1; // default value for type
        }

        for (int j = 0; j < sizeField0; j++) {
            uint8_t c      = *(contentPtr + j);
            uint64_t shift = (sizeField0 - (j + 1)) * 8;
            type |= c << shift;
        }

        uint64_t field1 = 0;
        for (int j = 0; j < sizeField1; j++) {
            uint8_t c      = *(contentPtr + j + sizeField0);
            uint64_t shift = (sizeField1 - (j + 1)) * 8;
            field1 |= c << shift;
        }

        uint64_t field2 = 0;
        for (int j = 0; j < sizeField2; j++) {
            uint8_t c      = *(contentPtr + j + sizeField0 + sizeField1);
            uint64_t shift = (sizeField2 - (j + 1)) * 8;
            field2 |= c << shift;
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
            spdlog::warn("Encountered unknown cross reference stream entry field type: {}", type);
            break;
        }
    }

    auto opt = stream->dictionary->values.find("Prev");
    if (!opt.has_value()) {
        return Result::ok();
    }

    currentTrailer->prev = document.allocator.allocate<Trailer>();
    return read_trailers(document, document.file.data + opt.value()->as<Integer>()->value, currentTrailer->prev);
}

Result read_trailers(Document &document, char *crossRefStartPtr, Trailer *currentTrailer) {
    // decide whether xref stream or table
    const auto xrefKeyword = std::string_view(crossRefStartPtr, 4);
    if (xrefKeyword != "xref") {
        //  stream -> parse stream
        // TODO how long is the stream? (just using the end of the file for parsing purposes)
        auto startxrefPtr      = document.file.data + document.file.sizeInBytes;
        auto startOfStream     = crossRefStartPtr;
        size_t lengthOfStream  = startxrefPtr - startOfStream;
        currentTrailer->stream = parseStream(document.allocator, startOfStream, lengthOfStream);
        document.file.metadata.trailers[currentTrailer] = std::string_view(startOfStream, lengthOfStream);
        return read_cross_reference_stream(document, currentTrailer->stream, currentTrailer);
    }

    //  table -> parse table and parse trailer dict
    auto crossRefPtr = crossRefStartPtr + 4;
    ignoreNewLines(crossRefPtr);

    int64_t spaceLocation = -1;
    char *currentReadPtr  = crossRefPtr;
    while (*currentReadPtr != '\n' && *currentReadPtr != '\r') {
        if (*currentReadPtr == ' ') {
            spaceLocation = currentReadPtr - crossRefPtr;
        }
        currentReadPtr++;
    }

    auto metaData = std::string(crossRefPtr, currentReadPtr - crossRefPtr);
    // TODO parse other cross-reference sections
    // TODO catch exceptions
    currentTrailer->crossReferenceTable.firstObjectNumber = std::stoll(metaData.substr(0, spaceLocation));
    currentTrailer->crossReferenceTable.objectCount       = std::stoll(metaData.substr(spaceLocation));

    ignoreNewLines(currentReadPtr);

    for (int i = 0; i < currentTrailer->crossReferenceTable.objectCount; i++) {
        // nnnnnnnnnn ggggg f__
        auto s = std::string(currentReadPtr, 20);
        // TODO catch exceptions
        uint64_t num0 = std::stoll(s.substr(0, 10));
        uint64_t num1 = std::stoll(s.substr(11, 16));

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
    while (std::string_view(currentReadPtr, 7) != "trailer") {
        currentReadPtr++;
        if (document.file.data + document.file.sizeInBytes < currentReadPtr + 7) {
            return Result::error("Unexpectedly reached end of file");
        }
    }

    currentReadPtr += 7;
    ignoreNewLines(currentReadPtr);

    size_t lengthOfTrailerDict = 1;
    while (std::string_view(currentReadPtr + lengthOfTrailerDict, 9) != "startxref") {
        lengthOfTrailerDict++;
        if (document.file.data + document.file.sizeInBytes < currentReadPtr + lengthOfTrailerDict) {
            return Result::error("Unexpectedly reached end of file");
        }
    }

    document.file.metadata.trailers[currentTrailer] =
          std::string_view(crossRefStartPtr, (currentReadPtr - crossRefStartPtr) + lengthOfTrailerDict);
    currentTrailer->dict = parseDict(document.allocator, currentReadPtr, lengthOfTrailerDict);
    auto opt             = currentTrailer->dict->values.find("Prev");
    if (!opt.has_value()) {
        return Result::ok();
    }

    currentTrailer->prev = document.allocator.allocate<Trailer>();
    return read_trailers(document, document.file.data + opt.value()->as<Integer>()->value, currentTrailer->prev);
}

std::pair<IndirectObject *, std::string_view> load_object(Document &document, CrossReferenceEntry &entry) {
    if (entry.type == CrossReferenceEntryType::FREE) {
        return {nullptr, ""};
    }

    if (entry.type == CrossReferenceEntryType::NORMAL) {
        auto start = document.file.data + entry.normal.byteOffset;

        // TODO this is dangerous (it might read past the end of the stream)
        size_t length = 0;
        while (std::string_view(start + length, 6) != "endobj") {
            length++;
        }
        length += 6;

        auto input  = std::string_view(start, length);
        auto text   = StringTextProvider(input);
        auto lexer  = TextLexer(text);
        auto parser = Parser(lexer, document.allocator, &document);
        auto result = parser.parse();
        ASSERT(result != nullptr);
        return {result->as<IndirectObject>(), input};
    }

    if (entry.type == CrossReferenceEntryType::COMPRESSED) {
        auto streamObject = document.objectList[entry.compressed.objectNumberOfStream];
        ASSERT(streamObject != nullptr);
        auto stream = streamObject->object->as<Stream>();
        ASSERT(stream->dictionary->must_find<Name>("Type")->value == "ObjStm");

        auto content      = stream->decode(document.allocator);
        auto textProvider = StringTextProvider(content);
        auto lexer        = TextLexer(textProvider);
        auto parser       = Parser(lexer, document.allocator, &document);
        int64_t N         = stream->dictionary->must_find<Integer>("N")->value;

        auto objectNumbers = std::vector<int64_t>(N);
        for (int i = 0; i < N; i++) {
            auto objNum      = parser.parse()->as<Integer>();
            objectNumbers[i] = objNum->value;
            // parse the byteOffset as well
            parser.parse();
        }

        auto objs = std::vector<Object *>(N);
        for (int i = 0; i < N; i++) {
            auto obj = parser.parse();
            objs[i]  = obj;
        }

        auto object = document.allocator.allocate<IndirectObject>(objectNumbers[entry.compressed.indexInStream], 0,
                                                                  objs[entry.compressed.indexInStream]);
        // TODO the content does not refer to the original PDF document, but instead to a decoded stream
        return {object, content};
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
    auto compressedEntries = std::vector<NumberedCrossReferenceEntry>();
    auto &crt              = trailer->crossReferenceTable;
    for (uint64_t objectNumber = crt.firstObjectNumber;
         objectNumber < static_cast<uint64_t>(crt.firstObjectNumber + crt.objectCount); objectNumber++) {
        auto itr = document.objectList.find(objectNumber);
        if (itr != document.objectList.end()) {
            continue;
        }
        CrossReferenceEntry &entry = crt.entries[objectNumber - crt.firstObjectNumber];
        if (entry.type == CrossReferenceEntryType::COMPRESSED) {
            compressedEntries.push_back({.entry = entry, .objectNumber = objectNumber});
            continue;
        }

        const std::pair<IndirectObject *, std::string_view> &object = load_object(document, entry);
        document.objectList[objectNumber]                           = object.first;
        document.file.metadata.objects[object.first] = {.data = object.second, .isInObjectStream = false};
    }

    for (auto &compressedEntry : compressedEntries) {
        const std::pair<IndirectObject *, std::string_view> &object = load_object(document, compressedEntry.entry);
        document.objectList[compressedEntry.objectNumber]           = object.first;
        document.file.metadata.objects[object.first]                       = {.data = object.second, .isInObjectStream = true};
    }

    return Result::ok();
}

Result read_data(Document &document, bool loadAllObjects) {
    // parse eof
    size_t eofMarkerLength = 5;
    auto eofMarkerStart    = document.file.data + (document.file.sizeInBytes - eofMarkerLength);
    if (document.file.data[document.file.sizeInBytes - 1] == '\n') {
        eofMarkerStart--;
    }
    if (document.file.data[document.file.sizeInBytes - 2] == '\r') {
        eofMarkerStart--;
    }
    if (std::string_view(eofMarkerStart, eofMarkerLength) != "%%EOF") {
        return Result::error("Last line did not have '%%EOF'");
    }

    char *lastCrossRefStartPtr = eofMarkerStart - 3;
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
        const auto str                  = std::string(lastCrossRefStartPtr, eofMarkerStart - 1 - lastCrossRefStartPtr);
        document.file.lastCrossRefStart = std::stoll(str);
    } catch (std::invalid_argument &err) {
        return Result::error("Failed to parse byte offset of cross reference table (std::invalid_argument): {}",
                             err.what());
    } catch (std::out_of_range &err) {
        return Result::error("Failed to parse byte offset of cross reference table (std::out_of_range): {}",
                             err.what());
    }

    auto crossRefStartPtr = document.file.data + document.file.lastCrossRefStart;
    auto trailersMetadata = std::vector<std::string_view>();
    auto result           = read_trailers(document, crossRefStartPtr, &document.file.trailer);
    if (result.has_error()) {
        return result;
    }
    if (!loadAllObjects) {
        return Result::ok();
    }

    return load_all_objects(document, &document.file.trailer);
}

Result Document::read_from_file(const std::string &filePath, Document &document, bool loadAllObjects) {
    auto is = std::ifstream();
    is.open(filePath, std::ios::in | std::ifstream::ate | std::ios::binary);

    if (!is.is_open()) {
        return Result::error("Failed to open pdf file for reading: '{}'", filePath);
    }

    document.file.sizeInBytes = is.tellg();
    document.allocator.init(document.file.sizeInBytes);
    document.file.data = document.allocator.allocate_chunk(document.file.sizeInBytes);

    is.seekg(0);
    is.read(document.file.data, static_cast<std::streamsize>(document.file.sizeInBytes));
    is.close();

    return read_data(document, loadAllObjects);
}

Result Document::read_from_memory(char *buffer, size_t size, Document &document, bool loadAllObjects) {
    // FIXME using the existing buffer collides with the memory management using the Allocator
    document.file.data        = buffer;
    document.file.sizeInBytes = size;
    document.allocator.init(document.file.sizeInBytes);

    return read_data(document, loadAllObjects);
}

} // namespace pdf
