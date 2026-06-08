#pragma once

#include <string>
#include <cstdint>

class Socket {
public:
    // Creates a new TCP IPv4 socket
    Socket();
    
    // Wraps an existing socket handle (e.g., from accept)
#if defined(_WIN32) || defined(_WIN64)
    using SocketHandle = std::uintptr_t;
#else
    using SocketHandle = int;
#endif

    static constexpr SocketHandle InvalidHandle = static_cast<SocketHandle>(~0);

    explicit Socket(SocketHandle handle) noexcept;

    ~Socket() noexcept;

    // Non-copyable
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    // Movable
    Socket(Socket&& other) noexcept;
    Socket& operator=(Socket&& other) noexcept;

    // Core networking operations
    void bind(std::uint16_t port);
    void listen(int backlog = 128); // SOMAXCONN equivalent
    Socket accept();
    
    // Options
    void enableReuseAddress();
    void setReceiveTimeout(int milliseconds);
    void setSendTimeout(int milliseconds);
    
    // I/O operations
    std::size_t send(const void* data, std::size_t size);
    std::size_t send(const std::string& data);
    std::size_t sendAll(const std::string& data);
    std::string receive(std::size_t maxBytes = 4096);
    
    void shutdown() noexcept;
    void close() noexcept;
    bool valid() const noexcept;
    SocketHandle handle() const noexcept;

private:
    SocketHandle m_handle;
};
