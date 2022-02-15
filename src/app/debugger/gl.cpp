#include "gl.h"

#include <glad/glad.h>
#include <spdlog/spdlog.h>

void GL_ClearError() {
    while (glGetError() != GL_NO_ERROR) {}
}

bool GL_LogCall(const char *function, const char *file, int line) {
    bool noErrors = true;
    while (GLenum error = glGetError() != GL_NO_ERROR) {
        spdlog::error("OpenGL error [0x{}]: {}/{}: ", error, file, function, line);
        noErrors = false;
    }
    return noErrors;
}
