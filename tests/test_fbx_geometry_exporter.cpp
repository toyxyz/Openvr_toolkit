#include "TestCases.h"
#include "TestSupport.h"
#include "FbxTestSupport.h"

#include "data/SessionTypes.h"

#include <filesystem>
#include <string>

namespace ovtr::test {
namespace {

ovtr::ExportStaticPoseTrack makeFbxMarkerTrack()
{
    ovtr::DeviceDescriptor marker;
    marker.id = 0xF0000001u;
    marker.runtimeIndex = 0xF0000001u;
    marker.serial = "marker_1";
    marker.displayName = "marker_1";
    marker.role = "marker";
    marker.modelName = "Marker Box";
    marker.deviceClass = ovtr::DeviceClass::Other;

    ovtr::ExportStaticPoseTrack track;
    track.device = marker;
    track.nodeName = "marker_1";
    track.translation = {4.0f, 5.0f, 6.0f};
    track.rotation = {0.0f, 0.0f, 0.0f, 1.0f};
    track.geometry = ovtr::makeBoxRenderModelGeometry(0.10f);
    return track;
}

} // namespace

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
    options.staticTracks.push_back(makeFbxMarkerTrack());
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
    require(fbx.find("Model::marker_1") != std::string::npos, "FBX marker model missing");
    require(fbx.find("Geometry::marker_1_Geometry") != std::string::npos, "FBX marker geometry missing");
    require(fbx.find("AnimCurve::marker_1_T_X") != std::string::npos, "FBX marker static curve missing");
    require(fbx.find("C: \"OO\",1000001,1000000") != std::string::npos, "FBX geometry should connect to model");

    std::filesystem::remove_all(testDir, ignored);
}

} // namespace ovtr::test
