#include "TcpServer.h"
#include "Logger.h"
#include "Metrics.h"
#include "HttpParser.h"
#include "HttpParseException.h"
#include "HttpResponse.h"
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

TcpServer::TcpServer(ThreadPool& threadPool, const Router& router, std::uint16_t port)
    : m_threadPool(threadPool), m_router(router), m_port(port) {
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
        std::string rawRequest;
        bool headersComplete = false;
        std::size_t expectedTotalSize = 0;

        // Ingestion Loop
        while (true) {
            std::string chunk = client.receive(4096);
            if (chunk.empty()) {
                break; // Client closed connection
            }
            rawRequest += chunk;

            if (!headersComplete) {
                std::size_t separatorLen = 4;
                std::size_t headerEnd = rawRequest.find("\r\n\r\n");
                if (headerEnd == std::string::npos) {
                    headerEnd = rawRequest.find("\n\n");
                    separatorLen = 2;
                }

                if (headerEnd != std::string::npos) {
                    headersComplete = true;
                    std::size_t bodySize = HttpParser::extractExpectedBodySize(rawRequest.substr(0, headerEnd));
                    expectedTotalSize = headerEnd + separatorLen + bodySize;
                }
            }

            if (headersComplete && rawRequest.size() >= expectedTotalSize) {
                break; // Full request assembled
            }
        }

        if (!rawRequest.empty()) {
            Metrics::getInstance().addBytesReceived(rawRequest.size());
            Logger::getInstance().info("Request received");

            HttpRequest req = HttpParser::parse(rawRequest);
            Metrics::getInstance().incrementRequests();

            HttpResponse res = m_router.dispatch(req);

            std::string resStr = res.toString();
            std::size_t sentBytes = client.sendAll(resStr);
            Metrics::getInstance().addBytesSent(sentBytes);
            Logger::getInstance().info("Response sent");
        }
    } catch (const HttpParseException& e) {
        Logger::getInstance().error(std::string("Parse error: ") + e.what());
        HttpResponse res = HttpResponse::badRequest();
        std::string resStr = res.toString();
        
        try {
            std::size_t sentBytes = client.sendAll(resStr);
            Metrics::getInstance().addBytesSent(sentBytes);
        } catch (...) {
            // Ignore send failures on error response
        }
    } catch (const std::exception& e) {
        Logger::getInstance().error(std::string("Server error: ") + e.what());
        HttpResponse res = HttpResponse::internalServerError();
        std::string resStr = res.toString();
        
        try {
            std::size_t sentBytes = client.sendAll(resStr);
            Metrics::getInstance().addBytesSent(sentBytes);
        } catch (...) {
            // Ignore send failures on error response
        }
    }

    client.close();
    Logger::getInstance().info("Client disconnected");
}
