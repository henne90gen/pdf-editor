#include "application.h"

#include <glad/glad.h>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <pdf/document.h>
#include <spdlog/spdlog.h>

#include "gl.h"

struct ApplicationState {
    pdf::Document document = {};
    bool hasLoadedObjects  = false;

    uint32_t program;
    uint32_t VBO, VAO;
};

static ApplicationState state;

util::Result checkLinkStatus(uint32_t program) {
    GLint isCompiled = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &isCompiled);
    if (isCompiled != GL_FALSE) {
        return util::Result::ok();
    }

    GLint maxLength = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

    // The maxLength includes the NULL character
    std::string errorLog(maxLength, ' ');
    glGetProgramInfoLog(program, maxLength, &maxLength, errorLog.data());

    // Provide the infolog in whatever manor you deem best.
    // Exit with failure.
    glDeleteProgram(program); // Don't leak the shader.
    return util::Result::error("failed to link program: {}", errorLog);
}

util::Result checkCompileStatus(uint32_t shader) {
    GLint isCompiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
    if (isCompiled != GL_FALSE) {
        return util::Result::ok();
    }

    GLint maxLength = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

    // The maxLength includes the NULL character
    std::string errorLog(maxLength, ' ');
    glGetShaderInfoLog(shader, maxLength, &maxLength, errorLog.data());

    // Provide the infolog in whatever manor you deem best.
    // Exit with failure.
    glDeleteShader(shader); // Don't leak the shader.
    return util::Result::error("failed to compile vertex shader: {}", errorLog);
}

#define TRY_VALUE(t, x)                                                                                                \
    {                                                                                                                  \
        auto status = x;                                                                                               \
        if (status.has_error()) {                                                                                      \
            return util::ValueResult<t>::of(status);                                                                   \
        }                                                                                                              \
    }

util::ValueResult<uint32_t> compileProgram(const std::string &vertexShaderCode, const std::string &fragmentShaderCode) {
    GL_Call(auto vertexShader = glCreateShader(GL_VERTEX_SHADER));
    auto vertexShaderCodePtr = const_cast<char *>(vertexShaderCode.c_str());
    GL_Call(glShaderSource(vertexShader, 1, &vertexShaderCodePtr, nullptr));
    GL_Call(glCompileShader(vertexShader));

    TRY_VALUE(uint32_t, checkCompileStatus(vertexShader))

    GL_Call(auto fragmentShader = glCreateShader(GL_FRAGMENT_SHADER));
    auto fragmentShaderCodePtr = const_cast<char *>(fragmentShaderCode.c_str());
    GL_Call(glShaderSource(fragmentShader, 1, &fragmentShaderCodePtr, nullptr));
    GL_Call(glCompileShader(fragmentShader));

    TRY_VALUE(uint32_t, checkCompileStatus(fragmentShader))

    GL_Call(auto program = glCreateProgram());
    GL_Call(glAttachShader(program, vertexShader));
    GL_Call(glAttachShader(program, fragmentShader));

    GL_Call(glLinkProgram(program));

    TRY_VALUE(uint32_t, checkLinkStatus(program));

    return util::ValueResult<uint32_t>::ok(program);
}

util::Result application_init(int argc, char **argv) {
    spdlog::info("Arguments:");
    for (int i = 0; i < argc; i++) {
        spdlog::info("  {}", argv[i]);
    }

    if (argc != 2) {
        return util::Result::error("Wrong number of arguments");
    }

    const std::string filePath = std::string(argv[1]);
    auto result                = pdf::Document::read_from_file(filePath, state.document, false);
    if (result.has_error()) {
        return result;
    }

    spdlog::info("Loaded '{}'", filePath);

    float vertices[] = {
          // positions         // colors
          0.5f,  -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom right
          -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, // bottom left
          0.0f,  0.5f,  0.0f, 0.0f, 0.0f, 1.0f  // top
    };
    GL_Call(glGenVertexArrays(1, &state.VAO));
    GL_Call(glGenBuffers(1, &state.VBO));
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    GL_Call(glBindVertexArray(state.VAO));

    GL_Call(glBindBuffer(GL_ARRAY_BUFFER, state.VBO));
    GL_Call(glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW));

    // position attribute
    GL_Call(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0));
    GL_Call(glEnableVertexAttribArray(0));
    // color attribute
    GL_Call(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float))));
    GL_Call(glEnableVertexAttribArray(1));

    auto vertexShaderSource =
          "#version 330 core\n"
          "layout (location = 0) in vec3 aPos;   // the position variable has attribute position 0\n"
          "layout (location = 1) in vec3 aColor; // the color variable has attribute position 1\n"
          "  \n"
          "out vec3 ourColor; // output a color to the fragment shader\n"
          "\n"
          "void main()\n"
          "{\n"
          "    gl_Position = vec4(aPos, 1.0);\n"
          "    ourColor = aColor; // set ourColor to the input color we got from the vertex data\n"
          "} ";
    auto fragmentShaderSource = "#version 330 core\n"
                                "out vec4 FragColor;  \n"
                                "in vec3 ourColor;\n"
                                "  \n"
                                "void main()\n"
                                "{\n"
                                "    FragColor = vec4(ourColor, 1.0);\n"
                                "}";
    auto compileResult        = compileProgram(vertexShaderSource, fragmentShaderSource);
    if (compileResult.has_error()) {
        return compileResult.drop_value();
    }

    state.program = compileResult.value();
    spdlog::info("setting program compile result:{}", state.program);

    return util::Result::ok();
}

util::Result application_run() {
    ImGui::Begin("Info");
    if (ImGui::Button("Load Objects")) {
        state.document.objects();
    }
    ImGui::Text("Object count: %ld", state.document.object_count(false));
    ImGui::End();

    GL_Call(glUseProgram(state.program));

    // update the uniform color
    float timeValue  = glfwGetTime();
    float greenValue = sin(timeValue) / 2.0f + 0.5f;
    GL_Call(auto vertexColorLocation = glGetUniformLocation(state.program, "ourColor"));
    GL_Call(glUniform3f(vertexColorLocation, 0.0f, greenValue, 0.0f));

    // now render the triangle
    GL_Call(glBindVertexArray(state.VAO));
    GL_Call(glDrawArrays(GL_TRIANGLES, 0, 3));

    return util::Result::ok();
}
