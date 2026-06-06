#include "TestCases.h"
#include "TestSupport.h"
#include "Win32ConfigTestSupport.h"

#include "platform/win32/ConfigStore.h"
#include "platform/win32/RecordSettingsModel.h"

#include <filesystem>
#include <limits>
#include <sstream>
#include <string>

namespace ovtr::test {

void testWin32RecordSettingsConfig()
{
    require(win32ConfigNearlyEqual(ovtr::win32::sanitizedRecordDelaySeconds(1.5f), 1.5f), "valid record delay");
    require(win32ConfigNearlyEqual(ovtr::win32::sanitizedRecordDelaySeconds(-1.0f), 0.0f), "negative record delay");
    require(
        win32ConfigNearlyEqual(
            ovtr::win32::sanitizedExportSampleRate(
                std::numeric_limits<float>::infinity(),
                60.0f
            ),
            60.0f
        ),
        "invalid sample rate fallback"
    );
    require(
        win32ConfigNearlyEqual(
            ovtr::win32::sanitizedExportSampleRate(50000.0f, 60.0f),
            1000.0f
        ),
        "large sample rate clamp"
    );

    std::istringstream recordInput(
        "directory=C:/captures\n"
        "session_directory=D:/sessions\n"
        "record_delay=2.250\n"
        "export_fps=90\n"
        "format=fbx\n"
        "start_recording_on_calibration=true\n"
        "export_after_recording=true\n"
        "apply_noise_filter_on_export=true\n"
        "noise_filter_cutoff_hz=9\n"
        "outlier_repair_strength=strong\n"
        "smoothing_iterations=2\n"
    );
    const ovtr::win32::RecordSettingsConfig record =
        ovtr::win32::parseRecordSettingsConfig(recordInput, 60.0f);
    require(record.exportDirectoryText == "C:/captures", "parse record directory");
    require(record.sessionDirectoryText == "D:/sessions", "parse session directory");
    require(win32ConfigNearlyEqual(record.recordDelaySeconds, 2.25f), "parse record delay");
    require(win32ConfigNearlyEqual(record.exportSampleRate, 90.0f), "parse record sample rate");
    require(record.startRecordingOnCalibration, "parse calibration-start recording option");
    require(record.exportAfterRecording, "parse export-after-recording option");
    require(record.applyNoiseFilterOnExport, "parse export noise filter option");
    require(win32ConfigNearlyEqual(record.noiseFilterCutoffHz, 8.0f), "parse nearest noise cutoff option");
    require(
        record.outlierRepairStrength == ovtr::win32::OutlierRepairStrength::Strong,
        "parse outlier repair strength"
    );
    require(
        record.smoothingIterations == 2,
        "parse smoothing iterations"
    );

    std::istringstream legacySmoothingInput("gaussian_smoothing_strength=strong\n");
    const ovtr::win32::RecordSettingsConfig legacySmoothingRecord =
        ovtr::win32::parseRecordSettingsConfig(legacySmoothingInput, 60.0f);
    require(
        legacySmoothingRecord.smoothingIterations == 4,
        "parse legacy gaussian smoothing strength key as iterations"
    );

    std::istringstream clampedSmoothingInput("smoothing_iterations=101\n");
    const ovtr::win32::RecordSettingsConfig clampedSmoothingRecord =
        ovtr::win32::parseRecordSettingsConfig(clampedSmoothingInput, 60.0f);
    require(clampedSmoothingRecord.smoothingIterations == 100, "clamp smoothing iterations from config");

    std::istringstream legacyRecordInput("  D:/legacy exports  \noutlier_repair_strength=unknown\n");
    const ovtr::win32::RecordSettingsConfig legacyRecord =
        ovtr::win32::parseRecordSettingsConfig(legacyRecordInput, 60.0f);
    require(legacyRecord.exportDirectoryText == "D:/legacy exports", "parse legacy record directory");
    require(legacyRecord.sessionDirectoryText.empty(), "legacy record has no session directory");
    require(!legacyRecord.applyNoiseFilterOnExport, "legacy record disables export noise filter");
    require(!legacyRecord.exportAfterRecording, "legacy record disables export-after-recording");
    require(win32ConfigNearlyEqual(legacyRecord.noiseFilterCutoffHz, 8.0f), "legacy record default noise cutoff");
    require(
        legacyRecord.outlierRepairStrength == ovtr::win32::OutlierRepairStrength::Light,
        "legacy record defaults outlier repair strength"
    );
    require(
        legacyRecord.smoothingIterations == 0,
        "legacy record defaults smoothing iterations"
    );

    const std::string serializedRecord = ovtr::win32::serializeRecordSettingsConfig(
        "E:/new exports",
        "F:/new sessions",
        -1.0f,
        std::numeric_limits<float>::infinity(),
        true,
        true,
        true,
        20.0f,
        ovtr::win32::OutlierRepairStrength::Normal,
        4,
        72.0f
    );
    require(
        serializedRecord.find("record_delay_seconds=0.000") != std::string::npos,
        "serialize sanitized record delay"
    );
    require(
        serializedRecord.find("session_directory=F:/new sessions") != std::string::npos,
        "serialize session directory"
    );
    require(
        serializedRecord.find("resample_fps=72.000") != std::string::npos,
        "serialize fallback sample rate"
    );
    require(
        serializedRecord.find("save_format") == std::string::npos,
        "serialize omits save format"
    );
    require(
        serializedRecord.find("start_recording_on_calibration=true") != std::string::npos,
        "serialize calibration-start recording option"
    );
    require(
        serializedRecord.find("apply_noise_filter_on_export=true") != std::string::npos,
        "serialize export noise filter option"
    );
    require(
        serializedRecord.find("export_after_recording=true") != std::string::npos,
        "serialize export-after-recording option"
    );
    require(
        serializedRecord.find("noise_filter_cutoff_hz=20.000") != std::string::npos,
        "serialize export noise filter cutoff"
    );
    require(
        serializedRecord.find("outlier_repair_strength=normal") != std::string::npos,
        "serialize outlier repair strength"
    );
    require(
        serializedRecord.find("smoothing_iterations=4") != std::string::npos,
        "serialize smoothing iterations"
    );

    const std::string serializedLargeSampleRate = ovtr::win32::serializeRecordSettingsConfig(
        "E:/new exports",
        "F:/new sessions",
        0.0f,
        50000.0f,
        false,
        false,
        false,
        0.5f,
        ovtr::win32::OutlierRepairStrength::None,
        0,
        72.0f
    );
    require(
        serializedLargeSampleRate.find("resample_fps=1000.000") != std::string::npos,
        "serialize clamped sample rate"
    );
    require(
        serializedLargeSampleRate.find("noise_filter_cutoff_hz=0.500") != std::string::npos,
        "serialize low export noise filter cutoff"
    );
    require(
        serializedLargeSampleRate.find("outlier_repair_strength=none") != std::string::npos,
        "serialize disabled outlier repair strength"
    );
    require(
        serializedLargeSampleRate.find("smoothing_iterations=0") != std::string::npos,
        "serialize disabled smoothing iterations"
    );

    ovtr::win32::RecordSettingsDialogInput dialogInput;
    dialogInput.initialDirectory = "relative dialog exports";
    dialogInput.initialSessionDirectory = "relative dialog sessions";
    dialogInput.initialRecordDelaySeconds = -3.0f;
    dialogInput.initialExportSampleRate = std::numeric_limits<float>::infinity();
    dialogInput.initialStartRecordingOnCalibration = true;
    dialogInput.initialExportAfterRecording = true;
    dialogInput.initialApplyNoiseFilterOnExport = true;
    dialogInput.initialNoiseFilterCutoffHz = 3.0f;
    dialogInput.initialOutlierRepairStrength = ovtr::win32::OutlierRepairStrength::Strong;
    dialogInput.initialSmoothingIterations = 1;
    dialogInput.defaultExportSampleRate = 72.0f;
    const ovtr::win32::RecordSettingsDialogResult initialDialogResult =
        ovtr::win32::initialRecordSettingsDialogResult(dialogInput);
    require(
        initialDialogResult.directory ==
            (std::filesystem::current_path() / "relative dialog exports").lexically_normal(),
        "record settings dialog normalizes initial directory"
    );
    require(
        initialDialogResult.sessionDirectory ==
            (std::filesystem::current_path() / "relative dialog sessions").lexically_normal(),
        "record settings dialog normalizes initial session directory"
    );
    require(
        win32ConfigNearlyEqual(initialDialogResult.recordDelaySeconds, 0.0f),
        "record settings dialog clamps delay"
    );
    require(
        win32ConfigNearlyEqual(initialDialogResult.exportSampleRate, 72.0f),
        "record settings dialog falls back sample rate"
    );
    require(
        initialDialogResult.startRecordingOnCalibration,
        "record settings dialog preserves calibration-start recording option"
    );
    require(
        initialDialogResult.exportAfterRecording,
        "record settings dialog preserves export-after-recording option"
    );
    require(initialDialogResult.applyNoiseFilterOnExport, "record settings dialog preserves noise filter option");
    require(
        win32ConfigNearlyEqual(initialDialogResult.noiseFilterCutoffHz, 2.0f),
        "record settings dialog normalizes noise cutoff"
    );
    require(
        initialDialogResult.outlierRepairStrength == ovtr::win32::OutlierRepairStrength::Strong,
        "record settings dialog preserves outlier repair strength"
    );
    require(
        initialDialogResult.smoothingIterations == 1,
        "record settings dialog preserves smoothing iterations"
    );
}

} // namespace ovtr::test
