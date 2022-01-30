#pragma once

#include "spdlog/spdlog.h"
#include <array>
#include <cstdio>
#include <fcntl.h>
#include <string>
#include <sys/fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace process {

struct Result {
    std::string output;
    std::string error;
    int status;
};

Result execute(const std::string &command, const std::vector<std::string> &args);

} // namespace process
