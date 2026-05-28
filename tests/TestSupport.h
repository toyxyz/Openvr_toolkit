#pragma once

#include "data/SessionTypes.h"
#include "export/RenderModelGeometry.h"

#include <array>
#include <cstdint>
#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace ovtr::test {

void require(bool condition, const std::string& message);

class ScopedCurrentPath {
public:
    explicit ScopedCurrentPath(const std::filesystem::path& path);
    ~ScopedCurrentPath();

    ScopedCurrentPath(const ScopedCurrentPath&) = delete;
    ScopedCurrentPath& operator=(const ScopedCurrentPath&) = delete;

private:
    std::filesystem::path previous_;
};

FrameSample makeTestFrame(std::uint64_t frameIndex);
std::vector<FrameSample> makeTestFrames(std::uint64_t frameCount);
DeviceDescriptor makeTestTracker(const std::string& serial);
RecordingSession makeTestSession(
    const std::string& sessionId,
    const std::string& sessionName,
    const std::filesystem::path& framesPath,
    const std::filesystem::path& indexPath,
    const std::vector<DeviceDescriptor>& devices
);
void writeFrameSamples(
    const std::filesystem::path& framesPath,
    const std::filesystem::path& indexPath,
    const std::vector<FrameSample>& frames,
    const std::string& errorPrefix
);
void writeTestFrames(
    const std::filesystem::path& framesPath,
    const std::filesystem::path& indexPath,
    std::uint64_t frameCount,
    const std::string& errorPrefix
);
std::string readTextFile(const std::filesystem::path& path);
std::vector<std::uint8_t> readBinaryFile(const std::filesystem::path& path);
std::uint32_t readLittleEndianUint32(const std::vector<std::uint8_t>& bytes, std::size_t offset);
RenderModelGeometry makeTriangleGeometry();
std::array<float, 4> axisAngleQuaternionZ(double degrees);
std::array<float, 4> axisAngleQuaternionY(double degrees);

} // namespace ovtr::test
