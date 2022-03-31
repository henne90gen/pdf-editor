#include "process.h"

#if !WIN32

#include <spdlog/spdlog.h>
#include <array>
#include <cstdio>
#include <fcntl.h>
#include <sys/fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

namespace pdf {

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

void manage_child_process(int *outfd, int *errfd, const std::string &command, const std::vector<std::string> &args) {
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

#define BUFFER_SIZE 1024

ValueResult<Execution> execute(const std::string &command, const std::vector<std::string> &args) {
    char outputBuffer[BUFFER_SIZE];
    char errorBuffer[BUFFER_SIZE];
    std::string outputResult;
    std::string errorResult;

    int outfd[2];
    if (pipe(outfd) == -1) {
        return ValueResult<Execution>::error("pipe for stdout has failed");
    }

    int errfd[2];
    if (pipe(errfd) == -1) {
        return ValueResult<Execution>::error("pipe for stderr has failed");
    }

    auto pid = fork();
    switch (pid) {
    case -1:
        return ValueResult<Execution>::error("for has failed");
    case 0: // child
        manage_child_process(outfd, errfd, command, args);
        return ValueResult<Execution>::error("this should never be reached");
    default: // parent
        break;
    }

    close(outfd[1]);
    close(errfd[1]);

    while (true) {
        auto outBytesRead = read(outfd[0], outputBuffer, BUFFER_SIZE);
        if (outBytesRead == -1 && errno != EAGAIN) {
            return ValueResult<Execution>::error("error reading stdout");
        }
        if (outBytesRead > 0) {
            outputResult += std::string(outputBuffer, outBytesRead);
        }

        auto errBytesRead = read(errfd[0], errorBuffer, BUFFER_SIZE);
        if (errBytesRead == -1 && errno != EAGAIN) {
            return ValueResult<Execution>::error("error reading stderr");
        }
        if (errBytesRead > 0) {
            errorResult += std::string(errorBuffer, errBytesRead);
        }

        if (outBytesRead == 0 && errBytesRead == 0) {
            break;
        }
    }

    int status;
    if (waitpid(pid, &status, 0) == -1) {
        return ValueResult<Execution>::error("failed to get the child status");
    }

    return ValueResult<Execution>::ok({outputResult, errorResult, status});
}

} // namespace util

#endif
