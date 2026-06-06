#include "platform/win32/RecordingCleanupActions.h"

#include <system_error>

namespace ovtr::win32 {
namespace {

bool canDeleteRecordingSessionFolder(
    const std::filesystem::path& sessionFolder,
    const std::filesystem::path& recordingsRoot
)
{
    if (!isPathWithinDirectory(sessionFolder, recordingsRoot)) {
        return false;
    }
    if (sessionFolder.filename().string().rfind("session_", 0) == 0) {
        return true;
    }
    std::error_code error;
    return std::filesystem::exists(sessionFolder / "manifest.json", error) && !error;
}

} // namespace

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
    if (!isPathWithinDirectory(sessionFolder, recordingsRoot)) {
        message = "session cleanup skipped: folder is outside recordings";
        return false;
    }
    if (!canDeleteRecordingSessionFolder(sessionFolder, recordingsRoot)) {
        message = "session cleanup skipped: folder is not a recording session";
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

bool deleteTemporarySessionFolders(
    const std::filesystem::path& recordingsRoot,
    std::vector<std::string>& messages
)
{
    messages.clear();
    std::error_code error;
    if (!std::filesystem::exists(recordingsRoot, error)) {
        return true;
    }
    if (!std::filesystem::is_directory(recordingsRoot, error)) {
        messages.push_back("session cleanup skipped: recordings root is not a directory");
        return false;
    }

    bool allSucceeded = true;
    for (const std::filesystem::directory_entry& entry :
         std::filesystem::directory_iterator(recordingsRoot, error)) {
        if (error) {
            messages.push_back("session cleanup failed: " + error.message());
            return false;
        }
        if (!entry.is_directory(error)) {
            continue;
        }
        std::filesystem::path folder = entry.path();
        if (!canDeleteRecordingSessionFolder(folder, recordingsRoot)) {
            continue;
        }
        std::string message;
        if (!deleteTemporarySessionFolder(folder, recordingsRoot, message)) {
            allSucceeded = false;
        }
        if (!message.empty()) {
            messages.push_back(message);
        }
    }
    return allSucceeded;
}

} // namespace ovtr::win32
