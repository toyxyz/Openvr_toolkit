#pragma once

namespace ovtr::win32 {

enum class StreamingOutputTarget {
    None,
    Vmc,
};

inline const wchar_t* streamingOutputTargetLabel(const StreamingOutputTarget target) noexcept
{
    switch (target) {
    case StreamingOutputTarget::Vmc:
        return L"VMC";
    case StreamingOutputTarget::None:
    default:
        return L"None";
    }
}

} // namespace ovtr::win32
