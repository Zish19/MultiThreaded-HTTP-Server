#pragma once

#include "Socket.h"
#include "ThreadPool.h"
#include "Router.h"
#include <atomic>
#include <cstdint>
#include <thread>

class TcpServer {
public:
    TcpServer(ThreadPool& threadPool, const Router& router, std::uint16_t port);
    ~TcpServer() noexcept;

    // Delete copy and move
    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;
    TcpServer(TcpServer&&) = delete;
    TcpServer& operator=(TcpServer&&) = delete;

    void start();
    void stop() noexcept;

private:
    void acceptLoop();
    void handleClient(Socket client);

private:
    ThreadPool& m_threadPool;
    const Router& m_router;
    std::uint16_t m_port;
    Socket m_serverSocket;
    
    std::atomic<bool> m_stop{false};
    std::thread m_acceptThread;
};
