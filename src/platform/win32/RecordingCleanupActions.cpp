#include "platform/win32/RecordingCleanupActions.h"

#include <system_error>

namespace ovtr::win32 {

bool isPathWithinDirectory(
    const std::filesystem::path& child,
    const std::filesystem::path& parent
)
{
    std::error_code error;
    const std::filesystem::path parentPath = std::filesystem::weakly_canonical(parent, error);
    if (error) {
        return false;
    }

    error.clear();
    std::filesystem::path childPath = std::filesystem::weakly_canonical(child, error);
    if (error) {
        error.clear();
        const std::filesystem::path childParentPath =
            std::filesystem::weakly_canonical(child.parent_path(), error);
        if (error) {
            return false;
        }
        childPath = (childParentPath / child.filename()).lexically_normal();
    }

    const std::filesystem::path relative = childPath.lexically_relative(parentPath);
    if (relative.empty()) {
        return false;
    }

    const auto first = relative.begin();
    if (first == relative.end()) {
        return false;
    }

    return *first != "..";
}

bool deleteTemporarySessionFolder(
    std::filesystem::path& currentSessionFolder,
    const std::filesystem::path& recordingsRoot,
    std::string& message
)
{
    message.clear();
    if (currentSessionFolder.empty()) {
        return true;
    }

    const std::filesystem::path sessionFolder = currentSessionFolder;
    const std::string folderName = sessionFolder.filename().string();
    if (folderName.rfind("session_", 0) != 0 ||
        !isPathWithinDirectory(sessionFolder, recordingsRoot)) {
        message = "session cleanup skipped: folder is outside recordings";
        return false;
    }

    std::error_code existsError;
    if (!std::filesystem::exists(sessionFolder, existsError)) {
        currentSessionFolder.clear();
        return true;
    }

    std::error_code removeError;
    const std::uintmax_t removedCount = std::filesystem::remove_all(sessionFolder, removeError);
    if (removeError) {
        message = "session cleanup failed: " + removeError.message();
        return false;
    }

    currentSessionFolder.clear();
    message = "Temporary session folder deleted: " + sessionFolder.string() +
        " (" + std::to_string(removedCount) + " entries)";
    return true;
}

} // namespace ovtr::win32
