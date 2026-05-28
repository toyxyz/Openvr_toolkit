#include "recording/BinarySessionWriter.h"

#include "util/BinaryBuffer.h"

#include <utility>

namespace ovtr {

BinarySessionWriter::~BinarySessionWriter()
{
    close();
}

bool BinarySessionWriter::open(const std::filesystem::path& framesPath, const std::filesystem::path& indexPath)
{
    close();
    lastError_.clear();

    std::string directoryError;
    if (!ensureParentDirectory(framesPath, &directoryError)) {
        setError("failed to create frame directory: " + directoryError);
        return false;
    }

    if (!ensureParentDirectory(indexPath, &directoryError)) {
        setError("failed to create index directory: " + directoryError);
        return false;
    }

    frames_.open(framesPath, std::ios::binary | std::ios::trunc);
    if (!frames_.is_open()) {
        setError("failed to open frame file");
        return false;
    }

    index_.open(indexPath, std::ios::binary | std::ios::trunc);
    if (!index_.is_open()) {
        setError("failed to open index file");
        close();
        return false;
    }

    frameCount_ = 0;
    return writeFrameHeader() && writeIndexHeader();
}

void BinarySessionWriter::close()
{
    if (frames_.is_open()) {
        frames_.flush();
        frames_.close();
    }

    if (index_.is_open()) {
        index_.flush();
        index_.close();
    }
}

bool BinarySessionWriter::isOpen() const
{
    return frames_.is_open() && index_.is_open();
}

std::uint64_t BinarySessionWriter::frameCount() const
{
    return frameCount_;
}

const std::string& BinarySessionWriter::lastError() const
{
    return lastError_;
}

void BinarySessionWriter::setError(std::string message)
{
    lastError_ = std::move(message);
}

} // namespace ovtr
