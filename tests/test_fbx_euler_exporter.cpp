#include "TestCases.h"
#include "TestSupport.h"
#include "FbxTestSupport.h"

#include "data/SessionTypes.h"
#include "math/QuaternionUtils.h"

#include <filesystem>
#include <string>

namespace ovtr::test {

void testFbxEulerDiscontinuityFilter()
{
    const std::filesystem::path testDir = std::filesystem::current_path() / ".tmp_ovtr_fbx_euler_filter_tests";
    std::error_code ignored;
    std::filesystem::remove_all(testDir, ignored);
    std::filesystem::create_directories(testDir);

    ovtr::FrameSample frame0 = makeTestFrame(0);
    ovtr::FrameSample frame1 = makeTestFrame(1);
    frame0.poses[0].rotation = ovtr::normalizeQuaternion(axisAngleQuaternionZ(170.0));
    frame1.poses[0].rotation = ovtr::normalizeQuaternion(axisAngleQuaternionZ(190.0));

    ovtr::FbxExportOptions options;
    options.includeGeometry = false;
    options.coordinatePolicy = ovtr::FbxCoordinatePolicy::OpenVRNative;
    const std::string fbx = exportFbxAsciiForTest(
        testDir,
        {frame0, frame1},
        {makeTestTracker("LHR-FILTER")},
        options,
        "fbx-euler-filter-test",
        "FBX Euler Filter Test",
        "FBX Euler filter"
    );
    const std::string rotationZCurve = findFbxObjectSection(fbx, "AnimCurve::Tracker_LHR_FILTER_R_Z");
    require(rotationZCurve.find("170.0000") != std::string::npos, "FBX Euler filter first Z key mismatch");
    require(
        rotationZCurve.find("190.0000") != std::string::npos ||
            rotationZCurve.find("189.999") != std::string::npos,
        "FBX Euler filter should continue through 180 degrees"
    );
    require(rotationZCurve.find("-170.0000") == std::string::npos, "FBX Euler filter should remove wrapped -170 degree key");

    std::filesystem::remove_all(testDir, ignored);
}

void testFbxEulerTripletCompatibilityFilter()
{
    const std::filesystem::path testDir = std::filesystem::current_path() / ".tmp_ovtr_fbx_euler_triplet_tests";
    std::error_code ignored;
    std::filesystem::remove_all(testDir, ignored);
    std::filesystem::create_directories(testDir);

    ovtr::FrameSample frame0 = makeTestFrame(0);
    ovtr::FrameSample frame1 = makeTestFrame(1);
    frame0.poses[0].rotation = ovtr::normalizeQuaternion(axisAngleQuaternionY(89.0));
    frame1.poses[0].rotation = ovtr::normalizeQuaternion(axisAngleQuaternionY(91.0));

    ovtr::FbxExportOptions options;
    options.includeGeometry = false;
    options.coordinatePolicy = ovtr::FbxCoordinatePolicy::OpenVRNative;
    const std::string fbx = exportFbxAsciiForTest(
        testDir,
        {frame0, frame1},
        {makeTestTracker("LHR-TRIPLET")},
        options,
        "fbx-euler-triplet-test",
        "FBX Euler Triplet Test",
        "FBX Euler triplet"
    );
    const std::string rotationXCurve = findFbxObjectSection(fbx, "AnimCurve::Tracker_LHR_TRIPLET_R_X");
    const std::string rotationYCurve = findFbxObjectSection(fbx, "AnimCurve::Tracker_LHR_TRIPLET_R_Y");
    const std::string rotationZCurve = findFbxObjectSection(fbx, "AnimCurve::Tracker_LHR_TRIPLET_R_Z");
    require(rotationXCurve.find("180.0000") == std::string::npos, "FBX Euler triplet filter should avoid X 180-degree flip");
    require(rotationZCurve.find("180.0000") == std::string::npos, "FBX Euler triplet filter should avoid Z 180-degree flip");
    require(
        rotationYCurve.find("89.0000") != std::string::npos ||
            rotationYCurve.find("88.999") != std::string::npos,
        "FBX Euler triplet first Y key mismatch"
    );
    require(
        rotationYCurve.find("91.0000") != std::string::npos ||
            rotationYCurve.find("91.000") != std::string::npos ||
            rotationYCurve.find("90.999") != std::string::npos,
        "FBX Euler triplet filter should keep Y continuous past 90 degrees"
    );

    std::filesystem::remove_all(testDir, ignored);
}

} // namespace ovtr::test
