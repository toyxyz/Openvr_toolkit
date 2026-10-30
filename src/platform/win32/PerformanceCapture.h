#pragma once

#include <windows.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace ovtr::win32 {

// Opt-in UI-thread tracing. No file writes or clocks per event when disabled.
class PerformanceCapture {
public:
    using Clock = std::chrono::steady_clock;
    static PerformanceCapture& instance()
    {
        static PerformanceCapture capture;
        return capture;
    }
    bool enabled() const noexcept { return output_.is_open(); }
    void input()
    {
        if (!enabled()) return;
        input_ = Clock::now();
        ++inputSequence_;
    }
    void record(const char* event, const Clock::time_point start)
    {
        if (!enabled()) return;
        const auto end = Clock::now();
        if (end - origin_ >= std::chrono::minutes(3)) {
            output_.close();
            return;
        }
        const double age = inputSequence_ != 0 ? milliseconds(end - input_) : -1.0;
        output_ << milliseconds(end - origin_) << ',' << event << ','
                << milliseconds(end - start) << ',' << inputSequence_ << ',' << age << '\n';
        if (end - lastFlush_ >= std::chrono::seconds(1)) {
            output_.flush();
            if (!output_) {
                OutputDebugStringW(L"Writing viewport_performance.csv failed; capture stopped.\n");
                output_.close();
            }
            lastFlush_ = end;
        }
    }
private:
    PerformanceCapture()
    {
        wchar_t executable[32768]{};
        const DWORD length = GetModuleFileNameW(nullptr, executable, 32768);
        if (length == 0 || length >= 32768) {
            throw std::runtime_error("Locating executable for performance capture failed");
        }
        const auto folder = std::filesystem::path(executable).parent_path();
        if (!std::filesystem::exists(folder / "performance_capture.flag")) return;
        output_.open(folder / "viewport_performance.csv", std::ios::trunc);
        if (!output_) throw std::runtime_error("Opening viewport performance capture failed");
        output_ << "elapsed_ms,event,duration_ms,input_sequence,input_age_ms\n";
    }
    static double milliseconds(const Clock::duration duration)
    {
        return std::chrono::duration<double, std::milli>(duration).count();
    }
    std::ofstream output_;
    Clock::time_point origin_ = Clock::now();
    Clock::time_point lastFlush_ = origin_;
    Clock::time_point input_{};
    unsigned long long inputSequence_ = 0;
};

class PerformanceScope {
public:
    explicit PerformanceScope(const char* event) : event_(event)
    {
        if (PerformanceCapture::instance().enabled()) start_ = PerformanceCapture::Clock::now();
    }
    ~PerformanceScope()
    {
        if (start_ != PerformanceCapture::Clock::time_point{}) {
            PerformanceCapture::instance().record(event_, start_);
        }
    }
private:
    const char* event_;
    PerformanceCapture::Clock::time_point start_{};
};

} // namespace ovtr::win32
