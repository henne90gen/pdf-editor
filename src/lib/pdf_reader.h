#pragma once

#include <string>

#include "pdf_file.h"

namespace pdf {

bool load_from_file(const std::string &filePath, File &file);

}
