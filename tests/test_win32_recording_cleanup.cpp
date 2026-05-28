#include "TestCases.h"
#include "TestSupport.h"

#include "platform/win32/RecordingCleanupActions.h"
#include "platform/win32/RecordingSessionActions.h"

#include <filesystem>
#include <fstream>
#include <system_error>

namespace ovtr::test {

void testWin32RecordingCleanup()
{
    const std::filesystem::path root = std::filesystem::current_path() / ".tmp_ovtr_recording_actions";
    const std::filesystem::path folder = ovtr::win32::recordingSessionFolder(root, "session_action");

    std::error_code ignored;
    std::filesystem::remove_all(root, ignored);
    std::filesystem::create_directories(folder, ignored);
    require(ovtr::win32::isPathWithinDirectory(folder, root), "path within directory");

    const std::filesystem::path sibling = root.parent_path() / ".tmp_ovtr_recording_actions_sibling";
    std::filesystem::remove_all(sibling, ignored);
    std::filesystem::create_directories(sibling, ignored);
    require(!ovtr::win32::isPathWithinDirectory(sibling, root), "sibling path is outside directory");

    const std::filesystem::path cleanupFolder = root / "session_cleanup";
    std::filesystem::create_directories(cleanupFolder, ignored);
    {
        std::ofstream output(cleanupFolder / "frames.bin");
        output << "temporary";
    }

    std::filesystem::path currentSessionFolder = cleanupFolder;
    std::string cleanupMessage;
    require(
        ovtr::win32::deleteTemporarySessionFolder(currentSessionFolder, root, cleanupMessage),
        "delete temporary session folder"
    );
    require(currentSessionFolder.empty(), "cleanup clears session folder");
    require(!std::filesystem::exists(cleanupFolder), "cleanup removes session folder");
    require(
        cleanupMessage.find("Temporary session folder deleted:") == 0,
        "cleanup reports deleted folder"
    );

    std::filesystem::path missingSessionFolder = root / "session_missing";
    require(
        ovtr::win32::deleteTemporarySessionFolder(missingSessionFolder, root, cleanupMessage),
        "cleanup tolerates missing session folder"
    );
    require(missingSessionFolder.empty(), "missing cleanup clears session folder");
    require(cleanupMessage.empty(), "missing cleanup has no message");

    std::filesystem::path outsideSessionFolder = sibling;
    require(
        !ovtr::win32::deleteTemporarySessionFolder(outsideSessionFolder, root, cleanupMessage),
        "cleanup rejects outside folder"
    );
    require(outsideSessionFolder == sibling, "rejected cleanup keeps session folder");
    require(
        cleanupMessage == "session cleanup skipped: folder is outside recordings",
        "rejected cleanup message"
    );

    std::filesystem::remove_all(root, ignored);
    std::filesystem::remove_all(sibling, ignored);
}

} // namespace ovtr::test
