#include "objects.h"

#include <cstring>
#include <spdlog/spdlog.h>
#include <zlib.h>

#include "document.h"

namespace pdf {

std::ostream &operator<<(std::ostream &os, Object::Type &type) {
#define __BYTECODE_OP(op)                                                                                              \
    case Object::Type::op:                                                                                             \
        os.write("Object::Type::" #op, strlen("Object::Type::" #op));                                                  \
        break;

    switch (type) {
        ENUMERATE_OBJECT_TYPES(__BYTECODE_OP)
    default:
        ASSERT(false);
    }
#undef __BYTECODE_OP
    return os;
}

std::string to_string(Object *object) {
    if (object == nullptr) {
        return "null";
    }

    switch (object->type) {
    case Object::Type::LITERAL_STRING:
    case Object::Type::BOOLEAN:
    case Object::Type::INTEGER:
    case Object::Type::HEXADECIMAL_STRING:
    case Object::Type::NAME:
    case Object::Type::REAL:
    case Object::Type::NULL_OBJECT:
    case Object::Type::INDIRECT_REFERENCE:
        return std::string(object->data);
    case Object::Type::ARRAY: {
        std::string result = "[ ";
        for (auto &entry : object->as<Array>()->values) {
            result += to_string(entry);
            result += ", ";
        }
        result += "]";
        return result;
    }
    case Object::Type::DICTIONARY: {
        std::string result = "<<\n";
        auto entries       = std::vector<StaticMap<std::string_view, Object *>::Entry>();
        entries.reserve(object->as<Dictionary>()->values.size());
        for (auto &entry : object->as<Dictionary>()->values) {
            entries.emplace_back(entry);
        }
        std::sort(entries.begin(), entries.end(),
                  [](const StaticMap<std::string_view, Object *>::Entry &a,
                     const StaticMap<std::string_view, Object *>::Entry &b) { return a.key < b.key; });
        for (auto &entry : entries) {
            result += "  ";
            result += entry.key;
            result += ": ";
            result += to_string(entry.value);
            result += "\n";
        }
        result += ">>";
        return result;
    }
    case Object::Type::OBJECT:
    case Object::Type::INDIRECT_OBJECT:
    case Object::Type::STREAM:
    case Object::Type::OBJECT_STREAM_CONTENT:
        break;
    }

    return "";
}

std::vector<std::string> Stream::filters() const {
    auto opt = dictionary->values.find("Filter");
    if (!opt.has_value()) {
        return {};
    }

    if (opt.value()->is<Name>()) {
        // TODO is this conversion to a string really necessary?
        return {std::string(opt.value()->as<Name>()->value())};
    }

    auto array  = opt.value()->as<Array>();
    auto result = std::vector<std::string>();
    result.reserve(array->values.size());
    for (auto filter : array->values) {
        // TODO is this conversion to a string really necessary?
        result.emplace_back(filter->as<Name>()->value());
    }
    return result;
}

std::string_view Stream::decode(Allocator &allocator) {
    if (decodedStream != nullptr) {
        return {decodedStream, decodedStreamSize};
    }

    const char *output = streamData.data();
    size_t outputSize  = streamData.length();

    auto fs = filters();
    if (fs.empty()) {
        return streamData;
    }

    for (const auto &filter : fs) {
        if (filter == "FlateDecode") {
            // FIXME implement handling of "DecodeParms" from stream dictionary
            //  Example values: Predictor=12, Columns=4

            // TODO this works, but is not optimal
            const char *input = output;
            size_t inputSize  = outputSize;

            outputSize = inputSize * 3;
            output     = allocator.allocate_chunk(outputSize);

            z_stream infstream;
            infstream.zalloc    = Z_NULL;
            infstream.zfree     = Z_NULL;
            infstream.opaque    = Z_NULL;
            infstream.avail_in  = (uInt)inputSize;  // size of input
            infstream.next_in   = (Bytef *)input;   // input char array
            infstream.avail_out = (uInt)outputSize; // size of output
            infstream.next_out  = (Bytef *)output;  // output char array

            // TODO error handling
            inflateInit(&infstream);
            inflate(&infstream, Z_NO_FLUSH);
            inflateEnd(&infstream);

            outputSize = infstream.total_out;

            // copy the result into a buffer that fits exactly
            char *tmp = allocator.allocate_chunk(outputSize);
            memcpy(tmp, output, outputSize);
            output = tmp;
        } else {
            spdlog::error("Unknown filter: {}", filter);
            ASSERT(false);
        }
        // TODO handle more filters
    }

    decodedStream     = output;
    decodedStreamSize = outputSize;
    return {output, outputSize};
}

std::string HexadecimalString::to_string() const {
    // TODO this is quite hacky
    // TODO is this conversion to a string really necessary?
    std::string tmp = std::string(data);
    if (tmp.size() % 2 == 1) {
        tmp += "0";
    }

    char buf[4] = {};
    char *low   = buf;
    low[1]      = '\0';
    char *high  = buf + 2;
    high[1]     = '\0';

    std::string result;
    for (size_t i = 0; i < tmp.size(); i += 2) {
        *low          = tmp[i + 1];
        *high         = tmp[i];
        auto lowVal   = std::strtol(low, nullptr, 16);
        auto highVal  = std::strtol(high, nullptr, 16);
        char nextChar = (char)(lowVal + 16 * highVal);
        result += nextChar;
    }

    return result;
}

void Array::remove_element(Document &document, size_t index) {
    ASSERT(index < values.size());
    document.delete_raw_section(values[index]->data);
    values.remove(index);
}

void Integer::set(Document &document, int64_t i) {
    value = i;
    document.delete_raw_section(data);

    // TODO avoid allocating twice here
    auto s            = std::to_string(i);
    char *new_content = document.allocator.allocate_chunk(s.size());
    memcpy(new_content, s.data(), s.size());

    // TODO can we do this without const-casting here?
    const char *insertionPoint = data.data();

    // NOTE: moving to the end of the data section, because it gets deleted right before insertion
    insertionPoint += data.size();

    document.add_raw_section(insertionPoint, new_content, s.size());
}

std::string_view EmbeddedFile::name() {
    auto fileMetadata = dictionary->must_find<Dictionary>("FileMetadata");
    return fileMetadata->must_find<LiteralString>("Name")->value();
}

bool EmbeddedFile::is_executable() {
    auto fileMetadata = dictionary->must_find<Dictionary>("FileMetadata");
    return fileMetadata->must_find<Boolean>("Executable")->value;
}

} // namespace pdf
