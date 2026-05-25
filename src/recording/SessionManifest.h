#pragma once

#include "data/SessionTypes.h"

#include <cstdint>
#include <filesystem>
#include <string>

namespace ovtr {

struct SessionManifestStats {
    std::uint64_t frameCount = 0;
    double durationSeconds = 0.0;
    std::uint64_t droppedFrames = 0;
    bool finalized = false;
};

std::string makeManifestJson(const RecordingSession& session, const SessionManifestStats& stats);
bool writeManifestJson(
    const RecordingSession& session,
    const SessionManifestStats& stats,
    const std::filesystem::path& path,
    std::string& outError
);

} // namespace ovtr

