#pragma once

#include "data/SessionTypes.h"
#include "export/FbxAsciiExporter.h"

#include <filesystem>
#include <string>
#include <vector>

namespace ovtr::test {

std::string exportFbxAsciiForTest(
    const std::filesystem::path& testDir,
    const std::vector<ovtr::FrameSample>& frames,
    const std::vector<ovtr::DeviceDescriptor>& devices,
    ovtr::FbxExportOptions options,
    const std::string& sessionId,
    const std::string& sessionName,
    const std::string& errorPrefix
);

inline std::string findFbxObjectSection(const std::string& fbx, const std::string& marker)
{
    const std::size_t start = fbx.find(marker);
    if (start == std::string::npos) {
        return {};
    }

    const std::size_t nextCurve = fbx.find("    AnimationCurve:", start + marker.size());
    if (nextCurve == std::string::npos) {
        return fbx.substr(start);
    }
    return fbx.substr(start, nextCurve - start);
}

} // namespace ovtr::test
