#pragma once

#define GL_Call(x)                                                                                                     \
    GL_ClearError();                                                                                                   \
    x;                                                                                                                 \
    ASSERT(GL_LogCall(#x, __FILE__, __LINE__))
#define GL_CheckErrors() ASSERT(GL_LogCall("", __FILE__, __LINE__))

void GL_ClearError();
bool GL_LogCall(const char *function, const char *file, int line);
