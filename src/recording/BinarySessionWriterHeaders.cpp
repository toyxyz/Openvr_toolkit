#include "recording/BinarySessionWriter.h"

#include "recording/BinaryIO.h"

namespace ovtr {

bool BinarySessionWriter::writeFrameHeader()
{
    const FrameFileHeader header;
    if (!writeBinaryArray(frames_, header.magic) ||
        !writeBinaryValue(frames_, header.formatVersion) ||
        !writeBinaryValue(frames_, header.endian) ||
        !writeBinaryValue(frames_, header.headerSize) ||
        !writeBinaryValue(frames_, header.reserved)) {
        setError("failed to write frame file header");
        return false;
    }

    return true;
}

bool BinarySessionWriter::writeIndexHeader()
{
    const FrameIndexHeader header;
    if (!writeBinaryArray(index_, header.magic) ||
        !writeBinaryValue(index_, header.formatVersion) ||
        !writeBinaryValue(index_, header.entrySize)) {
        setError("failed to write frame index header");
        return false;
    }

    return true;
}

} // namespace ovtr
