#include "platform/win32/ViewportAnimationControlSections.h"
#include "platform/win32/PlaybackTimelineView.h"
#include "platform/win32/UiTheme.h"

#include "platform/win32/AppImportedSceneState.h"
#include "platform/win32/AppLoadedSessionState.h"
#include "platform/win32/ImportedScenePlayback.h"
#include "platform/win32/SessionPlayback.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>

namespace ovtr::win32 {
namespace {

constexpr int kRulerHeight = 24;
constexpr int kBadgeHeight = 20;
constexpr int kPointerHeight = 6;
constexpr int kMajorLabelSpacing = 90;

std::int64_t majorFrameStep(const int frames, const int width)
{
    const double desired = static_cast<double>(frames) / (std::max)(1, width / kMajorLabelSpacing);
    const double scale = std::pow(10.0, std::floor(std::log10((std::max)(1.0, desired))));
    for (const double multiplier : {1.0, 2.0, 5.0, 10.0}) {
        if (multiplier * scale >= desired) return static_cast<std::int64_t>(multiplier * scale);
    }
    return static_cast<std::int64_t>(10.0 * scale);
}

void drawPlayhead(HDC dc, const RECT& bounds, const int x, const int currentFrame)
{
    const auto text = std::to_wstring(currentFrame);
    SIZE extent{};
    GetTextExtentPoint32W(dc, text.c_str(), static_cast<int>(text.size()), &extent);
    const int width = (std::min)(bounds.right - bounds.left,
        static_cast<LONG>((std::max)(24, static_cast<int>(extent.cx) + 12)));
    const int left = std::clamp(x - width / 2, static_cast<int>(bounds.left),
        static_cast<int>(bounds.right) - width);
    const int top = bounds.top + 1;
    const int bottom = top + kBadgeHeight;
    UniqueBrush brush(CreateSolidBrush(ui::accent));
    UniquePen pen(CreatePen(PS_SOLID, 1, ui::accent));
    SelectObjectGuard brushSelection(dc, brush.get());
    SelectObjectGuard penSelection(dc, pen.get());
    RoundRect(dc, left, top, left + width, bottom, 5, 5);
    POINT pointer[]{{x - 5, bottom - 2}, {x + 5, bottom - 2}, {x, bottom + kPointerHeight}};
    Polygon(dc, pointer, 3);
    MoveToEx(dc, x, bottom + kPointerHeight, nullptr);
    LineTo(dc, x, bounds.bottom);
    SetTextColor(dc, ui::onAccent);
    RECT label{left, top, left + width, bottom};
    DrawTextW(dc, text.c_str(), -1, &label, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
}

void drawAnimationTimeline(HDC dc, const ViewportControlLayout& layout,
    const double seconds, const double duration, const int currentFrame, const int totalFrames)
{
    const RECT rect = layout.timelineRect;
    const int width = rect.right - rect.left;
    if (!layout.animationValid || width <= 0) return;
    // The frame badge may extend into the side gutter while its point remains
    // exactly aligned with the seek position, including the first and last frame.
    const RECT bounds{layout.animationBarRect.left, rect.top,
        layout.animationBarRect.right, rect.bottom};
    const int saved = SaveDC(dc);
    if (saved) IntersectClipRect(dc, bounds.left, bounds.top, bounds.right, bounds.bottom);
    const int rulerBottom = rect.top + kRulerHeight;
    ui::fill(dc, bounds, ui::timelineTrack);
    ui::fill(dc, RECT{bounds.left, bounds.top, bounds.right, rulerBottom}, ui::timelineRuler);
    const int frames = totalFrames > 0 ? totalFrames : 250;
    const auto step = majorFrameStep(frames, width);
    UniquePen division(CreatePen(PS_SOLID, 1, ui::timelineDivision));
    {
        SelectObjectGuard penSelection(dc, division.get());
        SetBkMode(dc, TRANSPARENT);
        // Two broad track columns per numbered interval; no horizontal grid
        // and no dense per-frame tick marks in the ruler.
        const double interval = static_cast<double>(step) / 2.0;
        for (int tick = 0; ; ++tick) {
            const double frame = static_cast<double>(tick) * interval;
            if (frame > frames) break;
            const int x = rect.left + static_cast<int>(
                (std::max)(0.0, frame - 1.0) / (std::max)(1, frames - 1) * (width - 1));
            MoveToEx(dc, x, rulerBottom, nullptr);
            LineTo(dc, x, bounds.bottom);
            if (tick % 2 == 0 && frame > 0.0) {
                RECT label{x - 32, rect.top, x + 32, rulerBottom};
                SetTextColor(dc, ui::muted);
                const auto text = std::to_wstring(static_cast<std::int64_t>(frame));
                DrawTextW(dc, text.c_str(), -1, &label, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
            }
        }
    }
    const double factor = duration > 0.0 ? std::clamp(seconds / duration, 0.0, 1.0) : 0.0;
    const int x = rect.left + static_cast<int>(factor * (width - 1));
    SetBkMode(dc, TRANSPARENT);
    drawPlayhead(dc, bounds, x, currentFrame);
    if (saved) RestoreDC(dc, saved);
}

} // namespace

void drawCombinedAnimationTimeline(HDC dc, const ViewportControlLayout& layout,
    const AppImportedSceneState& glb, const AppLoadedSessionState& session)
{
    const auto view = playbackTimelineView(glb, session);
    drawAnimationTimeline(dc, layout, view.seconds, view.duration, view.currentFrame, view.totalFrames);
}

void drawImportedAnimationTimeline(HDC dc, const ViewportControlLayout& layout, const AppImportedSceneState& state)
{
    drawAnimationTimeline(dc, layout, state.importedScenePlaybackSeconds, importedSceneDurationSeconds(state),
        importedSceneCurrentFrame(state), importedSceneTotalFrames(state));
}

void drawLoadedSessionAnimationTimeline(HDC dc, const ViewportControlLayout& layout, const AppLoadedSessionState& state)
{
    drawAnimationTimeline(dc, layout, state.loadedSessionPlaybackSeconds, loadedSessionDurationSeconds(state),
        loadedSessionCurrentFrame(state), loadedSessionTotalFrames(state));
}

void drawEmptyAnimationTimeline(HDC dc, const ViewportControlLayout& layout)
{
    drawAnimationTimeline(dc, layout, 0.0, 0.0, 0, 0);
}

} // namespace ovtr::win32
