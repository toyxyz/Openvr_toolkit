#pragma once

#include <array>
#include <cstddef>
#include <istream>
#include <ostream>

namespace ovtr {

template <typename T>
bool writeBinaryValue(std::ostream& stream, const T& value)
{
    stream.write(reinterpret_cast<const char*>(&value), static_cast<std::streamsize>(sizeof(T)));
    return stream.good();
}

template <typename T>
bool readBinaryValue(std::istream& stream, T& value)
{
    stream.read(reinterpret_cast<char*>(&value), static_cast<std::streamsize>(sizeof(T)));
    return stream.good();
}

template <typename T, std::size_t Size>
bool writeBinaryArray(std::ostream& stream, const std::array<T, Size>& values)
{
    stream.write(
        reinterpret_cast<const char*>(values.data()),
        static_cast<std::streamsize>(sizeof(T) * values.size())
    );
    return stream.good();
}

template <typename T, std::size_t Size>
bool readBinaryArray(std::istream& stream, std::array<T, Size>& values)
{
    stream.read(
        reinterpret_cast<char*>(values.data()),
        static_cast<std::streamsize>(sizeof(T) * values.size())
    );
    return stream.good();
}

} // namespace ovtr

