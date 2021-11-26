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

bool Document::read_cross_reference_stream(Stream *stream, Trailer *currentTrailer) {
    auto W          = stream->dictionary->must_find<Array>("W");
    auto sizeField0 = W->values[0]->as<Integer>()->value;
    auto sizeField1 = W->values[1]->as<Integer>()->value;
    auto sizeField2 = W->values[2]->as<Integer>()->value;
    auto content    = stream->decode(allocator);

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
        return false;
    }

    currentTrailer->prev = allocator.allocate<Trailer>();
    return read_trailers(data + opt.value()->as<Integer>()->value, currentTrailer->prev);
}

bool Document::read_trailers(char *crossRefStartPtr, Trailer *currentTrailer) {
    // decide whether xref stream or table
    const auto xrefKeyword = std::string_view(crossRefStartPtr, 4);
    if (xrefKeyword != "xref") {
        //  stream -> parse stream
        // TODO how long is the stream? (just using the end of the file for parsing purposes)
        auto startxrefPtr      = data + sizeInBytes;
        auto startOfStream     = crossRefStartPtr;
        size_t lengthOfStream  = startxrefPtr - startOfStream;
        currentTrailer->stream = parseStream(allocator, startOfStream, lengthOfStream);
        return read_cross_reference_stream(currentTrailer->stream, currentTrailer);
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
        if (data + sizeInBytes < currentReadPtr + 7) {
            spdlog::error("Unexpectedly reached end of file");
            return true;
        }
    }

    currentReadPtr += 7;
    ignoreNewLines(currentReadPtr);

    size_t lengthOfTrailerDict = 1;
    while (std::string_view(currentReadPtr + lengthOfTrailerDict, 9) != "startxref") {
        lengthOfTrailerDict++;
        if (data + sizeInBytes < currentReadPtr + lengthOfTrailerDict) {
            spdlog::error("Unexpectedly reached end of file");
            return true;
        }
    }

    currentTrailer->dict = parseDict(allocator, currentReadPtr, lengthOfTrailerDict);
    auto opt             = currentTrailer->dict->values.find("Prev");
    if (!opt.has_value()) {
        return false;
    }

    currentTrailer->prev = allocator.allocate<Trailer>();
    return read_trailers(data + opt.value()->as<Integer>()->value, currentTrailer->prev);
}

bool Document::read_data() {
    // parse eof
    size_t eofMarkerLength = 5;
    auto eofMarkerStart    = data + (sizeInBytes - eofMarkerLength);
    if (data[sizeInBytes - 1] == '\n') {
        eofMarkerStart--;
    }
    if (data[sizeInBytes - 2] == '\r') {
        eofMarkerStart--;
    }
    if (std::string_view(eofMarkerStart, eofMarkerLength) != "%%EOF") {
        spdlog::error("Last line did not have '%%EOF'");
        return true;
    }

    char *lastCrossRefStartPtr = eofMarkerStart - 3;
    while ((*lastCrossRefStartPtr != '\n' && *lastCrossRefStartPtr != '\r') && data < lastCrossRefStartPtr) {
        lastCrossRefStartPtr--;
    }
    if (data == lastCrossRefStartPtr) {
        spdlog::error("Unexpectedly reached start of file");
        return true;
    }
    lastCrossRefStartPtr++;

    // parse startxref
    try {
        const auto str    = std::string(lastCrossRefStartPtr, eofMarkerStart - 1 - lastCrossRefStartPtr);
        lastCrossRefStart = std::stoll(str);
    } catch (std::invalid_argument &err) {
        spdlog::error("Failed to parse byte offset of cross reference table (std::invalid_argument): {}", err.what());
        return true;
    } catch (std::out_of_range &err) {
        spdlog::error("Failed to parse byte offset of cross reference table (std::out_of_range): {}", err.what());
        return true;
    }

    auto crossRefStartPtr = data + lastCrossRefStart;
    return read_trailers(crossRefStartPtr, &trailer);
}

bool Document::read_from_file(const std::string &filePath, Document &document) {
    auto is = std::ifstream();
    is.open(filePath, std::ios::in | std::ifstream::ate | std::ios::binary);

    if (!is.is_open()) {
        spdlog::error("Failed to open pdf file for reading: '{}'", filePath);
        return true;
    }

    document.sizeInBytes = is.tellg();
    document.allocator.init(document.sizeInBytes);
    document.data = document.allocator.allocate_chunk(document.sizeInBytes);

    is.seekg(0);
    is.read(document.data, static_cast<std::streamsize>(document.sizeInBytes));
    is.close();

    return document.read_data();
}

