#include "recording/BinarySessionWriter.h"

#include "recording/BinaryIO.h"

namespace ovtr {

bool BinarySessionWriter::appendFrame(const FrameSample& frame)
{
    if (!isOpen()) {
        setError("writer is not open");
        return false;
    }

    const auto byteOffsetPosition = frames_.tellp();
    if (byteOffsetPosition < std::streampos(0)) {
        setError("failed to query frame file offset");
        return false;
    }

    const auto byteOffset = static_cast<std::uint64_t>(byteOffsetPosition - std::streampos(0));
    if (!writeFrameRecord(frame)) {
        return false;
    }

    const FrameIndexEntry entry{frame.frameIndex, frame.timestampNs, byteOffset};
    if (!writeFrameIndexEntry(entry)) {
        return false;
    }

    ++frameCount_;
    return true;
}

bool BinarySessionWriter::writeFrameRecord(const FrameSample& frame)
{
    const auto poseCount = static_cast<std::uint32_t>(frame.poses.size());
    const std::uint32_t frameFlags = 0;

    if (!writeBinaryValue(frames_, frame.frameIndex) ||
        !writeBinaryValue(frames_, frame.timestampNs) ||
        !writeBinaryValue(frames_, frame.timeSeconds) ||
        !writeBinaryValue(frames_, poseCount) ||
        !writeBinaryValue(frames_, frameFlags)) {
        setError("failed to write frame record header");
        return false;
    }

    for (const PoseSample& pose : frame.poses) {
        if (!writeBinaryValue(frames_, pose.deviceId) ||
            !writeBinaryValue(frames_, pose.runtimeIndex) ||
            !writeBinaryArray(frames_, pose.position) ||
            !writeBinaryArray(frames_, pose.rotation) ||
            !writeBinaryArray(frames_, pose.velocity) ||
            !writeBinaryArray(frames_, pose.angularVelocity) ||
            !writeBinaryValue(frames_, pose.flags)) {
            setError("failed to write pose record");
            return false;
        }
    }

    return true;
}

bool BinarySessionWriter::writeFrameIndexEntry(const FrameIndexEntry& entry)
{
    if (!writeBinaryValue(index_, entry.frameIndex) ||
        !writeBinaryValue(index_, entry.timestampNs) ||
        !writeBinaryValue(index_, entry.byteOffset)) {
        setError("failed to write frame index entry");
        return false;
    }

    return true;
}

} // namespace ovtr
