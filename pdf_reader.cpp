#include "pdf_reader.h"

#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

// TODO create real assertion macro
#define ASSERT(x)

struct Line {
  char *start = nullptr;
  int32_t length = 0;
  Line() = default;
  Line(char *_start, int32_t _length) : start(_start), length(_length) {}
  std::string to_string() const { return std::string(start, length); }
};

struct Trailer {
  Line lastCrossRefStart = {};
  std::unordered_map<std::string, std::string> dict = {};
};

enum class TokenType {
  BOOLEAN = 0,
  INTEGER = 1,
  REAL = 2,
  LITERAL_STRING = 3,
  HEXADECIMAL_STRING = 4,
  NAME = 5,
  ARRAY_START = 6,
  ARRAY_END = 7,
  DICTIONARY_START = 8,
  DICTIONARY_END = 9,
};

struct Token {
  TokenType token;
  std::string content;
};

struct Lexer {
  Token getToken() {
std::string currentWord = "";
    auto floatToken = matchRegex("^[0-9]+\\.[0-9]+", TokenType::REAL);
    if (floatToken.has_value()) {
      currentWord = currentWord.substr(floatToken.value().content.length(), currentWord.length() - 1);
      return floatToken.value();
    }

    auto intToken = matchRegex("^[0-9]+", TokenType::INTEGER);
    if (intToken.has_value()) {
      currentWord = currentWord.substr(intToken.value().content.length(), currentWord.length() - 1);
      return intToken.value();
    }

  }
};

std::unordered_map<std::string, std::string> parseDict(std::vector<Line> lines,
                                                       int start, int end) {
  ASSERT(start < end);
  std::unordered_map<std::string, std::string> result = {};
  for (int i = start; i < end; i++) {
  }
  return result;
}

void pdf_reader::read(const std::string &filePath) {
  auto is = std::ifstream();
  is.open(filePath, std::ios::in | std::ifstream::ate | std::ios::binary);

  if (!is.is_open()) {
    std::cerr << "Failed to open pdf file." << std::endl;
    return;
  }

  auto fileSize = is.tellg();
  char *buf = (char *)malloc(fileSize);

  is.seekg(0);
  is.read(buf, fileSize);

  std::vector<Line> lines = {};
  char *lastLineStart = buf;
  for (int i = 0; i < fileSize; i++) {
    if (buf[i] == '\n') {
      unsigned long lineLength = i - (lastLineStart - buf);
      lines.emplace_back(lastLineStart, lineLength);
      lastLineStart = buf + i + 1;
    }
  }

  if (lines.back().to_string() != "%%EOF") {
    std::cerr << "Last line did not have '%%EOF'" << std::endl;
    return;
  }

  auto startxref = lines[lines.size() - 3];
  if (startxref.to_string() != "startxref") {
    std::cerr << "Expected startxref" << std::endl;
    return;
  }

  Trailer trailer = {};
  trailer.lastCrossRefStart = lines[lines.size() - 2];

  int startOfTrailer = -1;
  for (int i = lines.size() - 1; i >= 0; i--) {
    if (lines[i].to_string() == "trailer") {
      startOfTrailer = i;
      break;
    }
  }

  if (startOfTrailer == -1) {
    std::cerr << "Failed to find start of trailer" << std::endl;
    return;
  }

  int endOfTrailer = lines.size() - 4;
  trailer.dict = parseDict(lines, startOfTrailer, endOfTrailer);

  for (auto line : lines) {
    std::cout << line.to_string() << std::endl;
  }

  std::cout << std::endl << "Success" << std::endl;
}
