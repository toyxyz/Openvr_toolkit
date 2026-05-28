#include "TestCases.h"

#include <exception>
#include <iostream>

int main()
{
    try {
        ovtr::test::testSamplingScheduler();
        ovtr::test::testBinarySessionRoundTrip();
        ovtr::test::testBinarySessionRejectsTruncatedIndex();
        ovtr::test::testBinarySessionRejectsUnreasonablePoseCount();
        ovtr::test::testManifestWriter();
        ovtr::test::testFbxQuaternionToEuler();
        ovtr::test::testFbxSafeName();
        ovtr::test::testIdentifierUtilities();
        ovtr::test::testPoseInterpolation();
        ovtr::test::testPoseInterpolationSkipsUnreasonableResample();
        ovtr::test::testPoseTransform();
        ovtr::test::testFbxAsciiExport();
        ovtr::test::testFbxGeometryProvider();
        ovtr::test::testFbxEulerDiscontinuityFilter();
        ovtr::test::testFbxEulerTripletCompatibilityFilter();
        ovtr::test::testFbxExportSampleRate();
        ovtr::test::testGltfExport();
        ovtr::test::testGltfRejectsInvalidExportSampleRate();
        ovtr::test::testGlbExport();
        ovtr::test::testGlbExportUsesUint32IndicesForLargeMeshes();
        ovtr::test::testExportFilenameOnlyPaths();
        ovtr::test::testGlbImportRejectsMissingChunks();
        ovtr::test::testGlbImportRejectsDeclaredLengthMismatch();
        ovtr::test::testGlbImportRejectsInvalidAccessor();
        ovtr::test::testGlbImportRejectsUnsafeAccessorLayout();
        ovtr::test::testGlbImportAcceptsLargeIndex();
        ovtr::test::testRecordingController();
        ovtr::test::testMockVRProvider();
#ifdef _WIN32
        ovtr::test::testWin32ConfigStoreBasics();
        ovtr::test::testWin32ConfigStore();
        ovtr::test::testWin32DebugPanel();
        ovtr::test::testWin32DebugLayout();
        ovtr::test::testWin32DeviceList();
        ovtr::test::testWin32DeviceListLayout();
        ovtr::test::testWin32ImportedSceneLifecycle();
        ovtr::test::testWin32ImportedScenePlayback();
        ovtr::test::testWin32Layout();
        ovtr::test::testWin32MarkerState();
        ovtr::test::testWin32MarkerPoseCaptureAppliesOrigin();
        ovtr::test::testWin32MarkerListLayout();
        ovtr::test::testWin32OriginLayout();
        ovtr::test::testWin32OriginFormatting();
        ovtr::test::testWin32OriginDialogState();
        ovtr::test::testWin32OriginState();
        ovtr::test::testWin32PoseSamplingWorker();
        ovtr::test::testWin32RecordSettingsConfig();
        ovtr::test::testWin32RecordingActions();
        ovtr::test::testWin32RecordingCleanup();
        ovtr::test::testWin32RuntimeStatus();
        ovtr::test::testWin32StatusPanel();
        ovtr::test::testWin32String();
        ovtr::test::testWin32ViewportControlLayout();
        ovtr::test::testWin32ViewportGpuMesh();
        ovtr::test::testWin32ViewportSettingsConfig();
        ovtr::test::testWin32ViewportMath();
#endif
    } catch (const std::exception& error) {
        std::cerr << "Test failure: " << error.what() << '\n';
        return 1;
    }

    std::cout << "core tests passed\n";
    return 0;
}