bool Document::read_from_memory(char *buffer, size_t size, Document &document) {
    // FIXME using the existing buffer collides with the memory management using the Allocator
    document.data        = buffer;
    document.sizeInBytes = size;
    document.allocator.init(document.sizeInBytes);

    return document.read_data();
}

bool Document::write_to_stream(std::ostream &s) {
    if (changeSections.empty()) {
        s.write(data, static_cast<std::streamsize>(sizeInBytes));
        return s.bad();
    }

    std::sort(changeSections.begin(), changeSections.end(),
              [](const ChangeSection &a, const ChangeSection &b) { return a.start_pointer() < b.start_pointer(); });

    auto ptr                     = data;
    size_t bytesWrittenUntilXref = 0;
    write_content(s, ptr, bytesWrittenUntilXref);
    if (s.bad()) {
        return true;
    }

    // NOTE write everything up until the cross-reference table
    ASSERT(ptr <= data + lastCrossRefStart);
    auto size = lastCrossRefStart - (ptr - data);
    s.write(ptr, static_cast<std::streamsize>(size));
    bytesWrittenUntilXref += size;

    write_new_cross_ref_table(s);
    write_trailer_dict(s, bytesWrittenUntilXref);
    return s.bad();
}

void Document::write_content(std::ostream &s, char *&ptr, size_t &bytesWrittenUntilXref) {
    for (const auto &section : changeSections) {
        if (section.type == ChangeSectionType::DELETED) {
            // NOTE write content up until the deleted section
            auto size = section.deleted.deleted_area.data() - ptr;
            s.write(ptr, size);
            if (s.bad()) {
                return;
            }
            bytesWrittenUntilXref += size;

            ptr += size;
            ptr += section.deleted.deleted_area.size();
            continue;
        }
        if (section.type == ChangeSectionType::ADDED) {
            auto size = section.added.insertion_point - ptr;
            if (size < 0) {
                size = 0;
            }

            s.write(ptr, size);
            if (s.bad()) {
                return;
            }
            bytesWrittenUntilXref += size;

            ptr += size;
            s.write(section.added.new_content, static_cast<std::streamsize>(section.added.new_content_length));
            if (s.bad()) {
                return;
            }
            bytesWrittenUntilXref += section.added.new_content_length;
            continue;
        }
        ASSERT(false);
    }
}

void write_zero_padded_number(std::ostream &s, uint64_t num, int64_t maxNumDigits) {
    int64_t numDigits = 0;
    if (num != 0) {
        numDigits = static_cast<int64_t>(std::log10(num) + 1);
    }
    ASSERT(numDigits <= maxNumDigits);

    for (int i = 0; i < maxNumDigits - numDigits; i++) {
        s.write("0", 1);
    }

    if (num != 0) {
        auto tmp = std::to_string(num);
        s.write(tmp.c_str(), static_cast<std::streamsize>(tmp.size()));
    }
}

struct TempXRefEntry {
    int64_t objectNumber      = 0;
    CrossReferenceEntry entry = {};
};

bool should_apply_offset(const std::vector<ChangeSection> &changeSections, size_t changeSectionIndex,
                         const TempXRefEntry &crossRefEntry) {
    return changeSectionIndex <= 0 || changeSectionIndex - 1 >= changeSections.size() ||
           changeSections[changeSectionIndex - 1].type != ChangeSectionType::ADDED ||
           crossRefEntry.objectNumber != changeSections[changeSectionIndex - 1].objectNumber;
}

