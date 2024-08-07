#include "objects.h"

#include <cstring>
#include <spdlog/spdlog.h>
#include <zlib.h>

#include "pdf/document.h"

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

#if 0
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
        return "";
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
#endif

std::vector<std::string> Stream::filters() const {
    auto itr = dictionary->values.find("Filter");
    if (itr == dictionary->values.end()) {
        return {};
    }

    if (itr->second->is<Name>()) {
        // TODO is this conversion to a string really necessary?
        return {std::string(itr->second->as<Name>()->value)};
    }

    auto array  = itr->second->as<Array>();
    auto result = std::vector<std::string>();
    result.reserve(array->values.size());
    for (auto filter : array->values) {
        // TODO is this conversion to a string really necessary?
        result.emplace_back(filter->as<Name>()->value);
    }
    return result;
}

std::string_view Stream::decode(Allocator &allocator) {
    if (decodedStream != nullptr) {
        return {(char *)decodedStream, decodedStreamSize};
    }

    auto *output      = (uint8_t *)const_cast<char *>(streamData.data());
    size_t outputSize = streamData.length();

    auto fs = filters();
    if (fs.empty()) {
        return streamData;
    }

    for (const auto &filter : fs) {
        if (filter == "FlateDecode") {
            // FIXME implement handling of "DecodeParms" from stream dictionary
            //  Example values: Predictor=12, Columns=4

            auto *input      = output;
            size_t inputSize = outputSize;

            auto temp          = allocator.temporary();
            output                  = temp.arena().push(outputSize);
            const auto *firstOutput = output;

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
            while (true) {
                inflate(&infstream, Z_NO_FLUSH);
                if (infstream.avail_out != 0) {
                    break;
                }

                output              = temp.arena().push(outputSize);
                infstream.avail_out = (uInt)outputSize;
                infstream.next_out  = (Bytef *)output;
            }

            inflateEnd(&infstream);

            // copy the result into a buffer that fits exactly
            auto *tmp = allocator.arena().push(infstream.total_out);
            // NOTE the arena allocator allocates contiguous memory, which is why this works
            memcpy(tmp, firstOutput, infstream.total_out);

            output     = tmp;
            outputSize = infstream.total_out;
        } else {
            // TODO handle more filters
            spdlog::error("Unknown filter: {}", filter);
            ASSERT(false);
        }
    }

    decodedStream     = output;
    decodedStreamSize = outputSize;

    return {(char *)decodedStream, decodedStreamSize};
}

std::string_view deflate_buffer(Allocator &allocator, const uint8_t *srcData, size_t srcSize) {
    auto temp = allocator.temporary();
    auto tempSize  = srcSize * 2;
    auto tempData  = temp.arena().push(tempSize);

    z_stream stream  = {};
    stream.zalloc    = Z_NULL;
    stream.zfree     = Z_NULL;
    stream.opaque    = Z_NULL;
    stream.avail_in  = (uInt)srcSize;     // size of input
    stream.next_in   = (Bytef *)srcData;  // input char array
    stream.avail_out = (uInt)tempSize;    // size of output
    stream.next_out  = (Bytef *)tempData; // output char array

    auto ret = deflateInit(&stream, Z_BEST_COMPRESSION);
    if (ret != Z_OK) {
        // TODO error handling
        return {};
    }

    ret = deflate(&stream, Z_FINISH);
    if (ret != Z_STREAM_END) {
        // TODO error handling
        return {};
    }

    ret = deflateEnd(&stream);
    if (ret != Z_OK) {
        // TODO error handling
        return {};
    }

    auto resultSize = stream.total_out;
    auto resultData = allocator.arena().push(resultSize);
    std::memcpy(resultData, tempData, resultSize);
    return {(char *)resultData, resultSize};
}

void Stream::encode(Allocator &allocator, const std::string &data) {
    decodedStream                = nullptr;
    streamData                   = deflate_buffer(allocator, (uint8_t *)data.data(), data.size());
    dictionary->values["Length"] = allocator.arena().push<Integer>(streamData.size());
}

Stream *Stream::create_from_unencoded_data(Allocator &allocator,
                                           const UnorderedMap<std::string, Object *> &additionalDictionaryEntries,
                                           std::string_view unencodedData) {
    auto streamData = deflate_buffer(allocator, (uint8_t *)unencodedData.data(), unencodedData.size());

    auto dict = UnorderedMap<std::string, Object *>(allocator);
    for (const auto &entry : additionalDictionaryEntries) {
        dict[entry.first] = entry.second;
    }

    auto &arena    = allocator.arena();
    dict["Length"] = arena.push<Integer>(streamData.size());
    dict["Filter"] = arena.push<Name>("FlateDecode");

    auto dictionary = arena.push<Dictionary>(dict);
    return arena.push<Stream>(dictionary, streamData);
}

std::string HexadecimalString::to_string() const {
    // TODO this is quite hacky
    std::string tmp = value;
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

void Array::remove_element(Document & /*document*/, size_t index) {
    ASSERT(index < values.size());
    values.erase(values.begin() + index);
}

void Integer::set(Document & /*document*/, int64_t i) { value = i; }

std::optional<int64_t> EmbeddedFile::size() {
    const auto &paramsOpt = dictionary->find<Dictionary>("Params");
    if (!paramsOpt.has_value()) {
        return {};
    }

    const auto &sizeOpt = paramsOpt.value()->find<Integer>("Size");
    if (!sizeOpt.has_value()) {
        return {};
    }

    return {sizeOpt.value()->value};
}

std::string Object::type_string() const {
    switch (type) {
#define DECLARE_CASE(Name)                                                                                             \
    case Type::Name:                                                                                                   \
        return #Name;
        ENUMERATE_OBJECT_TYPES(DECLARE_CASE)
#undef DECLARE_CASE
    }
    return "unknown";
}

} // namespace pdf
