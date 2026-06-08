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
        // Shutting down the socket will unblock accept() on Linux
        m_serverSocket.shutdown();
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
        std::string requestBuffer;
        requestBuffer.reserve(HttpParser::MAX_TOTAL_HEADERS_SIZE);
        std::size_t bufferOffset = 0;
        bool keepAlive = true;
        std::uint64_t requestCount = 0;

        while (keepAlive && requestCount < 100) {
            bool headersComplete = false;
            std::size_t expectedTotalSize = 0;
            std::size_t headerEnd = 0;
            std::size_t separatorLen = 0;

            try {
                // Ingestion Loop
                while (true) {
                    std::string_view currentBuf(requestBuffer.data() + bufferOffset, requestBuffer.size() - bufferOffset);
                    
                    if (!headersComplete) {
                        headerEnd = currentBuf.find("\r\n\r\n");
                        separatorLen = 4;
                        if (headerEnd == std::string_view::npos) {
                            headerEnd = currentBuf.find("\n\n");
                            separatorLen = 2;
                        }

                        if (headerEnd != std::string_view::npos) {
                            headersComplete = true;
                            std::size_t bodySize = HttpParser::extractExpectedBodySize(currentBuf.substr(0, headerEnd));
                            expectedTotalSize = headerEnd + separatorLen + bodySize;
                        } else if (currentBuf.size() > HttpParser::MAX_TOTAL_HEADERS_SIZE) {
                            keepAlive = false;
                            break;
                        }
                    }

                    if (headersComplete && currentBuf.size() >= expectedTotalSize) {
                        break; // Full request assembled
                    }

                    std::string chunk = client.receive(4096);
                    if (chunk.empty()) {
                        keepAlive = false;
                        break;
                    }
                    requestBuffer += chunk;
                }

                if (!keepAlive && (requestBuffer.size() - bufferOffset) == 0) {
                    break; // Graceful close or timeout while idle
                }

                if (!keepAlive && !headersComplete) {
                    HttpResponse res = HttpResponse::badRequest();
                    res.addHeader("Connection", "close");
                    client.sendAll(res.toString());
                    break;
                }

                std::string_view currentBuf(requestBuffer.data() + bufferOffset, requestBuffer.size() - bufferOffset);
                std::string_view rawRequestView = currentBuf.substr(0, expectedTotalSize);

                Metrics::getInstance().addBytesReceived(rawRequestView.size());
                Logger::getInstance().info("Request received");

                HttpRequest req = HttpParser::parse(rawRequestView);
                Metrics::getInstance().incrementRequests();
                requestCount++;

                // Keep-Alive Logic
                std::string reqVersion = req.getVersion();
                auto connHeaderOpt = req.getHeader("connection");
                std::string connHeader = "";
                if (connHeaderOpt) {
                    connHeader = *connHeaderOpt;
                    for (char& c : connHeader) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                }

                if (reqVersion == "HTTP/1.0") {
                    keepAlive = (connHeader == "keep-alive");
                } else {
                    keepAlive = (connHeader != "close");
                }

                if (requestCount >= 100) {
                    keepAlive = false;
                }

                HttpResponse res = m_router.dispatch(req);

                if (keepAlive) {
                    res.addHeader("Connection", "keep-alive");
                } else {
                    res.addHeader("Connection", "close");
                }

                std::string resStr = res.toString();
                std::size_t sentBytes = client.sendAll(resStr);
                Metrics::getInstance().addBytesSent(sentBytes);
                Logger::getInstance().info("Response sent");

                bufferOffset += expectedTotalSize;
                
                if (bufferOffset > requestBuffer.size() / 2) {
                    requestBuffer.erase(0, bufferOffset);
                    bufferOffset = 0;
                }

            } catch (const HttpParseException& e) {
                Logger::getInstance().error(std::string("Parse error: ") + e.what());
                HttpResponse res = HttpResponse::badRequest();
                res.addHeader("Connection", "close");
                try {
                    std::size_t sentBytes = client.sendAll(res.toString());
                    Metrics::getInstance().addBytesSent(sentBytes);
                } catch (...) {}
                break;
            } catch (const std::exception& e) {
                if (requestBuffer.size() - bufferOffset == 0) {
                    break; // Silent close on timeout when idle
                }
                Logger::getInstance().error(std::string("Server error: ") + e.what());
                HttpResponse res = HttpResponse::internalServerError();
                res.addHeader("Connection", "close");
                try {
                    std::size_t sentBytes = client.sendAll(res.toString());
                    Metrics::getInstance().addBytesSent(sentBytes);
                } catch (...) {}
                break;
            }
        }

        if (requestCount > 0) {
            Metrics::getInstance().recordKeepAliveSession(requestCount);
        }

    } catch (...) {
        // Catch any unforeseen fatal exceptions
    }

    client.close();
    Logger::getInstance().info("Client disconnected");
}
