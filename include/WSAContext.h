#pragma once

class WSAContext {
public:
    WSAContext();
    ~WSAContext() noexcept;

    // Delete copy and move semantics
    WSAContext(const WSAContext&) = delete;
    WSAContext& operator=(const WSAContext&) = delete;
    WSAContext(WSAContext&&) = delete;
    WSAContext& operator=(WSAContext&&) = delete;
};
