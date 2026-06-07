#include "platform/win32/VmcSender.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>

#include <cstdint>
#include <string>

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

sockaddr_in* addressFromHandle(const unsigned long long value) noexcept
{
    return reinterpret_cast<sockaddr_in*>(value);
}

unsigned long long handleFromAddress(sockaddr_in* address) noexcept
{
    return reinterpret_cast<unsigned long long>(address);
}

} // namespace

VmcSender::~VmcSender()
{
    stop();
}

bool VmcSender::configure(
    const bool enabled,
    const std::string& hostValue,
    const int portValue,
    std::string& error
)
{
    std::lock_guard<std::mutex> lock(controlMutex_);
    if (!enabled) {
        stop();
        return true;
    }
    if (running() && host_ == hostValue && port_ == portValue) {
        return true;
    }
    stop();
    return start(hostValue, portValue, error);
}

bool VmcSender::start(const std::string& hostValue, const int portValue, std::string& error)
{
    if (hostValue.empty()) {
        error = "VMC target host is empty";
        return false;
    }
    if (!ensureWinsock(error)) {
        return false;
    }
    const SOCKET senderSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (senderSocket == INVALID_SOCKET) {
        error = socketError("socket");
        WSACleanup();
        return false;
    }

    auto* address = new sockaddr_in{};
    address->sin_family = AF_INET;
    address->sin_port = htons(static_cast<u_short>(portValue));
    if (inet_pton(AF_INET, hostValue.c_str(), &address->sin_addr) != 1) {
        error = "Invalid VMC target IPv4 host: " + hostValue;
        delete address;
        closesocket(senderSocket);
        WSACleanup();
        return false;
    }

    socketHandle_ = handleFromSocket(senderSocket);
    targetAddress_ = handleFromAddress(address);
    host_ = hostValue;
    port_ = portValue;
    return true;
}

bool VmcSender::sendPacket(const std::vector<std::uint8_t>& packet, std::string& error)
{
    std::lock_guard<std::mutex> lock(controlMutex_);
    if (!running() || packet.empty()) {
        return true;
    }
    const sockaddr_in* address = addressFromHandle(targetAddress_);
    const int sent = sendto(
        socketFromHandle(socketHandle_),
        reinterpret_cast<const char*>(packet.data()),
        static_cast<int>(packet.size()),
        0,
        reinterpret_cast<const sockaddr*>(address),
        sizeof(sockaddr_in)
    );
    if (sent == SOCKET_ERROR) {
        error = socketError("sendto");
        return false;
    }
    return true;
}

void VmcSender::stop() noexcept
{
    if (socketHandle_ != 0) {
        closesocket(socketFromHandle(socketHandle_));
        socketHandle_ = 0;
        host_.clear();
        port_ = 0;
        WSACleanup();
    }
    if (targetAddress_ != 0) {
        delete addressFromHandle(targetAddress_);
        targetAddress_ = 0;
    }
}

bool VmcSender::running() const noexcept
{
    return socketHandle_ != 0 && targetAddress_ != 0;
}

const std::string& VmcSender::host() const noexcept
{
    return host_;
}

int VmcSender::port() const noexcept
{
    return port_;
}

} // namespace ovtr::win32
