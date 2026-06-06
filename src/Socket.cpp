#include "Socket.h"
#include <stdexcept>
#include <vector>

#if defined(_WIN32) || defined(_WIN64)
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

Socket::Socket() : m_handle(InvalidHandle) {
    m_handle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_handle == InvalidHandle) {
        throw std::runtime_error("Failed to create socket");
    }
}

Socket::Socket(SocketHandle handle) noexcept : m_handle(handle) {}

Socket::~Socket() noexcept {
    close();
}

Socket::Socket(Socket&& other) noexcept : m_handle(other.m_handle) {
    other.m_handle = InvalidHandle;
}

Socket& Socket::operator=(Socket&& other) noexcept {
    if (this != &other) {
        close();
        m_handle = other.m_handle;
        other.m_handle = InvalidHandle;
    }
    return *this;
}

void Socket::enableReuseAddress() {
    int opt = 1;
    if (setsockopt(m_handle, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt)) < 0) {
        throw std::runtime_error("Failed to set SO_REUSEADDR on socket");
    }
}

void Socket::bind(std::uint16_t port) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (::bind(m_handle, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        throw std::runtime_error("Failed to bind socket to port " + std::to_string(port));
    }
}

void Socket::listen(int backlog) {
    if (::listen(m_handle, backlog) < 0) {
        throw std::runtime_error("Failed to listen on socket");
    }
}

Socket Socket::accept() {
    sockaddr_in clientAddr{};
    int clientAddrLen = sizeof(clientAddr);

    SocketHandle clientHandle = ::accept(m_handle, reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrLen);
    if (clientHandle == InvalidHandle) {
        throw std::runtime_error("Failed to accept client connection");
    }

    return Socket(clientHandle);
}

std::size_t Socket::send(const void* data, std::size_t size) {
    int bytesSent = ::send(m_handle, static_cast<const char*>(data), static_cast<int>(size), 0);
    if (bytesSent < 0) {
        throw std::runtime_error("Failed to send data");
    }
    return static_cast<std::size_t>(bytesSent);
}

std::size_t Socket::send(const std::string& data) {
    return send(data.data(), data.size());
}

std::string Socket::receive(std::size_t maxBytes) {
    std::vector<char> buffer(maxBytes);
    int bytesReceived = ::recv(m_handle, buffer.data(), static_cast<int>(maxBytes), 0);
    
    if (bytesReceived < 0) {
        throw std::runtime_error("Failed to receive data");
    }
    
    return std::string(buffer.data(), static_cast<std::size_t>(bytesReceived));
}

void Socket::close() noexcept {
    if (m_handle != InvalidHandle) {
#if defined(_WIN32) || defined(_WIN64)
        closesocket(m_handle);
#else
        ::close(m_handle);
#endif
        m_handle = InvalidHandle;
    }
}

bool Socket::valid() const noexcept {
    return m_handle != InvalidHandle;
}

Socket::SocketHandle Socket::handle() const noexcept {
    return m_handle;
}
