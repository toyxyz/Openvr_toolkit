#include "recording/BinarySessionReader.h"

#include <algorithm>
#include <utility>

namespace ovtr {

BinarySessionReader::~BinarySessionReader()
{
    close();
}

bool BinarySessionReader::open(const std::filesystem::path& framesPath, const std::filesystem::path& indexPath)
{
    close();
    lastError_.clear();

    frames_.open(framesPath, std::ios::binary);
    if (!frames_.is_open()) {
        setError("failed to open frame file");
        return false;
    }

    if (!readFrameHeader() || !readIndexFile(indexPath)) {
        close();
        return false;
    }

    return true;
}

void BinarySessionReader::close()
{
    if (frames_.is_open()) {
        frames_.close();
    }
    index_.clear();
}

bool BinarySessionReader::readFrame(const std::uint64_t frameIndex, FrameSample& outFrame)
{
    const auto found = std::lower_bound(
        index_.begin(),
        index_.end(),
        frameIndex,
        [](const FrameIndexEntry& entry, const std::uint64_t value) {
            return entry.frameIndex < value;
        }
    );

    if (found == index_.end() || found->frameIndex != frameIndex) {
        setError("frame index not found");
        return false;
    }

    return readFrameAtOffset(found->byteOffset, outFrame);
}

std::uint64_t BinarySessionReader::frameCount() const
{
    return static_cast<std::uint64_t>(index_.size());
}

const std::string& BinarySessionReader::lastError() const
{
    return lastError_;
}

void BinarySessionReader::setError(std::string message)
{
    lastError_ = std::move(message);
}

} // namespace ovtr
