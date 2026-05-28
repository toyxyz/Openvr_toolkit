#include "platform/win32/ViewportGpuMatcapDraw.h"

#include "platform/win32/AppViewportState.h"
#include "platform/win32/ViewportGlTextureBindingScope.h"
#include "platform/win32/ViewportMatcapShader.h"

#include <cmath>
#include <cstddef>

namespace ovtr::win32 {
namespace {

struct ScopedGlProgram {
    ScopedGlProgram() noexcept
    {
        glGetIntegerv(GL_CURRENT_PROGRAM, &previous_);
    }

    ~ScopedGlProgram()
    {
        glad_glUseProgram(static_cast<GLuint>(previous_));
    }

    GLint previous_ = 0;
};

void multiplyColumnMajor4(const float* a, const float* b, float* out) noexcept
{
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            out[col * 4 + row] =
                a[0 * 4 + row] * b[col * 4 + 0] +
                a[1 * 4 + row] * b[col * 4 + 1] +
                a[2 * 4 + row] * b[col * 4 + 2] +
                a[3 * 4 + row] * b[col * 4 + 3];
        }
    }
}

void currentMvpMatrix(float* out) noexcept
{
    float projection[16]{};
    float modelView[16]{};
    glGetFloatv(GL_PROJECTION_MATRIX, projection);
    glGetFloatv(GL_MODELVIEW_MATRIX, modelView);
    multiplyColumnMajor4(projection, modelView, out);
}

void identityNormalMatrix(float* out) noexcept
{
    out[0] = 1.0f;
    out[1] = 0.0f;
    out[2] = 0.0f;
    out[3] = 0.0f;
    out[4] = 1.0f;
    out[5] = 0.0f;
    out[6] = 0.0f;
    out[7] = 0.0f;
    out[8] = 1.0f;
}

void currentNormalMatrix(float* out) noexcept
{
    float m[16]{};
    glGetFloatv(GL_MODELVIEW_MATRIX, m);
    const float a00 = m[0], a01 = m[4], a02 = m[8];
    const float a10 = m[1], a11 = m[5], a12 = m[9];
    const float a20 = m[2], a21 = m[6], a22 = m[10];
    const float det =
        a00 * (a11 * a22 - a12 * a21) -
        a01 * (a10 * a22 - a12 * a20) +
        a02 * (a10 * a21 - a11 * a20);
    if (std::fabs(det) < 0.000001f) {
        identityNormalMatrix(out);
        return;
    }

    const float invDet = 1.0f / det;
    out[0] = (a11 * a22 - a12 * a21) * invDet;
    out[1] = (a02 * a21 - a01 * a22) * invDet;
    out[2] = (a01 * a12 - a02 * a11) * invDet;
    out[3] = (a12 * a20 - a10 * a22) * invDet;
    out[4] = (a00 * a22 - a02 * a20) * invDet;
    out[5] = (a02 * a10 - a00 * a12) * invDet;
    out[6] = (a10 * a21 - a11 * a20) * invDet;
    out[7] = (a01 * a20 - a00 * a21) * invDet;
    out[8] = (a00 * a11 - a01 * a10) * invDet;
}

float colorByte(const int value) noexcept
{
    if (value <= 0) {
        return 0.0f;
    }
    if (value >= 255) {
        return 1.0f;
    }
    return static_cast<float>(value) / 255.0f;
}

} // namespace

bool drawGpuMatcapMesh(
    AppViewportState& state,
    const ViewportGpuMesh& mesh,
    const GLuint matcapTexture,
    const RgbColor tintColor
)
{
    if (!state.gpuCapabilities.shaderVboAvailable || !mesh.vertexBuffer || !mesh.indexBuffer ||
        matcapTexture == 0 || !ensureMatcapShader(state)) {
        return false;
    }

    GLint previousArrayBuffer = 0;
    GLint previousElementBuffer = 0;
    GLint previousActiveTexture = 0;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previousArrayBuffer);
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &previousElementBuffer);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);

    const ViewportMatcapShaderState& shader = state.matcapShader;
    ScopedGlProgram programScope;
    glad_glUseProgram(shader.program.get());

    float mvp[16]{};
    float normal[9]{};
    currentMvpMatrix(mvp);
    currentNormalMatrix(normal);
    glad_glUniformMatrix4fv(shader.mvpUniform, 1, GL_FALSE, mvp);
    glad_glUniformMatrix3fv(shader.normalMatrixUniform, 1, GL_FALSE, normal);
    glad_glUniform4f(
        shader.tintUniform,
        colorByte(tintColor.r),
        colorByte(tintColor.g),
        colorByte(tintColor.b),
        1.0f
    );
    glad_glUniform1i(shader.matcapUniform, 0);

    glad_glActiveTexture(GL_TEXTURE0);
    {
        ScopedGlTexture2DBinding textureBinding(matcapTexture);
        glad_glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBuffer.get());
        glad_glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexBuffer.get());
        const GLsizei stride = static_cast<GLsizei>(sizeof(float) * 6);
        glad_glEnableVertexAttribArray(static_cast<GLuint>(shader.positionAttrib));
        glad_glEnableVertexAttribArray(static_cast<GLuint>(shader.normalAttrib));
        glad_glVertexAttribPointer(static_cast<GLuint>(shader.positionAttrib), 3, GL_FLOAT, GL_FALSE, stride, nullptr);
        glad_glVertexAttribPointer(
            static_cast<GLuint>(shader.normalAttrib),
            3,
            GL_FLOAT,
            GL_FALSE,
            stride,
            reinterpret_cast<const void*>(sizeof(float) * 3)
        );
        glDrawElements(GL_TRIANGLES, mesh.indexCount, mesh.indexType, nullptr);
        glad_glDisableVertexAttribArray(static_cast<GLuint>(shader.normalAttrib));
        glad_glDisableVertexAttribArray(static_cast<GLuint>(shader.positionAttrib));
    }

    glad_glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(previousArrayBuffer));
    glad_glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLuint>(previousElementBuffer));
    glad_glActiveTexture(static_cast<GLenum>(previousActiveTexture));
    return true;
}

} // namespace ovtr::win32
