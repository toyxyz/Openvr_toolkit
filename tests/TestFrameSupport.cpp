#include "TestSupport.h"

#include "math/QuaternionUtils.h"
#include "recording/BinarySessionWriter.h"

namespace ovtr::test {

ovtr::FrameSample makeTestFrame(const std::uint64_t frameIndex)
{
    ovtr::FrameSample frame;
    frame.frameIndex = frameIndex;
    frame.timestampNs = frameIndex * 11'111'111;
    frame.timeSeconds = static_cast<double>(frame.timestampNs) / 1'000'000'000.0;

    ovtr::PoseSample pose;
    pose.deviceId = 1;
    pose.runtimeIndex = 3;
    pose.position = {1.0f + static_cast<float>(frameIndex), 2.0f, 3.0f};
    pose.rotation = ovtr::normalizeQuaternion({0.0f, 0.5f, 0.0f, 0.5f});
    pose.velocity = {0.1f, 0.2f, 0.3f};
    pose.angularVelocity = {0.01f, 0.02f, 0.03f};
    pose.flags = ovtr::PoseFlagDeviceConnected | ovtr::PoseFlagPoseValid | ovtr::PoseFlagRecordEnabled;
    frame.poses.push_back(pose);

    return frame;
}

std::vector<ovtr::FrameSample> makeTestFrames(const std::uint64_t frameCount)
{
    std::vector<ovtr::FrameSample> frames;
    frames.reserve(static_cast<std::size_t>(frameCount));
    for (std::uint64_t frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
        frames.push_back(makeTestFrame(frameIndex));
    }
    return frames;
}

ovtr::DeviceDescriptor makeTestTracker(const std::string& serial)
{
    ovtr::DeviceDescriptor tracker;
    tracker.id = 1;
    tracker.runtimeIndex = 3;
    tracker.serial = serial;
    tracker.deviceClass = ovtr::DeviceClass::GenericTracker;
    return tracker;
}

ovtr::RecordingSession makeTestSession(
    const std::string& sessionId,
    const std::string& sessionName,
    const std::filesystem::path& framesPath,
    const std::filesystem::path& indexPath,
    const std::vector<ovtr::DeviceDescriptor>& devices
)
{
    ovtr::RecordingSession session;
    session.sessionId = sessionId;
    session.sessionName = sessionName;
    session.framesPath = framesPath;
    session.frameIndexPath = indexPath;
    session.devices = devices;
    return session;
}

void writeFrameSamples(
    const std::filesystem::path& framesPath,
    const std::filesystem::path& indexPath,
    const std::vector<ovtr::FrameSample>& frames,
    const std::string& errorPrefix
)
{
    ovtr::BinarySessionWriter writer;
    require(writer.open(framesPath, indexPath), errorPrefix + " writer open failed: " + writer.lastError());
    for (std::size_t frameIndex = 0; frameIndex < frames.size(); ++frameIndex) {
        require(
            writer.appendFrame(frames[frameIndex]),
            errorPrefix + " append frame " + std::to_string(frameIndex) + " failed: " + writer.lastError()
        );
    }
    writer.close();
}

void writeTestFrames(
    const std::filesystem::path& framesPath,
    const std::filesystem::path& indexPath,
    const std::uint64_t frameCount,
    const std::string& errorPrefix
)
{
    writeFrameSamples(framesPath, indexPath, makeTestFrames(frameCount), errorPrefix);
}

} // namespace ovtr::test
