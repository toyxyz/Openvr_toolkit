#include "platform/win32/ViewportMatcapShader.h"

#include "platform/win32/AppViewportState.h"

#include <string>
#include <vector>

namespace ovtr::win32 {
namespace {

std::string shaderInfoLog(const GLuint shader)
{
    GLint length = 0;
    glad_glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    if (length <= 1) {
        return "no shader info log";
    }

    std::vector<char> log(static_cast<std::size_t>(length), '\0');
    GLsizei written = 0;
    glad_glGetShaderInfoLog(shader, length, &written, log.data());
    return std::string(log.data(), static_cast<std::size_t>(written));
}

std::string programInfoLog(const GLuint program)
{
    GLint length = 0;
    glad_glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
    if (length <= 1) {
        return "no program info log";
    }

    std::vector<char> log(static_cast<std::size_t>(length), '\0');
    GLsizei written = 0;
    glad_glGetProgramInfoLog(program, length, &written, log.data());
    return std::string(log.data(), static_cast<std::size_t>(written));
}

UniqueGlShader compileShader(const GLenum type, const char* source, std::string& error)
{
    UniqueGlShader shader(glad_glCreateShader(type));
    if (!shader) {
        error = "glCreateShader failed";
        return {};
    }

    glad_glShaderSource(shader.get(), 1, &source, nullptr);
    glad_glCompileShader(shader.get());

    GLint ok = 0;
    glad_glGetShaderiv(shader.get(), GL_COMPILE_STATUS, &ok);
    if (ok == 0) {
        error = shaderInfoLog(shader.get());
        return {};
    }

    return shader;
}

bool queryShaderLocations(ViewportMatcapShaderState& shader)
{
    shader.positionAttrib = glad_glGetAttribLocation(shader.program.get(), "aPosition");
    shader.normalAttrib = glad_glGetAttribLocation(shader.program.get(), "aNormal");
    shader.mvpUniform = glad_glGetUniformLocation(shader.program.get(), "uModelViewProjection");
    shader.normalMatrixUniform = glad_glGetUniformLocation(shader.program.get(), "uNormalMatrix");
    shader.tintUniform = glad_glGetUniformLocation(shader.program.get(), "uTintColor");
    shader.matcapUniform = glad_glGetUniformLocation(shader.program.get(), "uMatcap");
    return shader.positionAttrib >= 0 && shader.normalAttrib >= 0 &&
        shader.mvpUniform >= 0 && shader.normalMatrixUniform >= 0 &&
        shader.tintUniform >= 0 && shader.matcapUniform >= 0;
}

} // namespace

const char* viewportMatcapVertexShaderSource() noexcept
{
    return
        "#version 120\n"
        "attribute vec3 aPosition;\n"
        "attribute vec3 aNormal;\n"
        "uniform mat4 uModelViewProjection;\n"
        "uniform mat3 uNormalMatrix;\n"
        "varying vec3 vNormal;\n"
        "void main() {\n"
        "  vNormal = normalize(uNormalMatrix * aNormal);\n"
        "  gl_Position = uModelViewProjection * vec4(aPosition, 1.0);\n"
        "}\n";
}

const char* viewportMatcapFragmentShaderSource() noexcept
{
    return
        "#version 120\n"
        "uniform sampler2D uMatcap;\n"
        "uniform vec4 uTintColor;\n"
        "varying vec3 vNormal;\n"
        "void main() {\n"
        "  vec3 n = normalize(vNormal);\n"
        "  vec2 uv = n.xy * 0.5 + 0.5;\n"
        "  gl_FragColor = texture2D(uMatcap, uv) * uTintColor;\n"
        "}\n";
}

bool ensureMatcapShader(AppViewportState& state) noexcept
{
    ViewportMatcapShaderState& shader = state.matcapShader;
    if (shader.program) {
        return true;
    }
    if (shader.buildFailed || !state.gpuCapabilities.shaderVboAvailable) {
        return false;
    }

    std::string error;
    UniqueGlShader vertex = compileShader(GL_VERTEX_SHADER, viewportMatcapVertexShaderSource(), error);
    if (!vertex) {
        shader.buildFailed = true;
        shader.failureReason = "matcap vertex shader compile failed: " + error;
        return false;
    }

    UniqueGlShader fragment = compileShader(GL_FRAGMENT_SHADER, viewportMatcapFragmentShaderSource(), error);
    if (!fragment) {
        shader.buildFailed = true;
        shader.failureReason = "matcap fragment shader compile failed: " + error;
        return false;
    }

    UniqueGlProgram program(glad_glCreateProgram());
    if (!program) {
        shader.buildFailed = true;
        shader.failureReason = "glCreateProgram failed";
        return false;
    }

    glad_glAttachShader(program.get(), vertex.get());
    glad_glAttachShader(program.get(), fragment.get());
    glad_glLinkProgram(program.get());

    GLint ok = 0;
    glad_glGetProgramiv(program.get(), GL_LINK_STATUS, &ok);
    if (ok == 0) {
        shader.buildFailed = true;
        shader.failureReason = "matcap shader link failed: " + programInfoLog(program.get());
        return false;
    }

    shader.program.reset(program.release());
    if (!queryShaderLocations(shader)) {
        shader.buildFailed = true;
        shader.failureReason = "matcap shader is missing an attribute or uniform";
        shader.program.reset();
        return false;
    }

    return true;
}

void resetMatcapShader(ViewportMatcapShaderState& shader) noexcept
{
    shader.program.reset();
    shader.positionAttrib = -1;
    shader.normalAttrib = -1;
    shader.mvpUniform = -1;
    shader.normalMatrixUniform = -1;
    shader.tintUniform = -1;
    shader.matcapUniform = -1;
    shader.buildFailed = false;
    shader.failureReason.clear();
}

} // namespace ovtr::win32
