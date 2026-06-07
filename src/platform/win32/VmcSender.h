#pragma once

#include <mutex>
#include <cstdint>
#include <string>
#include <vector>

namespace ovtr::win32 {

class VmcSender {
public:
    VmcSender() = default;
    ~VmcSender();

    VmcSender(const VmcSender&) = delete;
    VmcSender& operator=(const VmcSender&) = delete;

    bool configure(bool enabled, const std::string& host, int port, std::string& error);
    bool sendPacket(const std::vector<std::uint8_t>& packet, std::string& error);
    void stop() noexcept;
    bool running() const noexcept;
    const std::string& host() const noexcept;
    int port() const noexcept;

private:
    bool start(const std::string& host, int port, std::string& error);

    mutable std::mutex controlMutex_;
    unsigned long long socketHandle_ = 0;
    unsigned long long targetAddress_ = 0;
    std::string host_;
    int port_ = 0;
};

} // namespace ovtr::win32
