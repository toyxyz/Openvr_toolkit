#include "TestCases.h"
#include "TestSupport.h"
#include "FbxTestSupport.h"

#include "data/SessionTypes.h"

#include <filesystem>
#include <string>

namespace ovtr::test {

void testFbxGeometryProvider()
{
    const std::filesystem::path testDir = std::filesystem::current_path() / ".tmp_ovtr_fbx_geometry_provider_tests";
    std::error_code ignored;
    std::filesystem::remove_all(testDir, ignored);
    std::filesystem::create_directories(testDir);

    const std::filesystem::path fbxPath = testDir / "export.fbx";

    const ovtr::DeviceDescriptor tracker = makeTestTracker("LHR-GEOMETRY");

    bool providerCalled = false;
    ovtr::FbxExportOptions options;
    options.outputPath = fbxPath;
    options.coordinatePolicy = ovtr::FbxCoordinatePolicy::OpenVRNative;
    options.geometryProvider = [&providerCalled](const ovtr::DeviceDescriptor&) {
        providerCalled = true;
        return makeTriangleGeometry();
    };
    const std::string fbx = exportFbxAsciiForTest(
        testDir,
        {makeTestFrame(0)},
        {tracker},
        options,
        "fbx-geometry-provider-test",
        "FBX Geometry Provider Test",
        "FBX geometry provider"
    );
    require(providerCalled, "FBX geometry provider should be called");

    require(fbx.find("Geometry::Tracker_LHR_GEOMETRY_Geometry") != std::string::npos, "FBX geometry missing");
    require(fbx.find("C: \"OO\",1000001,1000000") != std::string::npos, "FBX geometry should connect to model");

    std::filesystem::remove_all(testDir, ignored);
}

} // namespace ovtr::test
