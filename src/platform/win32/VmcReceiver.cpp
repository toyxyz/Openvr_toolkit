#include "platform/win32/VmcReceiver.h"

#include "math/QuaternionUtils.h"
#include "platform/win32/VmcOscParser.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <exception>
#include <string>
#include <vector>

namespace ovtr::win32 {
namespace {

bool ensureWinsock(std::string& error)
{
    WSADATA data{};
    const int result = WSAStartup(MAKEWORD(2, 2), &data);
    if (result != 0) {
        error = "WSAStartup failed: " + std::to_string(result);
        return false;
    }
    return true;
}

std::string socketError(const char* operation)
{
    return std::string(operation) + " failed: WSA " + std::to_string(WSAGetLastError());
}

SOCKET socketFromHandle(const unsigned long long value) noexcept
{
    return static_cast<SOCKET>(value);
}

unsigned long long handleFromSocket(const SOCKET value) noexcept
{
    return static_cast<unsigned long long>(value);
}

} // namespace

VmcReceiver::~VmcReceiver()
{
    stop();
}

bool VmcReceiver::configure(const bool enabled, const int portValue, std::string& error)
{
    std::lock_guard<std::mutex> lock(controlMutex_);
    if (!enabled) {
        stop();
        return true;
    }
    if (running() && port_ == portValue) {
        return true;
    }
    stop();
    return start(portValue, error);
}

bool VmcReceiver::start(const int portValue, std::string& error)
{
    if (!ensureWinsock(error)) {
        return false;
    }

    const SOCKET receiverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (receiverSocket == INVALID_SOCKET) {
        error = socketError("socket");
        WSACleanup();
        return false;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(static_cast<u_short>(portValue));
    if (bind(receiverSocket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR) {
        error = socketError("bind");
        closesocket(receiverSocket);
        WSACleanup();
        return false;
    }

    stopRequested_.store(false, std::memory_order_release);
    socketHandle_ = handleFromSocket(receiverSocket);
    port_ = portValue;
    try {
        thread_ = std::thread([this, receiverSocket]() {
            run(handleFromSocket(receiverSocket));
        });
    } catch (const std::exception& exception) {
        error = std::string("VMC receiver thread start failed: ") + exception.what();
        closesocket(receiverSocket);
        socketHandle_ = 0;
        port_ = 0;
        WSACleanup();
        return false;
    }
    return true;
}

void VmcReceiver::stop() noexcept
{
    stopRequested_.store(true, std::memory_order_release);
    if (thread_.joinable()) {
        thread_.join();
    }
    if (socketHandle_ != 0) {
        closesocket(socketFromHandle(socketHandle_));
        socketHandle_ = 0;
        port_ = 0;
        WSACleanup();
    }
}

bool VmcReceiver::running() const noexcept
{
    return socketHandle_ != 0 && thread_.joinable();
}

int VmcReceiver::port() const noexcept
{
    return port_;
}

VmcFingerSnapshot VmcReceiver::snapshot() const
{
    std::lock_guard<std::mutex> lock(snapshotMutex_);
    return snapshot_;
}

void VmcReceiver::run(const unsigned long long socketHandle) noexcept
{
    const SOCKET receiverSocket = socketFromHandle(socketHandle);
    std::array<std::uint8_t, 8192> buffer{};
    while (!stopRequested_.load(std::memory_order_acquire)) {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(receiverSocket, &readSet);
        timeval timeout{0, 100000};
        const int selected = select(0, &readSet, nullptr, nullptr, &timeout);
        if (selected <= 0 || !FD_ISSET(receiverSocket, &readSet)) {
            continue;
        }
        const int received = recv(receiverSocket, reinterpret_cast<char*>(buffer.data()), static_cast<int>(buffer.size()), 0);
        if (received <= 0) {
            continue;
        }
        std::vector<VmcOscBonePose> bones;
        if (!parseVmcOscPacket(buffer.data(), static_cast<std::size_t>(received), bones)) {
            continue;
        }
        const auto now = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> lock(snapshotMutex_);
        for (const VmcOscBonePose& bone : bones) {
            ovtr::SkeletalHandSide side = ovtr::SkeletalHandSide::Left;
            std::uint32_t boneIndex = 0;
            if (!parseVmcFingerBoneName(bone.name, side, boneIndex) || boneIndex >= kVmcFingerNameBoneCount) {
                continue;
            }
            VmcFingerSideState& hand = snapshot_.hands[static_cast<std::size_t>(vmcSideIndex(side))];
            hand.positions[boneIndex] = bone.position;
            hand.rotations[boneIndex] = ovtr::normalizeQuaternion(bone.rotation);
            hand.valid[boneIndex] = true;
            hand.lastUpdate = now;
        }
    }
}

} // namespace ovtr::win32
