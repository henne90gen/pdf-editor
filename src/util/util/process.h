#pragma once

#include <string>
#include <vector>

namespace util {

struct Result {
    std::string output;
    std::string error;
    int status;
};

Result execute(const std::string &command, const std::vector<std::string> &args);

} // namespace util
