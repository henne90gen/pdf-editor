#pragma once

#include <string>
#include <vector>

#include <pdf/util/result.h>

// TODO move this closer to the tests?

namespace pdf {

struct Execution {
    std::string output;
    std::string error;
    int status;
};

ValueResult<Execution> execute(const std::string &command, const std::vector<std::string> &args);

} // namespace util
