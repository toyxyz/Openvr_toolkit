#include "glad/glad.h"

PFNGLGENBUFFERSPROC glad_glGenBuffers = nullptr;
PFNGLBINDBUFFERPROC glad_glBindBuffer = nullptr;
PFNGLBUFFERDATAPROC glad_glBufferData = nullptr;
PFNGLDELETEBUFFERSPROC glad_glDeleteBuffers = nullptr;
PFNGLCREATESHADERPROC glad_glCreateShader = nullptr;
PFNGLSHADERSOURCEPROC glad_glShaderSource = nullptr;
PFNGLCOMPILESHADERPROC glad_glCompileShader = nullptr;
PFNGLGETSHADERIVPROC glad_glGetShaderiv = nullptr;
PFNGLGETSHADERINFOLOGPROC glad_glGetShaderInfoLog = nullptr;
PFNGLDELETESHADERPROC glad_glDeleteShader = nullptr;
PFNGLCREATEPROGRAMPROC glad_glCreateProgram = nullptr;
PFNGLATTACHSHADERPROC glad_glAttachShader = nullptr;
PFNGLLINKPROGRAMPROC glad_glLinkProgram = nullptr;
PFNGLGETPROGRAMIVPROC glad_glGetProgramiv = nullptr;
PFNGLGETPROGRAMINFOLOGPROC glad_glGetProgramInfoLog = nullptr;
PFNGLDELETEPROGRAMPROC glad_glDeleteProgram = nullptr;
PFNGLUSEPROGRAMPROC glad_glUseProgram = nullptr;
PFNGLGETUNIFORMLOCATIONPROC glad_glGetUniformLocation = nullptr;
PFNGLGETATTRIBLOCATIONPROC glad_glGetAttribLocation = nullptr;
PFNGLUNIFORM1IPROC glad_glUniform1i = nullptr;
PFNGLUNIFORM4FPROC glad_glUniform4f = nullptr;
PFNGLUNIFORMMATRIX3FVPROC glad_glUniformMatrix3fv = nullptr;
PFNGLUNIFORMMATRIX4FVPROC glad_glUniformMatrix4fv = nullptr;
PFNGLACTIVETEXTUREPROC glad_glActiveTexture = nullptr;
PFNGLENABLEVERTEXATTRIBARRAYPROC glad_glEnableVertexAttribArray = nullptr;
PFNGLDISABLEVERTEXATTRIBARRAYPROC glad_glDisableVertexAttribArray = nullptr;
PFNGLVERTEXATTRIBPOINTERPROC glad_glVertexAttribPointer = nullptr;

namespace {

void* loadWglProc(const char* name)
{
    void* proc = reinterpret_cast<void*>(wglGetProcAddress(name));
    if (proc != nullptr && proc != reinterpret_cast<void*>(1) &&
        proc != reinterpret_cast<void*>(2) && proc != reinterpret_cast<void*>(3) &&
        proc != reinterpret_cast<void*>(-1)) {
        return proc;
    }

    HMODULE module = GetModuleHandleA("opengl32.dll");
    if (module == nullptr) {
        module = LoadLibraryA("opengl32.dll");
    }
    return module == nullptr ? nullptr : reinterpret_cast<void*>(GetProcAddress(module, name));
}

template <typename T>
bool loadOne(GLADloadproc load, T& target, const char* name)
{
    target = reinterpret_cast<T>(load(name));
    return target != nullptr;
}

} // namespace

int gladLoadGLLoader(GLADloadproc load)
{
    if (load == nullptr) {
        return 0;
    }

    bool ok = true;
    ok = loadOne(load, glad_glGenBuffers, "glGenBuffers") && ok;
    ok = loadOne(load, glad_glBindBuffer, "glBindBuffer") && ok;
    ok = loadOne(load, glad_glBufferData, "glBufferData") && ok;
    ok = loadOne(load, glad_glDeleteBuffers, "glDeleteBuffers") && ok;
    ok = loadOne(load, glad_glCreateShader, "glCreateShader") && ok;
    ok = loadOne(load, glad_glShaderSource, "glShaderSource") && ok;
    ok = loadOne(load, glad_glCompileShader, "glCompileShader") && ok;
    ok = loadOne(load, glad_glGetShaderiv, "glGetShaderiv") && ok;
    ok = loadOne(load, glad_glGetShaderInfoLog, "glGetShaderInfoLog") && ok;
    ok = loadOne(load, glad_glDeleteShader, "glDeleteShader") && ok;
    ok = loadOne(load, glad_glCreateProgram, "glCreateProgram") && ok;
    ok = loadOne(load, glad_glAttachShader, "glAttachShader") && ok;
    ok = loadOne(load, glad_glLinkProgram, "glLinkProgram") && ok;
    ok = loadOne(load, glad_glGetProgramiv, "glGetProgramiv") && ok;
    ok = loadOne(load, glad_glGetProgramInfoLog, "glGetProgramInfoLog") && ok;
    ok = loadOne(load, glad_glDeleteProgram, "glDeleteProgram") && ok;
    ok = loadOne(load, glad_glUseProgram, "glUseProgram") && ok;
    ok = loadOne(load, glad_glGetUniformLocation, "glGetUniformLocation") && ok;
    ok = loadOne(load, glad_glGetAttribLocation, "glGetAttribLocation") && ok;
    ok = loadOne(load, glad_glUniform1i, "glUniform1i") && ok;
    ok = loadOne(load, glad_glUniform4f, "glUniform4f") && ok;
    ok = loadOne(load, glad_glUniformMatrix3fv, "glUniformMatrix3fv") && ok;
    ok = loadOne(load, glad_glUniformMatrix4fv, "glUniformMatrix4fv") && ok;
    ok = loadOne(load, glad_glActiveTexture, "glActiveTexture") && ok;
    ok = loadOne(load, glad_glEnableVertexAttribArray, "glEnableVertexAttribArray") && ok;
    ok = loadOne(load, glad_glDisableVertexAttribArray, "glDisableVertexAttribArray") && ok;
    ok = loadOne(load, glad_glVertexAttribPointer, "glVertexAttribPointer") && ok;
    return ok ? 1 : 0;
}

int gladLoadGL(void)
{
    return gladLoadGLLoader(loadWglProc);
}
