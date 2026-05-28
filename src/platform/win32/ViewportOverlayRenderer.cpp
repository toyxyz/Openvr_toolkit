#include "platform/win32/ViewportOverlayRenderer.h"

#include "platform/win32/AppRecordingState.h"
#include "platform/win32/AppViewportState.h"
#include "platform/win32/StatusPanel.h"
#include "platform/win32/ViewportDrawPrimitives.h"
#include "platform/win32/ViewportGlMatrixScope.h"
#include "platform/win32/ViewportGlStateScope.h"

#include <gl/GL.h>

#include <chrono>
#include <mutex>
#include <string>

namespace ovtr::win32 {
namespace {

constexpr float kRecordingViewportBorderPixels = 5.0f;
constexpr float kRecordingElapsedRightPadding = 24.0f;
constexpr float kRecordingElapsedTopBaseline = 42.0f;
constexpr float kRecordingElapsedMinimumLeft = 8.0f;
constexpr float kRecordingElapsedApproxGlyphWidth = 16.0f;
constexpr RgbColor kRecordingElapsedTextColor{255, 255, 255};

bool shouldDrawRecordingViewportBorder(const AppRecordingState& state)
{
    std::lock_guard<std::mutex> lock(state.recordingMutex);
    return state.recorder.state() == ovtr::RecorderState::Recording;
}

long long recordingElapsedSeconds(const AppRecordingState& state)
{
    std::lock_guard<std::mutex> lock(state.recordingMutex);
    const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - state.recordingStart
    );
    const long long seconds = elapsed.count();
    return seconds > 0 ? seconds : 0;
}

void drawOverlayText2D(
    const std::string& text,
    const GLuint fontBase,
    const float x,
    const float y,
    const RgbColor color,
    const int width,
    const int height
)
{
    if (text.empty() || fontBase == 0 || width <= 0 || height <= 0) {
        return;
    }

    ScopedGlMatrixStack projection(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(
        0.0,
        static_cast<GLdouble>(width),
        static_cast<GLdouble>(height),
        0.0,
        -1.0,
        1.0
    );

    ScopedGlMatrixStack modelView(GL_MODELVIEW);
    glLoadIdentity();

    ScopedGlCapability depthTest(GL_DEPTH_TEST, false);
    ScopedGlCapability lighting(GL_LIGHTING, false);
    ScopedGlCapability texture2D(GL_TEXTURE_2D, false);
    ScopedGlCapability cullFace(GL_CULL_FACE, false);
    setGlColor(color);
    glRasterPos2f(x, y);
    drawLabelText3D(text, fontBase);
}

void drawRecordingViewportBorder2D(const int width, const int height)
{
    if (width <= 0 || height <= 0) {
        return;
    }

    ScopedGlMatrixStack projection(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(
        0.0,
        static_cast<GLdouble>(width),
        static_cast<GLdouble>(height),
        0.0,
        -1.0,
        1.0
    );

    ScopedGlMatrixStack modelView(GL_MODELVIEW);
    glLoadIdentity();

    ScopedGlCapability depthTest(GL_DEPTH_TEST, false);
    ScopedGlCapability lighting(GL_LIGHTING, false);
    ScopedGlCapability texture2D(GL_TEXTURE_2D, false);
    ScopedGlCapability cullFace(GL_CULL_FACE, false);
    glColor3f(1.0f, 0.04f, 0.02f);
    ScopedGlLineWidth lineWidth(kRecordingViewportBorderPixels);

    const float inset = kRecordingViewportBorderPixels * 0.5f;
    glBegin(GL_LINE_LOOP);
    glVertex2f(inset, inset);
    glVertex2f(static_cast<float>(width) - inset, inset);
    glVertex2f(static_cast<float>(width) - inset, static_cast<float>(height) - inset);
    glVertex2f(inset, static_cast<float>(height) - inset);
    glEnd();
}

void drawDelayCountdownOverlay2D(
    const AppRecordingState& recordingState,
    const AppViewportState& viewportState,
    const int width,
    const int height
)
{
    if (!recordingState.recordingDelayActive || !viewportState.glOverlayFontBase || width <= 0 || height <= 0) {
        return;
    }

    const std::string text = std::to_string(remainingRecordDelaySeconds(recordingState));
    drawOverlayText2D(
        text,
        viewportState.glOverlayFontBase.get(),
        24.0f,
        64.0f,
        viewportState.viewportSettings.labelTextColor,
        width,
        height
    );
}

void drawRecordingElapsedOverlay2D(
    const AppRecordingState& recordingState,
    const AppViewportState& viewportState,
    const int width,
    const int height
)
{
    if (!shouldDrawRecordingViewportBorder(recordingState) || !viewportState.glRecordingElapsedFontBase) {
        return;
    }

    const std::string text = std::to_string(recordingElapsedSeconds(recordingState)) + "s";
    const float estimatedWidth = static_cast<float>(text.size()) * kRecordingElapsedApproxGlyphWidth;
    const float rightAlignedX = static_cast<float>(width) - estimatedWidth - kRecordingElapsedRightPadding;
    const float x = rightAlignedX > kRecordingElapsedMinimumLeft ? rightAlignedX : kRecordingElapsedMinimumLeft;

    drawOverlayText2D(
        text,
        viewportState.glRecordingElapsedFontBase.get(),
        x,
        kRecordingElapsedTopBaseline,
        kRecordingElapsedTextColor,
        width,
        height
    );
}

} // namespace

void drawViewportOverlays2D(
    const AppRecordingState& recordingState,
    const AppViewportState& viewportState,
    const int width,
    const int height
)
{
    drawDelayCountdownOverlay2D(recordingState, viewportState, width, height);

    if (shouldDrawRecordingViewportBorder(recordingState)) {
        drawRecordingViewportBorder2D(width, height);
        drawRecordingElapsedOverlay2D(recordingState, viewportState, width, height);
    }
}

} // namespace ovtr::win32
