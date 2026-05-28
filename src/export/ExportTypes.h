#pragma once

#include <cmath>
#include <filesystem>
#include <string>

namespace ovtr {

inline constexpr double kMaxExportSampleRate = 1000.0;

inline bool isValidExportSampleRate(const double sampleRate) noexcept
{
    return sampleRate == 0.0 ||
        (std::isfinite(sampleRate) && sampleRate > 0.0 && sampleRate <= kMaxExportSampleRate);
}

inline double sanitizedExportSampleRateValue(const double sampleRate, const double fallback) noexcept
{
    if (!std::isfinite(sampleRate) || sampleRate <= 0.0) {
        if (!std::isfinite(fallback) || fallback <= 0.0) {
            return kMaxExportSampleRate;
        }
        return fallback > kMaxExportSampleRate ? kMaxExportSampleRate : fallback;
    }
    return sampleRate > kMaxExportSampleRate ? kMaxExportSampleRate : sampleRate;
}

struct ExportResult {
    bool success = false;
    std::filesystem::path outputPath;
    std::string error;
};

} // namespace ovtr
