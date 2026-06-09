#pragma once

namespace ovtr::test {

void testSamplingScheduler();
void testBinarySessionRoundTrip();
void testBinarySessionRejectsTruncatedIndex();
void testBinarySessionRejectsUnreasonablePoseCount();
void testManifestWriter();
void testRecordingController();

void testFbxQuaternionToEuler();
void testFbxSafeName();
void testIdentifierUtilities();
void testPoseInterpolation();
void testPoseInterpolationSkipsUnreasonableResample();
void testPoseTransform();

void testFbxAsciiExport();
void testFbxGeometryProvider();
void testFbxEulerDiscontinuityFilter();
void testFbxEulerTripletCompatibilityFilter();
void testFbxExportSampleRate();
void testGltfExport();
void testGltfRejectsInvalidExportSampleRate();
void testGlbExport();
void testGlbExportUsesUint32IndicesForLargeMeshes();
void testExportFilenameOnlyPaths();
void testSkeletalSyntheticPoseExportTracks();

void testGlbImportRejectsMissingChunks();
void testGlbImportRejectsDeclaredLengthMismatch();
void testGlbImportRejectsInvalidAccessor();
void testGlbImportRejectsUnsafeAccessorLayout();
void testGlbImportAcceptsLargeIndex();

void testMockVRProvider();

#ifdef _WIN32
void testWin32ConfigStore();
void testWin32ConfigStoreBasics();
void testWin32DebugPanel();
void testWin32DebugLayout();
void testWin32DeviceList();
void testWin32DeviceListLayout();
void testWin32ImportedSceneLifecycle();
void testWin32ImportedScenePlayback();
void testWin32Layout();
void testWin32MappingModel();
void testWin32MappingCalibration();
void testWin32MappingCalibrationAxes();
void testWin32MappingCoreDirectTargets();
void testWin32MappingEstimatedArmPole();
void testWin32MappingEstimatedChest();
void testWin32MappingLegParentFallback();
void testWin32MappingPelvisRotation();
void testWin32MappingPinnedTargets();
void testWin32MappingFingerSolve();
void testWin32MappingPanelLayout();
void testWin32MappingPanelStateLayout();
void testWin32MarkerState();
void testWin32MarkerPoseCaptureAppliesOrigin();
void testWin32MarkerListLayout();
void testWin32OriginLayout();
void testWin32OriginFormatting();
void testWin32OriginDialogState();
void testWin32OriginState();
void testWin32PoseSamplingWorker();
void testWin32ProfileLiveEdit();
void testWin32ProfileModel();
void testWin32ProfilePanelLayout();
void testWin32ProfileSerialization();
void testWin32ProfileSkeleton();
void testWin32ExportNoiseFilter();
void testWin32RecordSettingsConfig();
void testWin32RecordingActions();
void testWin32RecordingCleanup();
void testWin32RuntimeStatus();
void testWin32SessionPlayback();
void testWin32SideMenuVisibility();
void testWin32SkeletonConnectorRollStabilization();
void testWin32SkeletonLowerBoneIgnoresEndEffectorRotation();
void testWin32SkeletonPoseRestWorldJoints();
void testWin32SkeletonUpperArmIgnoresForearmDirection();
void testWin32SkeletonWorldSideAxesUseRestBasis();
void testWin32SkeletonGltfExport();
void testWin32SkeletonGltfFootFollowsToeDirection();
void testWin32SkeletonGltfHandRollFollowsForearm();
void testWin32SkeletonGltfLegChainKeepsRestAndAnimates();
void testWin32StatusPanel();
void testWin32String();
void testWin32ViewportControlLayout();
void testWin32ViewportGpuMesh();
void testWin32ViewportSettingsConfig();
void testWin32ViewportMath();
void testWin32VmcArmContinuity();
void testWin32VmcFingerInput();
void testWin32VmcFingerSolve();
#endif

} // namespace ovtr::test
