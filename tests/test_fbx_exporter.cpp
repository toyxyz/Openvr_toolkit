#include "TestCases.h"
#include "TestSupport.h"
#include "FbxTestSupport.h"

#include "data/SessionTypes.h"
#include "math/QuaternionUtils.h"

#include <cmath>
#include <filesystem>
#include <string>

namespace ovtr::test {

void testFbxAsciiExport()
{
    const std::filesystem::path testDir = std::filesystem::current_path() / ".tmp_ovtr_fbx_tests";
    std::error_code ignored;
    std::filesystem::remove_all(testDir, ignored);
    std::filesystem::create_directories(testDir);

    ovtr::FrameSample frame0 = makeTestFrame(0);
    ovtr::FrameSample frame1 = makeTestFrame(1);
    const float halfSqrt = std::sqrt(0.5f);
    frame0.poses[0].rotation = ovtr::normalizeQuaternion({0.0f, 0.0f, halfSqrt, halfSqrt});
    frame1.poses[0].rotation = ovtr::normalizeQuaternion({0.0f, 0.0f, halfSqrt, halfSqrt});
    ovtr::PoseSample invalidPose = frame1.poses[0];
    invalidPose.runtimeIndex = 4;
    invalidPose.deviceId = 2;
    invalidPose.flags = ovtr::PoseFlagDeviceConnected | ovtr::PoseFlagRecordEnabled;
    frame1.poses.push_back(invalidPose);

    const ovtr::DeviceDescriptor tracker = makeTestTracker("LHR-TEST");

    ovtr::DeviceDescriptor invalidTracker = makeTestTracker("LHR-INVALID");
    invalidTracker.id = 2;
    invalidTracker.runtimeIndex = 4;

    ovtr::FbxExportOptions options;
    options.includeGeometry = false;
    const std::string fbx = exportFbxAsciiForTest(
        testDir,
        {frame0, frame1},
        {tracker, invalidTracker},
        options,
        "fbx-test",
        "FBX Test",
        "FBX"
    );
    require(fbx.find("FBXVersion: 7400") != std::string::npos, "FBX version missing");
    require(fbx.find("P: \"UpAxis\", \"int\", \"Integer\", \"\",2") != std::string::npos, "FBX Blender-pass-through up axis missing");
    require(fbx.find("P: \"FrontAxis\", \"int\", \"Integer\", \"\",1") != std::string::npos, "FBX Blender-pass-through front axis missing");
    require(fbx.find("P: \"FrontAxisSign\", \"int\", \"Integer\", \"\",-1") != std::string::npos, "FBX Blender-pass-through front sign missing");
    require(fbx.find("P: \"CoordAxis\", \"int\", \"Integer\", \"\",0") != std::string::npos, "FBX Blender-pass-through coord axis missing");
    require(fbx.find("Model::OpenVR_Root") != std::string::npos, "FBX root model missing");
    require(fbx.find("Model::Tracker_LHR_TEST") != std::string::npos, "FBX tracker model missing");
    require(fbx.find("AnimCurveNode::Tracker_LHR_TEST_T\"") != std::string::npos, "FBX translation curve node missing");
    require(fbx.find("AnimCurveNode::Tracker_LHR_TEST_R\"") != std::string::npos, "FBX rotation curve node missing");
    require(fbx.find("P: \"RotationOrder\", \"enum\", \"\", \"\",0") != std::string::npos, "FBX XYZ rotation order missing");
    require(fbx.find("AnimCurveNode::Tracker_LHR_TEST_T_X\"") == std::string::npos, "FBX should not create per-axis translation curve nodes");
    require(fbx.find("AnimCurve::Tracker_LHR_TEST_T_X") != std::string::npos, "FBX translation curve missing");
    require(fbx.find("AnimCurve::Tracker_LHR_TEST_T_Y") != std::string::npos, "FBX translation Y curve missing");
    require(fbx.find("AnimCurve::Tracker_LHR_TEST_T_Z") != std::string::npos, "FBX translation Z curve missing");
    const std::string translationYCurve = findFbxObjectSection(fbx, "AnimCurve::Tracker_LHR_TEST_T_Y");
    const std::string translationZCurve = findFbxObjectSection(fbx, "AnimCurve::Tracker_LHR_TEST_T_Z");
    require(translationYCurve.find("-3.000000000") != std::string::npos, "FBX Y translation should use Blender front/back");
    require(translationZCurve.find("2.000000000") != std::string::npos, "FBX Z translation should use Blender height");
    require(fbx.find("AnimCurve::Tracker_LHR_TEST_R_Y") != std::string::npos, "FBX rotation Y curve missing");
    require(fbx.find("AnimCurve::Tracker_LHR_TEST_R_Z") != std::string::npos, "FBX rotation Z curve missing");
    const std::string rotationYCurve = findFbxObjectSection(fbx, "AnimCurve::Tracker_LHR_TEST_R_Y");
    const std::string rotationZCurve = findFbxObjectSection(fbx, "AnimCurve::Tracker_LHR_TEST_R_Z");
    require(
        rotationYCurve.find("-90.0000") != std::string::npos ||
            rotationYCurve.find("-89.999") != std::string::npos,
        "FBX OpenVR Z rotation should become Blender R_Y"
    );
    require(rotationZCurve.find("a: 0.000000000,0.000000000") != std::string::npos, "FBX Z curve should not receive OpenVR Z rotation");
    require(fbx.find("AnimCurve::Tracker_LHR_INVALID_T_X") != std::string::npos, "FBX invalid tracker curve missing");
    require(fbx.find("KeyValueFloat: *0") != std::string::npos, "invalid pose should not create key values");
    require(fbx.find("C: \"OO\",1000000,0") == std::string::npos, "FBX device should not be connected directly to the scene");
    require(fbx.find("C: \"OO\",1000000,") != std::string::npos, "FBX device should be connected to a parent root");

    std::filesystem::remove_all(testDir, ignored);
}

} // namespace ovtr::test
