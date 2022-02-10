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

void write_object(std::ostream &s, Object *object);
void write_boolean_object(std::ostream &s, Boolean *boolean) {
    if (boolean->value) {
        s << "true";
    } else {
        s << "false";
    }
}
void write_null_object(std::ostream &s, Null *) {
    // TODO check with the spec again
    s << "null";
}
void write_integer_object(std::ostream &s, Integer *integer) { s << integer->value; }
void write_real_object(std::ostream &s, Real *real) {
    auto str = std::to_string(real->value);
    s << str;
}
void write_hexadecimal_string_object(std::ostream &s, HexadecimalString *hexadecimal) {
    s << "<" << hexadecimal->value << ">";
}
void write_literal_string_object(std::ostream &s, LiteralString *literal) { s << "(" << literal->value << ")"; }
void write_name_object(std::ostream &s, Name *name) { s << "/" << name->value; }
void write_array_object(std::ostream &s, Array *array) {
    s << "[";
    for (size_t i = 0; i < array->values.size(); i++) {
        const auto obj = array->values[i];
        write_object(s, obj);
        if (i < array->values.size() - 1) {
            s << " ";
        }
    }
    s << "]";
}
void write_dictionary_object(std::ostream &s, Dictionary *dictionary) {
    s << "<<";
    int i = 0;
    for (const auto &itr : dictionary->values) {
        if (i != 0) {
            s << " ";
        }
        s << "/" << itr.key << " ";
        write_object(s, itr.value);
        i++;
    }
    s << ">>";
}
void write_indirect_reference_object(std::ostream &s, IndirectReference *reference) {
    s << reference->objectNumber << " " << reference->generationNumber << " R";
}
void write_indirect_object(std::ostream &s, IndirectObject *object) {
    s << object->objectNumber << " " << object->generationNumber << " obj\n";
    write_object(s, object->object);
    s << "\nendobj\n\n";
}
void write_stream_object(std::ostream &s, Stream *stream) {
    write_dictionary_object(s, stream->dictionary);
    s << "\nstream\n";
    s << stream->streamData;
    s << "\nendstream\n";
}
void write_object_stream_content_object(std::ostream &, ObjectStreamContent *) { ASSERT(false); }
void write_object(std::ostream &s, Object *object) {
    switch (object->type) {
    case Object::Type::BOOLEAN:
        write_boolean_object(s, object->as<Boolean>());
        break;
    case Object::Type::INTEGER:
        write_integer_object(s, object->as<Integer>());
        break;
    case Object::Type::REAL:
        write_real_object(s, object->as<Real>());
        break;
    case Object::Type::HEXADECIMAL_STRING:
        write_hexadecimal_string_object(s, object->as<HexadecimalString>());
        break;
    case Object::Type::LITERAL_STRING:
        write_literal_string_object(s, object->as<LiteralString>());
        break;
    case Object::Type::NAME:
        write_name_object(s, object->as<Name>());
        break;
    case Object::Type::ARRAY:
        write_array_object(s, object->as<Array>());
        break;
    case Object::Type::DICTIONARY:
        write_dictionary_object(s, object->as<Dictionary>());
        break;
    case Object::Type::INDIRECT_REFERENCE:
        write_indirect_reference_object(s, object->as<IndirectReference>());
        break;
    case Object::Type::INDIRECT_OBJECT:
        write_indirect_object(s, object->as<IndirectObject>());
        break;
    case Object::Type::STREAM:
        write_stream_object(s, object->as<Stream>());
        break;
    case Object::Type::NULL_OBJECT:
        write_null_object(s, object->as<Null>());
        break;
    case Object::Type::OBJECT_STREAM_CONTENT:
        write_object_stream_content_object(s, object->as<ObjectStreamContent>());
        break;
    default:
        ASSERT(false);
    }
}

void write_objects(Document &document, std::ostream &s, std::unordered_map<uint64_t, uint64_t> &byteOffsets) {
    for (auto &entry : document.objectList) {
        if (entry.second == nullptr) {
            continue;
        }

        byteOffsets[entry.first] = s.tellp();
        write_object(s, entry.second);
    }
}

void write_trailer(Document &document, std::ostream &s, std::unordered_map<uint64_t, uint64_t> &byteOffsets) {
    auto startXref = s.tellp();
    if (document.file.trailer.dict != nullptr) {
        s << "xref ";
        s << 0 << " " << byteOffsets.size();

        s << "0000000000 65535 f \n";
        for (size_t i = 0; i < byteOffsets.size(); i++) {
            write_zero_padded_number(s, byteOffsets[i + 1], 10);
            s << " 00000 n \n";
        }

        s << "trailer\n";
        write_dictionary_object(s, document.file.trailer.dict);
    } else {
        write_indirect_object(s, document.file.trailer.streamObject);
    }

    s << "\nstartxref\n" << startXref << "\n%%EOF\n";
}

Result write_to_stream(Document &document, std::ostream &s) {
    write_header(s);

    std::unordered_map<uint64_t, uint64_t> byteOffsets = {};
    write_objects(document, s, byteOffsets);
    write_trailer(document, s, byteOffsets);

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
