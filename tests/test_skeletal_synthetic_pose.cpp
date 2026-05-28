#include "TestCases.h"
#include "FbxTestSupport.h"
#include "TestSupport.h"

#include "data/SkeletalSyntheticPose.h"
#include "export/ExportPoseTrack.h"
#include "export/GltfExporter.h"

#include <array>
#include <cstdint>
#include <cmath>
#include <filesystem>
#include <string>
#include <vector>

namespace ovtr::test {
namespace {

const ovtr::ExportPoseTrack* findTrack(
    const std::vector<ovtr::ExportPoseTrack>& tracks,
    const std::string& nodeName
)
{
    for (const ovtr::ExportPoseTrack& track : tracks) {
        if (track.nodeName == nodeName) {
            return &track;
        }
    }
    return nullptr;
}

ovtr::PoseSample makeSyntheticBonePose(
    const ovtr::SkeletalHandSide side,
    const std::uint32_t boneIndex,
    const std::array<float, 3>& position
)
{
    ovtr::PoseSample pose;
    pose.deviceId = ovtr::skeletalBoneRuntimeIndex(side, boneIndex);
    pose.runtimeIndex = pose.deviceId;
    pose.position = position;
    pose.rotation = {0.0f, 0.0f, 0.0f, 1.0f};
    pose.flags = ovtr::PoseFlagDeviceConnected | ovtr::PoseFlagPoseValid | ovtr::PoseFlagRecordEnabled;
    return pose;
}

ovtr::PoseSample makeSyntheticBonePose()
{
    return makeSyntheticBonePose(ovtr::SkeletalHandSide::Left, 7, {0.12f, 0.34f, 0.56f});
}

ovtr::FrameSample makeSkeletalChainFrame()
{
    ovtr::FrameSample frame;
    frame.frameIndex = 0;
    frame.timestampNs = 0;
    frame.timeSeconds = 0.0;
    frame.poses.push_back(makeSyntheticBonePose(ovtr::SkeletalHandSide::Left, 0, {1.0f, 0.0f, 0.0f}));
    frame.poses.push_back(makeSyntheticBonePose(ovtr::SkeletalHandSide::Left, 1, {1.0f, 1.0f, 0.0f}));
    frame.poses.push_back(makeSyntheticBonePose(ovtr::SkeletalHandSide::Left, 6, {1.0f, 1.0f, 1.0f}));
    frame.poses.push_back(makeSyntheticBonePose(ovtr::SkeletalHandSide::Left, 7, {1.0f, 1.0f, 2.0f}));
    return frame;
}

std::string readGlbJson(const std::filesystem::path& path)
{
    const std::vector<std::uint8_t> glb = readBinaryFile(path);
    require(glb.size() > 28, "skeletal GLB output should contain chunks");
    require(readLittleEndianUint32(glb, 0) == 0x46546c67u, "skeletal GLB magic mismatch");
    require(readLittleEndianUint32(glb, 16) == 0x4e4f534au, "skeletal GLB first chunk should be JSON");
    const std::uint32_t jsonLength = readLittleEndianUint32(glb, 12);
    return std::string(reinterpret_cast<const char*>(glb.data() + 20), jsonLength);
}

} // namespace

void testSkeletalSyntheticPoseExportTracks()
{
    ovtr::SkeletalHandSide decodedSide = ovtr::SkeletalHandSide::Right;
    std::uint32_t decodedBone = 0;
    const std::uint32_t runtimeIndex =
        ovtr::skeletalBoneRuntimeIndex(ovtr::SkeletalHandSide::Left, 7);
    require(ovtr::isSkeletalBoneRuntimeIndex(runtimeIndex), "skeletal runtime index should be recognized");
    require(
        ovtr::decodeSkeletalBoneRuntimeIndex(runtimeIndex, decodedSide, decodedBone),
        "skeletal runtime index should decode"
    );
    require(decodedSide == ovtr::SkeletalHandSide::Left && decodedBone == 7, "skeletal index decode mismatch");
    require(ovtr::skeletalBoneName(2) == "Thumb_0", "thumb bone name mismatch");
    require(ovtr::skeletalBoneName(7) == "Index_1", "index bone name mismatch");
    std::uint32_t parentBone = 0;
    require(ovtr::skeletalBoneParentIndex(7, parentBone) && parentBone == 6, "index parent mismatch");
    require(!ovtr::skeletalBoneParentIndex(0, parentBone), "root should not have a parent");

    const std::filesystem::path testDir = std::filesystem::current_path() / ".tmp_ovtr_skeletal_pose_tests";
    std::error_code ignored;
    std::filesystem::remove_all(testDir, ignored);
    std::filesystem::create_directories(testDir);

    ovtr::FrameSample frame = makeTestFrame(0);
    frame.poses.push_back(makeSyntheticBonePose());
    const std::filesystem::path framesPath = testDir / "frames.bin";
    const std::filesystem::path indexPath = testDir / "frame_index.bin";
    writeFrameSamples(framesPath, indexPath, {frame}, "skeletal pose");

    const ovtr::RecordingSession session = makeTestSession(
        "skeletal-test",
        "Skeletal Test",
        framesPath,
        indexPath,
        {makeTestTracker("LHR-SKEL")}
    );

    std::vector<ovtr::ExportPoseTrack> tracks;
    std::string error;
    require(ovtr::collectExportPoseTracks(session, {}, tracks, error), "skeletal collect failed: " + error);

    const ovtr::ExportPoseTrack* track = findTrack(tracks, "Skeletal_Left_Index_1");
    require(track != nullptr, "skeletal recorded pose should create an export track");
    require(track->keys.size() == 1, "skeletal export track should keep recorded key");
    require(track->geometry.available, "skeletal export track should include box geometry");
    require(track->geometry.vertices.size() == 24, "skeletal box vertex count mismatch");
    require(track->geometry.indices.size() == 36, "skeletal box index count mismatch");
    const ovtr::RenderModelPositionBounds bounds = ovtr::renderModelPositionBounds(track->geometry);
    require(bounds.valid, "skeletal box bounds should be valid");
    const float sizeX = bounds.max[0] - bounds.min[0];
    const float sizeY = bounds.max[1] - bounds.min[1];
    const float sizeZ = bounds.max[2] - bounds.min[2];
    require(std::abs(sizeX - sizeY) < 0.0001f && std::abs(sizeY - sizeZ) < 0.0001f, "skeletal box should be cubic");
    require(std::abs(sizeX - ovtr::kSkeletalBoneBoxEdgeMeters) < 0.0001f, "skeletal box edge size mismatch");
    require(track->device.displayName == "Skeletal_Left_Index_1", "skeletal display name mismatch");
    require(track->device.renderModelName == ovtr::kSkeletalBoneBoxRenderModelName, "skeletal model name mismatch");

    const std::filesystem::path glbPath = testDir / "skeletal_box.glb";
    ovtr::GltfExportOptions options;
    options.outputPath = glbPath;
    options.format = ovtr::GltfExportFormat::Glb;
    const ovtr::ExportResult result = ovtr::exportSessionToGltf(session, options);
    require(result.success, "skeletal GLB box export failed: " + result.error);
    const std::string json = readGlbJson(glbPath);
    require(json.find("\"name\": \"Skeletal_Left_Index_1\"") != std::string::npos, "skeletal GLB node missing");
    require(json.find("\"name\": \"Skeletal_Left_Index_1_Mesh\"") != std::string::npos, "skeletal GLB box mesh missing");
    require(json.find("\"modelName\": \"ovtr_skeletal_bone_box\"") != std::string::npos, "skeletal GLB model missing");
    require(json.find("\"mesh\"") != std::string::npos, "skeletal GLB should export box mesh reference");

    const std::filesystem::path hierarchyFramesPath = testDir / "hierarchy_frames.bin";
    const std::filesystem::path hierarchyIndexPath = testDir / "hierarchy_frame_index.bin";
    writeFrameSamples(hierarchyFramesPath, hierarchyIndexPath, {makeSkeletalChainFrame()}, "skeletal hierarchy");
    const ovtr::RecordingSession hierarchySession = makeTestSession(
        "skeletal-hierarchy-test",
        "Skeletal Hierarchy Test",
        hierarchyFramesPath,
        hierarchyIndexPath,
        {}
    );

    ovtr::GltfExportOptions hierarchyOptions;
    hierarchyOptions.outputPath = testDir / "skeletal_hierarchy.glb";
    hierarchyOptions.format = ovtr::GltfExportFormat::Glb;
    const ovtr::ExportResult hierarchyResult = ovtr::exportSessionToGltf(hierarchySession, hierarchyOptions);
    require(hierarchyResult.success, "skeletal hierarchy GLB export failed: " + hierarchyResult.error);
    const std::string hierarchyJson = readGlbJson(hierarchyOptions.outputPath);
    const std::size_t rootNode = hierarchyJson.find("\"name\": \"Skeletal_Left_Root\"");
    const std::size_t wristNode = hierarchyJson.find("\"name\": \"Skeletal_Left_Wrist\"");
    const std::size_t index0Node = hierarchyJson.find("\"name\": \"Skeletal_Left_Index_0\"");
    const std::size_t index1Node = hierarchyJson.find("\"name\": \"Skeletal_Left_Index_1\"");
    require(rootNode != std::string::npos, "skeletal hierarchy root node missing");
    require(wristNode != std::string::npos, "skeletal hierarchy wrist node missing");
    require(index0Node != std::string::npos, "skeletal hierarchy index0 node missing");
    require(index1Node != std::string::npos, "skeletal hierarchy index1 node missing");
    require(rootNode < wristNode && wristNode < index0Node && index0Node < index1Node, "skeletal hierarchy node order mismatch");
    require(hierarchyJson.find("\"children\": [1]") != std::string::npos, "GLB scene root should contain skeletal root");
    require(hierarchyJson.find("\"children\": [2]", rootNode) != std::string::npos, "GLB root bone should parent wrist");
    require(hierarchyJson.find("\"children\": [3]", wristNode) != std::string::npos, "GLB wrist should parent index0");
    require(hierarchyJson.find("\"children\": [4]", index0Node) != std::string::npos, "GLB index0 should parent index1");
    require(hierarchyJson.find("\"LeftHand\"") == std::string::npos, "GLB should keep box nodes, not hand skeleton nodes");
    require(
        hierarchyJson.find("\"name\": \"Skeletal_Left_Wrist\", \"translation\": [0.000000000,1.000000000,0.000000000]") != std::string::npos,
        "GLB wrist should use parent-local translation"
    );
    require(
        hierarchyJson.find("\"name\": \"Skeletal_Left_Index_1\", \"translation\": [0.000000000,0.000000000,1.000000000]") != std::string::npos,
        "GLB child finger box should use parent-local translation"
    );

    ovtr::FbxExportOptions fbxOptions;
    fbxOptions.includeGeometry = false;
    fbxOptions.coordinatePolicy = ovtr::FbxCoordinatePolicy::OpenVRNative;
    const std::string fbx = exportFbxAsciiForTest(
        testDir,
        {makeSkeletalChainFrame()},
        {},
        fbxOptions,
        "skeletal-fbx-hierarchy-test",
        "Skeletal FBX Hierarchy Test",
        "skeletal FBX hierarchy"
    );
    require(fbx.find("Model::Skeletal_Left_Index_1") != std::string::npos, "FBX skeletal box model missing");
    require(fbx.find("C: \"OO\",1000000,1000040") != std::string::npos, "FBX root bone should attach to export root");
    require(fbx.find("C: \"OO\",1000010,1000000") != std::string::npos, "FBX wrist should attach to root bone");
    require(fbx.find("C: \"OO\",1000020,1000010") != std::string::npos, "FBX index0 should attach to wrist");
    require(fbx.find("C: \"OO\",1000030,1000020") != std::string::npos, "FBX index1 should attach to index0");

    std::filesystem::remove_all(testDir, ignored);
}

} // namespace ovtr::test
