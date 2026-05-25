#pragma once

#include "data/SessionTypes.h"
#include "recording/BinarySessionFormat.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>

namespace ovtr {

class BinarySessionWriter {
public:
    BinarySessionWriter() = default;
    ~BinarySessionWriter();

    BinarySessionWriter(const BinarySessionWriter&) = delete;
    BinarySessionWriter& operator=(const BinarySessionWriter&) = delete;

    bool open(const std::filesystem::path& framesPath, const std::filesystem::path& indexPath);
    bool appendFrame(const FrameSample& frame);
    void close();

    bool isOpen() const;
    std::uint64_t frameCount() const;
    const std::string& lastError() const;

private:
    bool writeFrameHeader();
    bool writeIndexHeader();
    void setError(std::string message);

    std::ofstream frames_;
    std::ofstream index_;
    std::uint64_t frameCount_ = 0;
    std::string lastError_;
};

} // namespace ovtr

