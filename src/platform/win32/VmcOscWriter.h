#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ovtr::win32 {

struct VmcOscTransform {
    std::array<float, 3> position{0.0f, 0.0f, 0.0f};
    std::array<float, 4> rotation{0.0f, 0.0f, 0.0f, 1.0f};
};

class VmcOscWriter {
public:
    struct Message {
        std::string address;
        std::string types;
        std::vector<std::uint8_t> arguments;
    };

    void addStatus(int loaded);
    void addTime(float timeSeconds);
    void addRootTransform(const std::string& name, const VmcOscTransform& transform);
    void addBoneTransform(const std::string& name, const VmcOscTransform& transform);

    std::vector<std::uint8_t> makeBundle() const;
    std::vector<std::vector<std::uint8_t>> makeBundles(std::size_t maxBundleBytes) const;

private:
    void addTransformMessage(
        const char* address,
        const std::string& name,
        const VmcOscTransform& transform
    );

    std::vector<Message> messages_;
};

} // namespace ovtr::win32
