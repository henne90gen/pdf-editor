#pragma once

#include <string>
#include <vector>

#include "result.h"

namespace util {

struct Execution {
    std::string output;
    std::string error;
    int status;
};

ValueResult<Execution> execute(const std::string &command, const std::vector<std::string> &args);

} // namespace util
