#pragma once

#include "export/RenderModelGeometry.h"

#include <array>
#include <filesystem>
#include <string>
#include <vector>

namespace ovtr {

struct ImportedGltfKey {
    double timeSeconds = 0.0;
    std::array<float, 3> translation{0.0f, 0.0f, 0.0f};
    std::array<float, 4> rotation{0.0f, 0.0f, 0.0f, 1.0f};
};

struct ImportedGltfNode {
    std::string name;
    int nodeIndex = -1;
    int meshIndex = -1;
    std::array<float, 3> translation{0.0f, 0.0f, 0.0f};
    std::array<float, 4> rotation{0.0f, 0.0f, 0.0f, 1.0f};
    std::array<float, 3> scale{1.0f, 1.0f, 1.0f};
    std::vector<ImportedGltfKey> keys;
};

struct ImportedGltfScene {
    std::filesystem::path sourcePath;
    std::string name;
    double durationSeconds = 0.0;
    std::vector<RenderModelGeometry> meshes;
    std::vector<ImportedGltfNode> nodes;
};

struct GltfImportResult {
    bool success = false;
    std::filesystem::path inputPath;
    ImportedGltfScene scene;
    std::string error;
};

GltfImportResult importGlbScene(const std::filesystem::path& inputPath);

bool sampleImportedGltfNodePose(
    const ImportedGltfNode& node,
    double timeSeconds,
    std::array<float, 3>& translation,
    std::array<float, 4>& rotation
);

} // namespace ovtr