void Document::write_new_cross_ref_table(std::ostream &s) {
    auto crossReferenceEntries = std::vector<TempXRefEntry>(trailer.crossReferenceTable.entries.size());
    for (int i = 0; i < static_cast<int>(trailer.crossReferenceTable.entries.size()); i++) {
        crossReferenceEntries[i] = {
              .objectNumber = i + trailer.crossReferenceTable.firstObjectNumber,
              .entry        = trailer.crossReferenceTable.entries[i],
        };
    }

    std::sort(crossReferenceEntries.begin(), crossReferenceEntries.end(),
              [](const TempXRefEntry &a, const TempXRefEntry &b) {
                  if (a.entry.type == CrossReferenceEntryType::FREE ||
                      b.entry.type == CrossReferenceEntryType::COMPRESSED) {
                      return true;
                  }
                  if (b.entry.type == CrossReferenceEntryType::FREE ||
                      a.entry.type == CrossReferenceEntryType::COMPRESSED) {
                      return false;
                  }
                  ASSERT(a.entry.type == CrossReferenceEntryType::NORMAL &&
                         b.entry.type == CrossReferenceEntryType::NORMAL);
                  return a.entry.normal.byteOffset < b.entry.normal.byteOffset;
              });

    /*
     *      ↓       ↓
     * |---------------------|
     *        ↑___
     */

    size_t changeSectionIndex = 0;
    int64_t offset            = 0;
    for (auto &crossRefEntry : crossReferenceEntries) {
        if (crossRefEntry.entry.type == CrossReferenceEntryType::COMPRESSED) {
            // TODO implement support for rewriting compressed cross reference entries
            continue;
        }

        if (crossRefEntry.entry.type == CrossReferenceEntryType::FREE) {
            // TODO is there something that needs to be done here?
            continue;
        }

        ASSERT(crossRefEntry.entry.type == CrossReferenceEntryType::NORMAL);

        while (changeSectionIndex < changeSections.size()) {
            auto &changeSection = changeSections[changeSectionIndex];
            if (changeSection.type == ChangeSectionType::DELETED) {
                size_t deletedOffset = changeSection.deleted.deleted_area.data() - data;
                if (deletedOffset > crossRefEntry.entry.normal.byteOffset) {

                    // TODO add test for this if statement
                    if (should_apply_offset(changeSections, changeSectionIndex, crossRefEntry)) {
                        crossRefEntry.entry.normal.byteOffset += offset;
                    }
                    break;
                }

                offset -= static_cast<int64_t>(changeSection.deleted.deleted_area.size());
                changeSectionIndex++;
                continue;
            }
            if (changeSection.type == ChangeSectionType::ADDED) {
                size_t addedOffset = changeSection.added.insertion_point - data;
                if (addedOffset > crossRefEntry.entry.normal.byteOffset) {
                    crossRefEntry.entry.normal.byteOffset += offset;
                    break;
                }

                offset += static_cast<int64_t>(changeSection.added.new_content_length);
                changeSectionIndex++;
                continue;
            }
            break;
        }

        if (changeSectionIndex >= changeSections.size()) {
            if (should_apply_offset(changeSections, changeSectionIndex, crossRefEntry)) {
                crossRefEntry.entry.normal.byteOffset += offset;
            }
            continue;
        }
    }

    std::sort(crossReferenceEntries.begin(), crossReferenceEntries.end(),
              [](const TempXRefEntry &a, const TempXRefEntry &b) { return a.objectNumber < b.objectNumber; });

    auto xrefKeyword = "xref\n";
    s.write(xrefKeyword, 5);
    auto firstObjectNumberStr = std::to_string(trailer.crossReferenceTable.firstObjectNumber);
    s.write(firstObjectNumberStr.c_str(), static_cast<std::streamsize>(firstObjectNumberStr.size()));
    s.write(" ", 1);
    auto objectCountStr = std::to_string(trailer.crossReferenceTable.objectCount);
    s.write(objectCountStr.c_str(), static_cast<std::streamsize>(objectCountStr.size()));
    s.write("\n", 1);

    for (auto &entry : crossReferenceEntries) {
        // TODO iterate over all entries and write them to the output stream
        if (entry.entry.type == CrossReferenceEntryType::NORMAL) {
            write_zero_padded_number(s, entry.entry.normal.byteOffset, 10);
            s.write(" ", 1);
            write_zero_padded_number(s, entry.entry.normal.generationNumber, 5);
            s.write(" n \n", 4);
            continue;
        }
        if (entry.entry.type == CrossReferenceEntryType::FREE) {
            write_zero_padded_number(s, entry.entry.free.nextFreeObjectNumber, 10);
            s.write(" ", 1);
            write_zero_padded_number(s, entry.entry.free.nextFreeObjectGenerationNumber, 5);
            s.write(" f \n", 4);
            continue;
        }
        ASSERT(false);
    }
}

void Document::write_trailer_dict(std::ostream &s, size_t bytesWrittenUntilXref) const {
    s.write("trailer\n", 8);
    s.write(trailer.dict->data.data(), static_cast<std::streamsize>(trailer.dict->data.size()));
    s.write("\nstartxref\n", 11);
    auto tmp = std::to_string(bytesWrittenUntilXref);
    s.write(tmp.c_str(), static_cast<std::streamsize>(tmp.size()));
    s.write("\n%%EOF", 6);
}

bool Document::write_to_file(const std::string &filePath) {
    auto os = std::ofstream();
    os.open(filePath, std::ios::out | std::ios::binary);

    if (!os.is_open()) {
        spdlog::error("Failed to open pdf file for writing: '{}'", filePath);
        return true;
    }

    auto result = write_to_stream(os);
    os.close();
    return result;
}

bool Document::write_to_memory(char *&buffer, size_t &size) {
    std::stringstream ss;

    auto error = write_to_stream(ss);
    if (error) {
        return true;
    }

    auto result = ss.str();
    size        = result.size();
    buffer      = (char *)malloc(size);
    memcpy(buffer, result.data(), size);
    return false;
}

} // namespace pdf
