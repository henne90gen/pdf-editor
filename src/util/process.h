#pragma once

#include <string>
#include <vector>

namespace process {

struct Result {
    std::string output;
    std::string error;
    int status;
};

Result execute(const std::string &command, const std::vector<std::string> &args);

} // namespace process
