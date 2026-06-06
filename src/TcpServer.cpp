#include "TcpServer.h"
#include "Logger.h"
#include "Metrics.h"
#include <stdexcept>
#include <iostream>

class ActiveConnectionGuard {
public:
    ActiveConnectionGuard() {
        Metrics::getInstance().incrementTotalConnections();
        Metrics::getInstance().incrementActiveConnections();
    }
    ~ActiveConnectionGuard() {
        Metrics::getInstance().decrementActiveConnections();
    }
};

TcpServer::TcpServer(ThreadPool& threadPool, std::uint16_t port)
    : m_threadPool(threadPool), m_port(port) {
}

TcpServer::~TcpServer() noexcept {
    stop();
}

void TcpServer::start() {
    m_serverSocket.enableReuseAddress();
    m_serverSocket.bind(m_port);
    m_serverSocket.listen();
    
    Logger::getInstance().info("Server started on port " + std::to_string(m_port));
    
    m_stop.store(false, std::memory_order_release);
    m_acceptThread = std::thread(&TcpServer::acceptLoop, this);
}

void TcpServer::stop() noexcept {
    if (!m_stop.exchange(true, std::memory_order_acq_rel)) {
        // Closing the server socket will interrupt any blocking accept() call
        m_serverSocket.close();
        
        if (m_acceptThread.joinable()) {
            m_acceptThread.join();
        }
    }
}

void TcpServer::acceptLoop() {
    while (!m_stop.load(std::memory_order_acquire)) {
        try {
            Socket client = m_serverSocket.accept();
            client.setReceiveTimeout(5000); // 5 seconds timeout
            client.setSendTimeout(5000);    // 5 seconds timeout
            
            if (m_stop.load(std::memory_order_acquire)) {
                break;
            }

            m_threadPool.enqueue(
                [this, client = std::move(client)]() mutable {
                    handleClient(std::move(client));
                }
            );
        } catch (const std::exception& e) {
            if (!m_stop.load(std::memory_order_acquire)) {
                Logger::getInstance().error(std::string("Accept error: ") + e.what());
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    }
}

void TcpServer::handleClient(Socket client) {
    ActiveConnectionGuard connectionGuard;
    Logger::getInstance().info("Client connected");

    try {
        // Simple Phase 3 interaction: Receive, Log, Respond
        std::string receivedData = client.receive();
        Metrics::getInstance().addBytesReceived(receivedData.size());
        
        Logger::getInstance().info("Bytes received: " + std::to_string(receivedData.size()));

        std::string response = "Server received: " + receivedData;
        std::size_t sentBytes = client.send(response);
        Metrics::getInstance().addBytesSent(sentBytes);
        
        Logger::getInstance().info("Bytes sent: " + std::to_string(sentBytes));

    } catch (const std::exception& e) {
        Logger::getInstance().error(std::string("Client error: ") + e.what());
    }

    Logger::getInstance().info("Client disconnected");
}
