#include "pdf_objects.h"

#include <cstring>
#include <zlib.h>

namespace pdf {

std::ostream &operator<<(std::ostream &os, Object::Type type) {
#define __BYTECODE_OP(op)                                                                                              \
    case Object::Type::op:                                                                                             \
        os.write(#op, strlen(#op));                                                                                    \
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
        return {itr->second->as<Name>()->value};
    }

    auto array  = itr->second->as<Array>();
    auto result = std::vector<std::string>();
    result.reserve(array->values.size());
    for (auto filter : array->values) {
        result.push_back(filter->as<Name>()->value);
    }
    return result;
}

std::string Stream::to_string() const {
    char *output      = data;
    size_t outputSize = length;

    auto fs = filters();
    for (const auto &filter : fs) {
        if (filter == "FlateDecode") {
            // TODO this works, but is far from optimal
            char *input      = output;
            size_t inputSize = outputSize;

            output     = (char *)malloc(inputSize);
            outputSize = inputSize * 2;

            z_stream infstream;
            infstream.zalloc    = Z_NULL;
            infstream.zfree     = Z_NULL;
            infstream.opaque    = Z_NULL;
            infstream.avail_in  = (uInt)inputSize;  // size of input
            infstream.next_in   = (Bytef *)input;   // input char array
            infstream.avail_out = (uInt)outputSize; // size of output
            infstream.next_out  = (Bytef *)output;  // output char array

            // the actual DE-compression work.
            inflateInit(&infstream);
            inflate(&infstream, Z_NO_FLUSH);
            inflateEnd(&infstream);
        } else {
            std::cerr << "Unknown filter: " << filter << std::endl;
        }
        // TODO handle more filters
    }

    return std::string(output, outputSize);
}

std::string HexadecimalString::to_string() const {
    // TODO this is quite hacky
    std::string tmp = value;
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

} // namespace pdf
