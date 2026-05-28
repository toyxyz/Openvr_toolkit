#include "platform/win32/WindowLayout.h"

namespace ovtr::win32 {

int visibleDebugLogLineCount(const RECT& messagesRect)
{
    return visibleDebugLineCountForRect(messagesRect);
}

} // namespace ovtr::win32
