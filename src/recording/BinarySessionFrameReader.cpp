#include "recording/BinarySessionReader.h"

#include "recording/BinaryIO.h"

namespace ovtr {
namespace {

constexpr std::uint32_t kMaxFramePoseCount = 256;

} // namespace

bool BinarySessionReader::readFrameAtOffset(const std::uint64_t byteOffset, FrameSample& outFrame)
{
    if (!frames_.is_open()) {
        setError("reader is not open");
        return false;
    }

    frames_.clear();
    frames_.seekg(static_cast<std::streamoff>(byteOffset), std::ios::beg);
    if (!frames_.good()) {
        setError("failed to seek frame file");
        return false;
    }

    std::uint32_t poseCount = 0;
    std::uint32_t frameFlags = 0;
    if (!readBinaryValue(frames_, outFrame.frameIndex) ||
        !readBinaryValue(frames_, outFrame.timestampNs) ||
        !readBinaryValue(frames_, outFrame.timeSeconds) ||
        !readBinaryValue(frames_, poseCount) ||
        !readBinaryValue(frames_, frameFlags)) {
        setError("failed to read frame record header");
        return false;
    }

    if (poseCount > kMaxFramePoseCount) {
        setError("unreasonable pose count in frame record");
        return false;
    }

    outFrame.poses.clear();
    outFrame.poses.reserve(poseCount);
    for (std::uint32_t i = 0; i < poseCount; ++i) {
        PoseSample pose;
        if (!readBinaryValue(frames_, pose.deviceId) ||
            !readBinaryValue(frames_, pose.runtimeIndex) ||
            !readBinaryArray(frames_, pose.position) ||
            !readBinaryArray(frames_, pose.rotation) ||
            !readBinaryArray(frames_, pose.velocity) ||
            !readBinaryArray(frames_, pose.angularVelocity) ||
            !readBinaryValue(frames_, pose.flags)) {
            setError("failed to read pose record");
            return false;
        }
        outFrame.poses.push_back(pose);
    }

    return true;
}

} // namespace ovtr
