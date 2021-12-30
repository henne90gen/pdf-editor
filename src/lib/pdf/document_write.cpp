#include "document.h"

#include <fstream>
#include <spdlog/spdlog.h>
#include <sstream>

namespace pdf {

void write_zero_padded_number(std::ostream &s, uint64_t num, int64_t maxNumDigits) {
    int64_t numDigits = 0;
    if (num != 0) {
        numDigits = static_cast<int64_t>(std::log10(num) + 1);
    }
    ASSERT(numDigits <= maxNumDigits);

    for (int i = 0; i < maxNumDigits - numDigits; i++) {
        s << "0";
    }

    if (num != 0) {
        s << std::to_string(num);
    }
}

void write_header(std::ostream &s) { s << "%PDF-1.6\n%äüöß\n"; }

size_t write_objects(Document &document, std::ostream &s, std::unordered_map<uint64_t, uint64_t> &byteOffsets) {
    size_t currentOffset = 19;
    for (auto &entry : document.objectList) {
        if (entry.second == nullptr) {
            continue;
        }

        s << entry.second->data;
        byteOffsets[entry.first] = currentOffset;
        currentOffset += entry.second->data.size();
    }
    return currentOffset;
}

void write_trailer(Document &document, std::ostream &s, std::unordered_map<uint64_t, uint64_t> &byteOffsets,
                   size_t startXref) {
    s << "\n\nxref ";
    s << 0 << " " << byteOffsets.size();

    s << "0000000000 65535 f \n";
    for (size_t i = 0; i < byteOffsets.size(); i++) {
        write_zero_padded_number(s, byteOffsets[i + 1], 10);
        s << " 00000 n \n";
    }

    s << "trailer\n";
    s << document.file.trailer.dict->data;

    s << "\nstartxref\n" << startXref << "\n%%EOF\n";
}

Result write_to_stream(Document &document, std::ostream &s) {
    // FIXME remove this (it is only to ensure that all objects have been loaded
    auto objs = document.objects();

    write_header(s);

    std::unordered_map<uint64_t, uint64_t> byteOffsets = {};
    auto startXref                                     = write_objects(document, s, byteOffsets);

    write_trailer(document, s, byteOffsets, startXref);

    return Result::bool_(s.bad(), "Failed to write data to file");
}

Result Document::write_to_file(const std::string &filePath) {
    auto os = std::ofstream();
    os.open(filePath, std::ios::out | std::ios::binary);

    if (!os.is_open()) {
        return Result::error("Failed to open pdf file for writing: '{}'", filePath);
    }

    auto result = write_to_stream(*this, os);
    os.close();
    return result;
}

Result Document::write_to_memory(char *&buffer, size_t &size) {
    std::stringstream ss;
    auto error = write_to_stream(*this, ss);
    if (error.has_error()) {
        return error;
    }

    auto result = ss.str();
    size        = result.size();
    buffer      = (char *)malloc(size);
    memcpy(buffer, result.data(), size);
    return Result::ok();
}
} // namespace pdf
