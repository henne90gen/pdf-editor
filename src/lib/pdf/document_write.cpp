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

size_t count_digits(int64_t number) {
    size_t result = 0;
    while (number > 0) {
        number /= 10;
        result++;
    }
    return result;
}

void write_header(std::ostream &s) { s << "%PDF-1.6\n%äüöß\n"; }

size_t write_object(std::ostream &s, Object *object);
size_t write_boolean_object(std::ostream &s, Boolean *boolean) {
    if (boolean->value) {
        s << "true";
        return 4;
    } else {
        s << "false";
        return 5;
    }
}
size_t write_null_object(std::ostream &s, Null *) {
    // TODO check with the spec again
    s << "null";
    return 4;
}
size_t write_integer_object(std::ostream &s, Integer *integer) {
    s << integer->value;
    return count_digits(integer->value);
}
size_t write_real_object(std::ostream &s, Real *real) {
    auto str = std::to_string(real->value);
    s << str;
    return str.size();
}
size_t write_hexadecimal_string_object(std::ostream &s, HexadecimalString *hexadecimal) {
    s << "<" << hexadecimal->value << ">";
    return 2 + hexadecimal->value.length();
}
size_t write_literal_string_object(std::ostream &s, LiteralString *literal) {
    s << "(" << literal->value << ")";
    return 2 + literal->value.length();
}
size_t write_name_object(std::ostream &s, Name *name) {
    s << "/" << name->value;
    return 1 + name->value.size();
}
size_t write_array_object(std::ostream &s, Array *array) {
    s << "[";
    size_t bytesWritten = 0;
    for (size_t i = 0; i < array->values.size(); i++) {
        const auto obj = array->values[i];
        bytesWritten += write_object(s, obj);
        if (i < array->values.size() - 1) {
            bytesWritten++;
            s << " ";
        }
    }
    s << "]";
    return bytesWritten + 2;
}
size_t write_dictionary_object(std::ostream &s, Dictionary *dictionary) {
    s << "<<";
    size_t bytesWritten = 0;
    for (const auto &itr : dictionary->values) {
        if (bytesWritten != 0) {
            s << " ";
            bytesWritten++;
        }
        s << "/" << itr.key << " ";
        bytesWritten += 2 + itr.key.length();
        bytesWritten += write_object(s, itr.value);
    }
    s << ">>";
    return bytesWritten + 4;
}
size_t write_indirect_reference_object(std::ostream &s, IndirectReference *reference) {
    s << reference->objectNumber << " " << reference->generationNumber << " R";
    return 3 + count_digits(reference->objectNumber) + count_digits(reference->generationNumber);
}
size_t write_indirect_object(std::ostream &s, IndirectObject *object) {
    s << object->objectNumber << " " << object->generationNumber << " obj\n";
    size_t bytesWritten = 6 + count_digits(object->objectNumber) + count_digits(object->generationNumber);
    bytesWritten += write_object(s, object->object);
    s << "\nendobj\n\n";
    bytesWritten += 9;
    return bytesWritten;
}
size_t write_stream_object(std::ostream &s, Stream *stream) {
    auto bytesWritten = write_dictionary_object(s, stream->dictionary);
    s << "\nstream\n";
    s << stream->streamData;
    s << "\nendstream\n";
    bytesWritten += stream->streamData.size();
    bytesWritten += 19;
    return bytesWritten;
}
size_t write_object_stream_content_object(std::ostream &, ObjectStreamContent *) {
    ASSERT(false);
    return 0;
}
size_t write_object(std::ostream &s, Object *object) {
    switch (object->type) {
    case Object::Type::BOOLEAN:
        return write_boolean_object(s, object->as<Boolean>());
    case Object::Type::INTEGER:
        return write_integer_object(s, object->as<Integer>());
    case Object::Type::REAL:
        return write_real_object(s, object->as<Real>());
    case Object::Type::HEXADECIMAL_STRING:
        return write_hexadecimal_string_object(s, object->as<HexadecimalString>());
    case Object::Type::LITERAL_STRING:
        return write_literal_string_object(s, object->as<LiteralString>());
    case Object::Type::NAME:
        return write_name_object(s, object->as<Name>());
    case Object::Type::ARRAY:
        return write_array_object(s, object->as<Array>());
    case Object::Type::DICTIONARY:
        return write_dictionary_object(s, object->as<Dictionary>());
    case Object::Type::INDIRECT_REFERENCE:
        return write_indirect_reference_object(s, object->as<IndirectReference>());
    case Object::Type::INDIRECT_OBJECT:
        return write_indirect_object(s, object->as<IndirectObject>());
    case Object::Type::STREAM:
        return write_stream_object(s, object->as<Stream>());
    case Object::Type::NULL_OBJECT:
        return write_null_object(s, object->as<Null>());
    case Object::Type::OBJECT_STREAM_CONTENT:
        return write_object_stream_content_object(s, object->as<ObjectStreamContent>());
    default:
        ASSERT(false);
    }
}

size_t write_objects(Document &document, std::ostream &s, std::unordered_map<uint64_t, uint64_t> &byteOffsets) {
    size_t currentOffset = 19;
    for (auto &entry : document.objectList) {
        if (entry.second == nullptr) {
            continue;
        }

#if 1
        byteOffsets[entry.first] = currentOffset;
        currentOffset += write_object(s, entry.second);
#else
        s << entry.second->data;
        byteOffsets[entry.first] = currentOffset;
        currentOffset += entry.second->data.size();
#endif
    }
    return currentOffset;
}

void write_trailer(Document &document, std::ostream &s, std::unordered_map<uint64_t, uint64_t> &byteOffsets,
                   size_t startXref) {
    s << "xref ";
    s << 0 << " " << byteOffsets.size();

    s << "0000000000 65535 f \n";
    for (size_t i = 0; i < byteOffsets.size(); i++) {
        write_zero_padded_number(s, byteOffsets[i + 1], 10);
        s << " 00000 n \n";
    }

    s << "trailer\n";
    write_dictionary_object(s, document.file.trailer.dict);

    s << "\nstartxref\n" << startXref << "\n%%EOF\n";
}

Result write_to_stream(Document &document, std::ostream &s) {
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
