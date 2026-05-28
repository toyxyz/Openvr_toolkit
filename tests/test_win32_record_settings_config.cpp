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
        "record_delay=2.250\n"
        "export_fps=90\n"
        "format=fbx\n"
    );
    const ovtr::win32::RecordSettingsConfig record =
        ovtr::win32::parseRecordSettingsConfig(recordInput, 60.0f);
    require(record.exportDirectoryText == "C:/captures", "parse record directory");
    require(win32ConfigNearlyEqual(record.recordDelaySeconds, 2.25f), "parse record delay");
    require(win32ConfigNearlyEqual(record.exportSampleRate, 90.0f), "parse record sample rate");
    require(record.saveFormat == ovtr::win32::ExportFormat::Fbx, "parse record format");

    std::istringstream legacyRecordInput("  D:/legacy exports  \n");
    const ovtr::win32::RecordSettingsConfig legacyRecord =
        ovtr::win32::parseRecordSettingsConfig(legacyRecordInput, 60.0f);
    require(legacyRecord.exportDirectoryText == "D:/legacy exports", "parse legacy record directory");

    const std::string serializedRecord = ovtr::win32::serializeRecordSettingsConfig(
        "E:/new exports",
        -1.0f,
        std::numeric_limits<float>::infinity(),
        ovtr::win32::ExportFormat::Glb,
        72.0f
    );
    require(
        serializedRecord.find("record_delay_seconds=0.000") != std::string::npos,
        "serialize sanitized record delay"
    );
    require(
        serializedRecord.find("resample_fps=72.000") != std::string::npos,
        "serialize fallback sample rate"
    );

    const std::string serializedLargeSampleRate = ovtr::win32::serializeRecordSettingsConfig(
        "E:/new exports",
        0.0f,
        50000.0f,
        ovtr::win32::ExportFormat::Glb,
        72.0f
    );
    require(
        serializedLargeSampleRate.find("resample_fps=1000.000") != std::string::npos,
        "serialize clamped sample rate"
    );

    ovtr::win32::RecordSettingsDialogInput dialogInput;
    dialogInput.initialDirectory = "relative dialog exports";
    dialogInput.initialRecordDelaySeconds = -3.0f;
    dialogInput.initialExportSampleRate = std::numeric_limits<float>::infinity();
    dialogInput.initialSaveFormat = ovtr::win32::ExportFormat::Fbx;
    dialogInput.defaultExportSampleRate = 72.0f;
    const ovtr::win32::RecordSettingsDialogResult initialDialogResult =
        ovtr::win32::initialRecordSettingsDialogResult(dialogInput);
    require(
        initialDialogResult.directory ==
            (std::filesystem::current_path() / "relative dialog exports").lexically_normal(),
        "record settings dialog normalizes initial directory"
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
        initialDialogResult.saveFormat == ovtr::win32::ExportFormat::Fbx,
        "record settings dialog preserves save format"
    );
}

} // namespace ovtr::test
