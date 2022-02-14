#if _WIN32

#include "process.h"

#include <spdlog/spdlog.h>
#include <windows.h>

namespace util {

#define BUFFER_SIZE 1024

std::string readPipeToString(HANDLE pipeHandle) {
    std::string result;
    DWORD bytesRead;
    char buf[BUFFER_SIZE];

    while (true) {
        auto success = ReadFile(pipeHandle, buf, BUFFER_SIZE, &bytesRead, nullptr);
        if (!success || bytesRead == 0) {
            break;
        }

        result += std::string_view(buf, bytesRead);
    }

    return result;
}

void logError(DWORD error) {
    LPSTR messageBuffer = nullptr;
    size_t size =
          FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                         nullptr, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, nullptr);

    std::string message(messageBuffer, size);
    LocalFree(messageBuffer);
    spdlog::error(message);
}

Result execute(const std::string &command, const std::vector<std::string> &args) {
    LPCTSTR lpApplicationName = command.c_str();

    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength              = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle       = TRUE;
    saAttr.lpSecurityDescriptor = nullptr;

    HANDLE stdoutRead  = nullptr;
    HANDLE stdoutWrite = nullptr;
    if (!CreatePipe(&stdoutRead, &stdoutWrite, &saAttr, 0)) {
        spdlog::error("Failed to create pipe for stdout");
        return {};
    }
    if (!SetHandleInformation(stdoutRead, HANDLE_FLAG_INHERIT, 0)) {
        spdlog::error("Failed to configure pipe for stdout");
        return {};
    }

    HANDLE stderrRead  = nullptr;
    HANDLE stderrWrite = nullptr;
    if (!CreatePipe(&stderrRead, &stderrWrite, &saAttr, 0)) {
        spdlog::error("Failed to create pipe for stderr");
        return {};
    }
    if (!SetHandleInformation(stderrRead, HANDLE_FLAG_INHERIT, 0)) {
        spdlog::error("Failed to configure pipe for stderr");
        return {};
    }

    // additional information
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    // set the size of the structures
    ZeroMemory(&si, sizeof(si));
    si.cb         = sizeof(si);
    si.hStdOutput = stdoutWrite;
    si.hStdError  = stderrWrite;
    si.dwFlags |= STARTF_USESTDHANDLES;
    ZeroMemory(&pi, sizeof(pi));

    std::string arguments;
    for (auto &arg : args) {
        arguments += " " + arg;
    }

    char *lpCommandLine = const_cast<char *>(arguments.c_str());
    auto success        = CreateProcess(lpApplicationName, // the path
                                        lpCommandLine,     // Command line
                                        nullptr,           // Process handle not inheritable
                                        nullptr,           // Thread handle not inheritable
                                        TRUE,              // Set handle inheritance to FALSE
                                        0,                 // No creation flags
                                        nullptr,           // Use parent's environment block
                                        nullptr,           // Use parent's starting directory
                                        &si,               // Pointer to STARTUPINFO structure
                                        &pi // Pointer to PROCESS_INFORMATION structure (removed extra parentheses)
           );
    if (!success) {
        auto error = GetLastError();
        spdlog::error("Failed to start child process");
        logError(error);
        return {};
    }

    // Close handles to the ends of the pipes that have been taken over by the child process
    CloseHandle(stdoutWrite);
    CloseHandle(stderrWrite);

    // Wait until child process exits.
    WaitForSingleObject(pi.hProcess, INFINITE);

    Result result = {};
    result.output = readPipeToString(stdoutRead);
    result.error  = readPipeToString(stderrRead);

    DWORD exitCode;
    success = GetExitCodeProcess(pi.hProcess, &exitCode);
    if (!success) {
        spdlog::error("Failed to get exit code of child process");
        return {};
    }
    result.status = static_cast<int>(exitCode);
    // Close handles
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return result;
}

} // namespace process

#endif
