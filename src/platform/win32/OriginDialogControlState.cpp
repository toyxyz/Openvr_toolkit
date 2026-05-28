#include "platform/win32/OriginDialogSessionInternal.h"

#include "platform/win32/Dialogs.h"

#include <array>

namespace ovtr::win32 {

bool readOriginDialogControls(HWND hwnd, OriginDialogState& dialog, const bool showWarning)
{
    std::array<float, 3> offset{};
    std::array<float, 3> rotation{};
    for (int axis = 0; axis < 3; ++axis) {
        if (!readFiniteFloatEdit(
                dialog.controls.positionEdits[static_cast<std::size_t>(axis)],
                offset[static_cast<std::size_t>(axis)]
            ) ||
            !readFiniteFloatEdit(
                dialog.controls.rotationEdits[static_cast<std::size_t>(axis)],
                rotation[static_cast<std::size_t>(axis)]
            )) {
            if (showWarning) {
                MessageBoxW(
                    hwnd,
                    L"Origin values must be valid numbers.",
                    L"Origin Settings",
                    MB_OK | MB_ICONWARNING
                );
            }
            return false;
        }
    }

    OriginDialogValues values;
    values.enabled = dialog.controls.enabledCheck
        ? SendMessageW(dialog.controls.enabledCheck, BM_GETCHECK, 0, 0) == BST_CHECKED
        : true;
    values.offset = offset;
    values.rotationDegrees = rotation;
    dialog.workingValues = values;
    return true;
}

void updateOriginDialogControls(OriginDialogState& dialog)
{
    dialog.updatingControls = true;
    if (dialog.controls.enabledCheck) {
        SendMessageW(
            dialog.controls.enabledCheck,
            BM_SETCHECK,
            dialog.workingValues.enabled ? BST_CHECKED : BST_UNCHECKED,
            0
        );
    }

    for (int axis = 0; axis < 3; ++axis) {
        setEditText(
            dialog.controls.positionEdits[static_cast<std::size_t>(axis)],
            formatFloatText(dialog.workingValues.offset[static_cast<std::size_t>(axis)])
        );
        setEditText(
            dialog.controls.rotationEdits[static_cast<std::size_t>(axis)],
            formatFloatText(dialog.workingValues.rotationDegrees[static_cast<std::size_t>(axis)])
        );
    }
    dialog.updatingControls = false;
}

} // namespace ovtr::win32
