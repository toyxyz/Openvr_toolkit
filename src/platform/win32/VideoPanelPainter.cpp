#include "platform/win32/VideoPanelPainter.h"
#include "platform/win32/UiTheme.h"

#include "platform/win32/AppState.h"
#include "platform/win32/Win32GdiResources.h"
#include "platform/win32/Win32String.h"

#include <algorithm>

namespace ovtr::win32 {
namespace {

constexpr int kCameraRowHeight = 34;

void drawBorderedBox(HDC dc, const RECT& rect, const COLORREF fill)
{
    UniqueBrush brush(CreateSolidBrush(fill));
    FillRect(dc, &rect, brush.get());
    UniquePen pen(CreatePen(PS_SOLID, 1, ui::border));
    SelectObjectGuard penSelection(dc, pen.get());
    SelectObjectGuard brushSelection(dc, GetStockObject(NULL_BRUSH));
    Rectangle(dc, rect.left, rect.top, rect.right, rect.bottom);
}

VideoFrameSnapshot displayedFrame(const AppWindowState& state)
{
    if (state.loadedSessionActive && !state.loadedSession.videoPath.empty()) {
        return state.sessionVideoPlayback
            ? state.sessionVideoPlayback->latestFrame()
            : VideoFrameSnapshot{};
    }
    return state.videoCapture ? state.videoCapture->latestFrame() : VideoFrameSnapshot{};
}

void drawPreview(HDC dc, const RECT& rect, const VideoFrameSnapshot& frame)
{
    drawBorderedBox(dc, rect, RGB(5, 7, 10));
    if (!frame || frame->width == 0 || frame->height == 0 || frame->bgra.empty()) {
        SetTextColor(dc, ui::muted);
        RECT text = rect;
        DrawTextW(dc, L"No video frame", -1, &text, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        return;
    }
    const int availableWidth = (std::max)(1, static_cast<int>(rect.right - rect.left - 2));
    const int availableHeight = (std::max)(1, static_cast<int>(rect.bottom - rect.top - 2));
    const double scale = (std::min)(
        static_cast<double>(availableWidth) / frame->width,
        static_cast<double>(availableHeight) / frame->height);
    const int width = (std::max)(1, static_cast<int>(frame->width * scale));
    const int height = (std::max)(1, static_cast<int>(frame->height * scale));
    const int left = rect.left + (rect.right - rect.left - width) / 2;
    const int top = rect.top + (rect.bottom - rect.top - height) / 2;
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = static_cast<LONG>(frame->width);
    // Media Foundation RGB32 frames are exposed top-down. A negative DIB height
    // keeps GDI from interpreting the first row as the bottom of the image.
    info.bmiHeader.biHeight = frame->topDown
        ? -static_cast<LONG>(frame->height)
        : static_cast<LONG>(frame->height);
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    // COLORONCOLOR drops source pixels when shrinking a camera frame and creates
    // harsh moire/posterization. HALFTONE performs a filtered resize instead.
    const int previousStretchMode = SetStretchBltMode(dc, HALFTONE);
    POINT previousBrushOrigin{};
    SetBrushOrgEx(dc, 0, 0, &previousBrushOrigin);
    StretchDIBits(dc, left, top, width, height, 0, 0,
        static_cast<int>(frame->width), static_cast<int>(frame->height),
        frame->bgra.data(), &info, DIB_RGB_COLORS, SRCCOPY);
    SetBrushOrgEx(dc, previousBrushOrigin.x, previousBrushOrigin.y, nullptr);
    SetStretchBltMode(dc, previousStretchMode);
}

} // namespace

void paintVideoPanel(
    HDC drawDc,
    HFONT bodyFont,
    HFONT headerFont,
    const AppWindowState& state,
    const VideoPanelLayout& layout
) {
    if (!layout.valid) {
        return;
    }
    SetBkMode(drawDc, TRANSPARENT);
    drawBorderedBox(drawDc, layout.boxRect, ui::panel);
    SelectObjectGuard headerSelection(drawDc, headerFont ? headerFont : bodyFont);
    SetTextColor(drawDc, ui::text);
    RECT header = layout.headerRect;
    DrawTextW(drawDc, L"Video Recording", -1, &header, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    drawBorderedBox(drawDc, layout.refreshButtonRect, ui::header);
    SelectObject(drawDc, bodyFont);
    SetTextColor(drawDc, ui::text);
    RECT refresh = layout.refreshButtonRect;
    DrawTextW(drawDc, L"Refresh Cameras", -1, &refresh, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SetTextColor(drawDc, RGB(132, 211, 174));
    RECT status = layout.statusRect;
    DrawTextW(drawDc, widen(state.videoStatusMessage).c_str(), -1, &status,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    drawBorderedBox(drawDc, layout.cameraListRect, ui::field);
    RECT disableRow{layout.cameraListRect.left + 1, layout.cameraListRect.top,
        layout.cameraListRect.right - 1, layout.cameraListRect.top + kCameraRowHeight};
    if (state.selectedVideoCameraLink.empty()) {
        UniqueBrush selected(CreateSolidBrush(RGB(40, 91, 78)));
        FillRect(drawDc, &disableRow, selected.get());
    }
    disableRow.left += 9;
    SetTextColor(drawDc, ui::text);
    DrawTextW(drawDc, L"Disable", -1, &disableRow,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    for (int index = 0; index < static_cast<int>(state.videoCameras.size()) &&
        index + 1 < layout.visibleCameraCount; ++index) {
        RECT row{layout.cameraListRect.left + 1,
            layout.cameraListRect.top + (index + 1) * kCameraRowHeight,
            layout.cameraListRect.right - 1,
            layout.cameraListRect.top + (index + 2) * kCameraRowHeight};
        if (index == state.selectedVideoCamera) {
            UniqueBrush selected(CreateSolidBrush(RGB(40, 91, 78)));
            FillRect(drawDc, &row, selected.get());
        }
        row.left += 9;
        SetTextColor(drawDc, ui::text);
        DrawTextW(drawDc, state.videoCameras[index].displayName.c_str(), -1, &row,
            DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    }
    drawPreview(drawDc, layout.previewRect, displayedFrame(state));
    if (state.videoCapture && state.videoCapture->recording()) {
        RECT recording = layout.previewRect;
        recording.left += 10;
        recording.top += 8;
        SetTextColor(drawDc, RGB(255, 92, 92));
        DrawTextW(drawDc, L"REC", -1, &recording, DT_LEFT | DT_TOP | DT_SINGLELINE);
    }
}

} // namespace ovtr::win32
