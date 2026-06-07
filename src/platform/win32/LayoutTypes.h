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
    RECT quadViewButtonRect{0, 0, 0, 0};
    RECT showTextButtonRect{0, 0, 0, 0};
    RECT showModelButtonRect{0, 0, 0, 0};
    RECT smoothButtonRect{0, 0, 0, 0};
    RECT recordButtonRect{0, 0, 0, 0};
    RECT sessionBoxRect{0, 0, 0, 0};
    RECT sessionLabelRect{0, 0, 0, 0};
    RECT sessionValueRect{0, 0, 0, 0};
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

struct MarkerListLayout {
    RECT boxRect{0, 0, 0, 0};
    RECT headerRect{0, 0, 0, 0};
    RECT contentRect{0, 0, 0, 0};
    int visibleItemCount = 0;
    bool valid = false;
};

struct SessionListLayout {
    RECT boxRect{0, 0, 0, 0};
    RECT headerRect{0, 0, 0, 0};
    RECT contentRect{0, 0, 0, 0};
    int visibleItemCount = 0;
    bool valid = false;
};

struct StreamingPanelLayout {
    RECT boxRect{0, 0, 0, 0};
    RECT headerRect{0, 0, 0, 0};
    RECT targetBoxRect{0, 0, 0, 0};
    RECT targetLabelRect{0, 0, 0, 0};
    RECT targetValueRect{0, 0, 0, 0};
    RECT targetDropdownRect{0, 0, 0, 0};
    RECT vmcBoxRect{0, 0, 0, 0};
    RECT vmcHeaderRect{0, 0, 0, 0};
    RECT hostBoxRect{0, 0, 0, 0};
    RECT hostLabelRect{0, 0, 0, 0};
    RECT hostValueRect{0, 0, 0, 0};
    RECT portBoxRect{0, 0, 0, 0};
    RECT portLabelRect{0, 0, 0, 0};
    RECT portValueRect{0, 0, 0, 0};
    RECT armSpacingBoxRect{0, 0, 0, 0};
    RECT armSpacingLabelRect{0, 0, 0, 0};
    RECT armSpacingValueRect{0, 0, 0, 0};
    RECT legSpacingBoxRect{0, 0, 0, 0};
    RECT legSpacingLabelRect{0, 0, 0, 0};
    RECT legSpacingValueRect{0, 0, 0, 0};
    bool valid = false;
    bool vmcVisible = false;
};

struct ProfilePanelLayout {
    RECT panelRect{0, 0, 0, 0};
    RECT splitterRect{0, 0, 0, 0};
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
