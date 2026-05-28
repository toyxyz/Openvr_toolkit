#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace ovtr::win32 {

struct DebugScrollbarLayout {
    RECT trackRect{0, 0, 0, 0};
    RECT thumbRect{0, 0, 0, 0};
    bool valid = false;
};

struct ViewportControlLayout {
    RECT barRect{0, 0, 0, 0};
    RECT recordButtonRect{0, 0, 0, 0};
    RECT animationBarRect{0, 0, 0, 0};
    RECT firstFrameButtonRect{0, 0, 0, 0};
    RECT playPauseButtonRect{0, 0, 0, 0};
    RECT lastFrameButtonRect{0, 0, 0, 0};
    RECT timelineRect{0, 0, 0, 0};
    RECT frameTextRect{0, 0, 0, 0};
    RECT closeButtonRect{0, 0, 0, 0};
    bool valid = false;
    bool animationValid = false;
};

struct DeviceListLayout {
    RECT boxRect{0, 0, 0, 0};
    RECT headerRect{0, 0, 0, 0};
    RECT contentRect{0, 0, 0, 0};
    int visibleItemCount = 0;
    bool valid = false;
};

struct OriginPanelLayout {
    RECT boxRect{0, 0, 0, 0};
    RECT valueRect{0, 0, 0, 0};
    bool valid = false;
};

struct OriginStepperButton {
    RECT rect{0, 0, 0, 0};
    bool rotation = false;
    int axis = 0;
    float delta = 0.0f;
    bool valid = false;
};

struct OriginStepperAxisLayout {
    RECT axisLabelRect{0, 0, 0, 0};
    RECT valueRect{0, 0, 0, 0};
    OriginStepperButton minusButton{};
    OriginStepperButton plusButton{};
    bool rotation = false;
    int axis = 0;
    bool valid = false;
};

} // namespace ovtr::win32
