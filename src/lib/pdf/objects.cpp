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

std::vector<std::string> Stream::filters() const {
    auto itr = dictionary->values.find("Filter");
    if (itr == dictionary->values.end()) {
        return {};
    }

    if (itr->second->is<Name>()) {
        // TODO is this conversion to a string really necessary?
        return {std::string(itr->second->as<Name>()->value())};
    }

    auto array  = itr->second->as<Array>();
    auto result = std::vector<std::string>();
    result.reserve(array->values.size());
    for (auto filter : array->values) {
        // TODO is this conversion to a string really necessary?
        result.emplace_back(filter->as<Name>()->value());
    }
    return result;
}

std::string_view Stream::to_string() const {
    const char *output = stream_data.data();
    size_t outputSize  = stream_data.length();

    auto fs = filters();
    if (fs.empty()) {
        return stream_data;
    }

    for (const auto &filter : fs) {
        if (filter == "FlateDecode") {
            // TODO this works, but is not optimal
            const char *input = output;
            size_t inputSize  = outputSize;

            outputSize = inputSize * 3;
            output     = (char *)malloc(outputSize);

            z_stream infstream;
            infstream.zalloc    = Z_NULL;
            infstream.zfree     = Z_NULL;
            infstream.opaque    = Z_NULL;
            infstream.avail_in  = (uInt)inputSize;  // size of input
            infstream.next_in   = (Bytef *)input;   // input char array
            infstream.avail_out = (uInt)outputSize; // size of output
            infstream.next_out  = (Bytef *)output;  // output char array

            inflateInit(&infstream);
            inflate(&infstream, Z_NO_FLUSH);
            inflateEnd(&infstream);

            outputSize = infstream.total_out;

            // copy the result into a buffer that fits exactly
            char *tmp = (char *)malloc(outputSize);
            memcpy(tmp, output, outputSize);
            free((void *)output);
            output = tmp;
        } else {
            spdlog::error("Unknown filter: {}", filter);
            ASSERT(false);
        }
        // TODO handle more filters
    }

    return {output, outputSize};
}

std::string HexadecimalString::to_string() const {
    // TODO this is quite hacky
    // TODO is this conversion to a string really necessary?
    std::string tmp = std::string(data);
    if (tmp.size() % 2 == 1) {
        tmp += "0";
    }

    char *buf  = (char *)malloc(4);
    char *low  = buf;
    low[1]     = '\0';
    char *high = buf + 2;
    high[1]    = '\0';

    std::string result;
    for (int i = 0; i < tmp.size(); i += 2) {
        *high         = tmp[i];
        *low          = tmp[i + 1];
        auto highVal  = std::strtol(high, nullptr, 16);
        auto lowVal   = std::strtol(low, nullptr, 16);
        char nextChar = (char)(lowVal + 16 * highVal);
        result += nextChar;
    }

    free(buf);
    return result;
}

void Array::remove_element(Document &document, size_t index) {
    ASSERT(index < values.size());
    document.deleted_sections.push_back(values[index]->data);
    values.erase(values.begin() + static_cast<int64_t>(index));
}

} // namespace pdf
