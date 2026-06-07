#include "platform/win32/VmcOscWriter.h"

#include <cstring>
#include <utility>

namespace ovtr::win32 {
namespace {

void appendPaddedString(std::vector<std::uint8_t>& out, const std::string& value)
{
    out.insert(out.end(), value.begin(), value.end());
    out.push_back(0);
    while ((out.size() % 4U) != 0U) {
        out.push_back(0);
    }
}

void appendInt32(std::vector<std::uint8_t>& out, const std::int32_t value)
{
    out.push_back(static_cast<std::uint8_t>((value >> 24) & 0xff));
    out.push_back(static_cast<std::uint8_t>((value >> 16) & 0xff));
    out.push_back(static_cast<std::uint8_t>((value >> 8) & 0xff));
    out.push_back(static_cast<std::uint8_t>(value & 0xff));
}

void appendFloat32(std::vector<std::uint8_t>& out, const float value)
{
    static_assert(sizeof(float) == sizeof(std::uint32_t));
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    appendInt32(out, static_cast<std::int32_t>(bits));
}

std::vector<std::uint8_t> makeMessageBytes(const VmcOscWriter::Message& message)
{
    std::vector<std::uint8_t> bytes;
    appendPaddedString(bytes, message.address);
    appendPaddedString(bytes, "," + message.types);
    bytes.insert(bytes.end(), message.arguments.begin(), message.arguments.end());
    return bytes;
}

void appendBundleHeader(std::vector<std::uint8_t>& out)
{
    appendPaddedString(out, "#bundle");
    for (int i = 0; i < 7; ++i) {
        out.push_back(0);
    }
    out.push_back(1);
}

void appendBundleMessage(
    std::vector<std::uint8_t>& out,
    const std::vector<std::uint8_t>& bytes
) {
    appendInt32(out, static_cast<std::int32_t>(bytes.size()));
    out.insert(out.end(), bytes.begin(), bytes.end());
}

void appendTransformArgs(
    std::vector<std::uint8_t>& out,
    const std::string& name,
    const VmcOscTransform& transform
) {
    appendPaddedString(out, name);
    appendFloat32(out, transform.position[0]);
    appendFloat32(out, transform.position[1]);
    appendFloat32(out, transform.position[2]);
    appendFloat32(out, transform.rotation[0]);
    appendFloat32(out, transform.rotation[1]);
    appendFloat32(out, transform.rotation[2]);
    appendFloat32(out, transform.rotation[3]);
}

} // namespace

void VmcOscWriter::addStatus(const int loaded)
{
    Message message;
    message.address = "/VMC/Ext/OK";
    message.types = "i";
    appendInt32(message.arguments, loaded);
    messages_.push_back(std::move(message));
}

void VmcOscWriter::addTime(const float timeSeconds)
{
    Message message;
    message.address = "/VMC/Ext/T";
    message.types = "f";
    appendFloat32(message.arguments, timeSeconds);
    messages_.push_back(std::move(message));
}

void VmcOscWriter::addRootTransform(const std::string& name, const VmcOscTransform& transform)
{
    addTransformMessage("/VMC/Ext/Root/Pos", name, transform);
}

void VmcOscWriter::addBoneTransform(const std::string& name, const VmcOscTransform& transform)
{
    addTransformMessage("/VMC/Ext/Bone/Pos", name, transform);
}

void VmcOscWriter::addTransformMessage(
    const char* address,
    const std::string& name,
    const VmcOscTransform& transform
) {
    Message message;
    message.address = address;
    message.types = "sfffffff";
    appendTransformArgs(message.arguments, name, transform);
    messages_.push_back(std::move(message));
}

std::vector<std::uint8_t> VmcOscWriter::makeBundle() const
{
    std::vector<std::uint8_t> out;
    appendBundleHeader(out);
    for (const Message& message : messages_) {
        const std::vector<std::uint8_t> bytes = makeMessageBytes(message);
        appendBundleMessage(out, bytes);
    }
    return out;
}

std::vector<std::vector<std::uint8_t>> VmcOscWriter::makeBundles(
    const std::size_t maxBundleBytes
) const {
    std::vector<std::vector<std::uint8_t>> bundles;
    std::vector<std::uint8_t> current;
    std::size_t currentMessages = 0;
    appendBundleHeader(current);

    for (const Message& message : messages_) {
        const std::vector<std::uint8_t> bytes = makeMessageBytes(message);
        const std::size_t entryBytes = sizeof(std::int32_t) + bytes.size();
        if (currentMessages > 0 && current.size() + entryBytes > maxBundleBytes) {
            bundles.push_back(std::move(current));
            current.clear();
            currentMessages = 0;
            appendBundleHeader(current);
        }
        appendBundleMessage(current, bytes);
        ++currentMessages;
    }

    if (currentMessages > 0 || messages_.empty()) {
        bundles.push_back(std::move(current));
    }
    return bundles;
}

} // namespace ovtr::win32
