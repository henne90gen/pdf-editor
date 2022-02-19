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
    auto opt = dictionary->values.find("Filter");
    if (!opt.has_value()) {
        return {};
    }

    if (opt.value()->is<Name>()) {
        // TODO is this conversion to a string really necessary?
        return {std::string(opt.value()->as<Name>()->value)};
    }

    auto array  = opt.value()->as<Array>();
    auto result = std::vector<std::string>();
    result.reserve(array->values.size());
    for (auto filter : array->values) {
        // TODO is this conversion to a string really necessary?
        result.emplace_back(filter->as<Name>()->value);
    }
    return result;
}

std::string_view Stream::decode(util::Allocator &allocator) {
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

void deflate_buffer(const char *srcData, size_t srcSize, const char *&destData, size_t &destSize) {
    // TODO check that deflate_buffer actually works, deflateEnd returns a Z_DATA_ERROR
    destSize = srcSize * 2;
    destData = (char *)malloc(destSize);

    z_stream stream  = {};
    stream.zalloc    = Z_NULL;
    stream.zfree     = Z_NULL;
    stream.opaque    = Z_NULL;
    stream.avail_in  = (uInt)srcSize;     // size of input
    stream.next_in   = (Bytef *)srcData;  // input char array
    stream.avail_out = (uInt)destSize;    // size of output
    stream.next_out  = (Bytef *)destData; // output char array

    auto ret = deflateInit(&stream, Z_BEST_COMPRESSION);
    if (ret != Z_OK) {
        // TODO error handling
        return;
    }

    ret = deflate(&stream, Z_FULL_FLUSH);
    if (ret != Z_OK) {
        // TODO error handling
        return;
    }

    ret      = deflateEnd(&stream);
    destSize = stream.total_out;
    if (ret != Z_OK) {
        // TODO error handling
        return;
    }
}

void Stream::encode(util::Allocator &allocator, const std::string &data) {
    const char *encodedData = nullptr;
    size_t encodedDataSize  = 0;
    deflate_buffer(data.data(), data.size(), encodedData, encodedDataSize);

    auto newStreamData = allocator.allocate_chunk(encodedDataSize);
    memcpy(newStreamData, encodedData, encodedDataSize);
    free((void *)encodedData);

    streamData = std::string_view(newStreamData, encodedDataSize);
    // TODO update the dictionary with the correct length
}

Stream *Stream::create_from_unencoded_data(util::Allocator &allocator,
                                           const std::unordered_map<std::string, Object *> &additionalDictionaryEntries,
                                           std::string_view unencodedData) {
    const char *encodedData = nullptr;
    size_t encodedDataSize  = 0;
    deflate_buffer(unencodedData.data(), unencodedData.size(), encodedData, encodedDataSize);

    std::unordered_map<std::string, Object *> dict = {};
    for (const auto &entry : additionalDictionaryEntries) {
        dict[entry.first] = entry.second;
    }
    dict["Length"] = allocator.allocate<Integer>(encodedDataSize);
    dict["Filter"] = allocator.allocate<Name>("FlateDecode");

    // TODO this might be inefficient: maybe place the stream data after the stream object into the allocator
    auto streamData = allocator.allocate_chunk(encodedDataSize);
    memcpy(streamData, encodedData, encodedDataSize);
    free((void *)encodedData);

    auto dictionary = Dictionary::create(allocator, dict);
    return allocator.allocate<Stream>(dictionary, std::string_view(streamData, encodedDataSize));
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
    values.remove(index);
    // TODO track the deletion of the i-th element
}

void Integer::set(Document & /*document*/, int64_t i) {
    value = i;
    // TODO track the change of this value
}

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

} // namespace pdf
