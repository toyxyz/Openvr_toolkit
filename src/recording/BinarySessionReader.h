#pragma once

#include "data/SessionTypes.h"
#include "recording/BinarySessionFormat.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace ovtr {

class BinarySessionReader {
public:
    BinarySessionReader() = default;
    ~BinarySessionReader();

    BinarySessionReader(const BinarySessionReader&) = delete;
    BinarySessionReader& operator=(const BinarySessionReader&) = delete;

    bool open(const std::filesystem::path& framesPath, const std::filesystem::path& indexPath);
    void close();

    bool readFrame(std::uint64_t frameIndex, FrameSample& outFrame);
    std::uint64_t frameCount() const;
    const std::string& lastError() const;

private:
    bool readFrameHeader();
    bool readIndexFile(const std::filesystem::path& indexPath);
    bool readIndex(std::istream& indexFile);
    bool readFrameAtOffset(std::uint64_t byteOffset, FrameSample& outFrame);
    void setError(std::string message);

    std::ifstream frames_;
    std::vector<FrameIndexEntry> index_;
    std::string lastError_;
};

} // namespace ovtr
