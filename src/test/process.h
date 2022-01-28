#pragma once

#include <array>
#include <cstdio>
#include <fcntl.h>
#include <spdlog/spdlog.h>
#include <string>
#include <sys/fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace Process {

struct Result {
    std::string output;
    std::string error;
    int status;
};

char **prepare_argv(const std::string &command, const std::vector<std::string> &args) {
    char **argv = (char **)malloc((args.size() + 2) * sizeof(char *));
    for (size_t i = 1; i < args.size() + 1; i++) {
        char *s = const_cast<char *>(args[i - 1].c_str());
        argv[i] = s;
    }
    argv[0]               = const_cast<char *>(command.c_str());
    argv[args.size() + 1] = nullptr;
    return argv;
}

Result execute(const std::string &command, const std::vector<std::string> &args) {
    char outputBuffer;
    char errorBuffer;
    std::string outputResult;
    std::string errorResult;

    int outfd[2];
    if (pipe(outfd) == -1) {
        spdlog::error("pipe for stdout has failed");
        return {};
    }

    int errfd[2];
    if (pipe(errfd) == -1) {
        spdlog::error("pipe for stderr has failed");
        return {};
    }

    auto pid = fork();
    switch (pid) {
    case -1:
        spdlog::error("fork has failed");
        return {};
    case 0: // child
    {
        close(outfd[0]);
        dup2(outfd[1], STDOUT_FILENO);
        close(outfd[1]);

        close(errfd[0]);
        dup2(errfd[1], STDERR_FILENO);
        close(errfd[1]);

        auto argv = prepare_argv(command, args);
        if (execv(command.c_str(), argv) == -1) {
            spdlog::error("execv has failed: {}", errno);
            if (errno == ENOENT) {
                spdlog::error("No such file or directory");
            }
            if (errno == EACCES) {
                spdlog::error("Permission denied");
            }
        }
        exit(errno);
    }
    default: // parent
        break;
    }

    close(outfd[1]);
    close(errfd[1]);

    ssize_t outBytesRead;
    ssize_t errBytesRead;
    while (true) {
        outBytesRead = read(outfd[0], &outputBuffer, 1);
        if (outBytesRead == -1 && errno != EAGAIN) {
            spdlog::error("error reading stdout");
            return {};
        }
        if (outBytesRead > 0) {
            outputResult += std::string(&outputBuffer, 1);
        }

        errBytesRead = read(errfd[0], &errorBuffer, 1);
        if (errBytesRead == -1 && errno != EAGAIN) {
            spdlog::error("error reading stderr");
            return {};
        }
        if (errBytesRead > 0) {
            errorResult += std::string(&errorBuffer, 1);
        }

        if (outBytesRead == 0 && errBytesRead == 0) {
            break;
        }
    }

    int status;
    if (waitpid(pid, &status, 0) == -1) {
        spdlog::error("failed to get the child status");
        return {};
    }

    return {outputResult, errorResult, status};
}
} // namespace Process
