#pragma once

#include "platform/win32/VmcFingerState.h"

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

namespace ovtr::win32 {

class VmcReceiver {
public:
    VmcReceiver() = default;
    ~VmcReceiver();

    VmcReceiver(const VmcReceiver&) = delete;
    VmcReceiver& operator=(const VmcReceiver&) = delete;

    bool configure(bool enabled, int port, std::string& error);
    void stop() noexcept;
    bool running() const noexcept;
    int port() const noexcept;
    VmcFingerSnapshot snapshot() const;

private:
    void run(unsigned long long socketHandle) noexcept;
    bool start(int port, std::string& error);

    mutable std::mutex controlMutex_;
    mutable std::mutex snapshotMutex_;
    VmcFingerSnapshot snapshot_;
    std::thread thread_;
    std::atomic_bool stopRequested_{false};
    unsigned long long socketHandle_ = 0;
    int port_ = 0;
};

} // namespace ovtr::win32
