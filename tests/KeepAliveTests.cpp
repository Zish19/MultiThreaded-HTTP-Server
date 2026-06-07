#include "TcpServer.h"
#include "Socket.h"
#include "Router.h"
#include "ThreadPool.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "Config.h"
#include "Logger.h"
#include "Metrics.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>

#if defined(_WIN32) || defined(_WIN64)
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

void connectSocket(Socket& client, std::uint16_t port) {
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    
#if defined(_WIN32) || defined(_WIN64)
    if (::connect(client.handle(), reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) {
#else
    if (::connect(static_cast<int>(client.handle()), reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) {
#endif
        throw std::runtime_error("Failed to connect to test server");
    }
}
void setupServer(TcpServer& server, Router& router) {
    router.get("/", [](const HttpRequest&) {
        HttpResponse res;
        res.setStatusCode(200);
        res.setBody("OK");
        return res;
    });

    router.get("/health", [](const HttpRequest&) {
        HttpResponse res;
        res.setStatusCode(200);
        res.setBody("HEALTHY");
        return res;
    });

    server.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // wait for server to start
}

std::string sendAndReceive(Socket& client, const std::string& request) {
    client.sendAll(request);
    
    std::string response;
    try {
        while (true) {
            std::string chunk = client.receive(4096);
            if (chunk.empty()) break;
            response += chunk;
            if (response.find("OK") != std::string::npos || 
                response.find("HEALTHY") != std::string::npos ||
                response.find("400 Bad Request") != std::string::npos) {
                // Wait briefly to see if connection drops
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                break;
            }
        }
    } catch (...) {}
    return response;
}

void testPipelinedRequests(std::uint16_t port) {
    Socket client;
    connectSocket(client, port);
    
    std::string request = 
        "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n"
        "GET /health HTTP/1.1\r\nHost: localhost\r\n\r\n";
    
    client.sendAll(request);
    
    std::string response;
    int responsesReceived = 0;
    try {
        while (responsesReceived < 2) {
            std::string chunk = client.receive(4096);
            if (chunk.empty()) break;
            response += chunk;
            
            // Count responses by headers
            std::size_t pos = 0;
            responsesReceived = 0;
            while ((pos = response.find("HTTP/1.1 200 OK", pos)) != std::string::npos) {
                responsesReceived++;
                pos += 15;
            }
        }
    } catch (...) {}

    assert(responsesReceived == 2);
    assert(response.find("OK") != std::string::npos);
    assert(response.find("HEALTHY") != std::string::npos);
    client.close();
    std::cout << "[PASS] testPipelinedRequests\n";
}

void testKeepAliveLimit(std::uint16_t port) {
    Socket client;
    connectSocket(client, port);
    
    for (int i = 0; i < 100; ++i) {
        std::string request = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
        std::string response = sendAndReceive(client, request);
        assert(response.find("200 OK") != std::string::npos);
        if (i < 99) {
            assert(response.find("Connection: keep-alive") != std::string::npos);
        } else {
            assert(response.find("Connection: close") != std::string::npos);
        }
    }
    
    // 101st request should fail or socket should be closed
    try {
        std::string request = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
        client.sendAll(request);
        std::string response = client.receive(4096);
        assert(response.empty()); // server closed it
    } catch (...) {} // Send/Receive threw meaning closed
    
    client.close();
    std::cout << "[PASS] testKeepAliveLimit\n";
}

void testExplicitClose(std::uint16_t port) {
    Socket client;
    connectSocket(client, port);
    
    std::string request = "GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";
    std::string response = sendAndReceive(client, request);
    
    assert(response.find("200 OK") != std::string::npos);
    assert(response.find("Connection: close") != std::string::npos);
    
    try {
        std::string nextReq = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
        client.sendAll(nextReq);
        std::string res2 = client.receive(4096);
        assert(res2.empty());
    } catch (...) {}
    
    client.close();
    std::cout << "[PASS] testExplicitClose\n";
}

void testHttp10DefaultClose(std::uint16_t port) {
    Socket client;
    connectSocket(client, port);
    
    std::string request = "GET / HTTP/1.0\r\nHost: localhost\r\n\r\n";
    std::string response = sendAndReceive(client, request);
    
    assert(response.find("200 OK") != std::string::npos);
    assert(response.find("Connection: close") != std::string::npos);
    
    client.close();
    std::cout << "[PASS] testHttp10DefaultClose\n";
}

void testHttp10KeepAlive(std::uint16_t port) {
    Socket client;
    connectSocket(client, port);
    
    std::string request = "GET / HTTP/1.0\r\nHost: localhost\r\nConnection: keep-alive\r\n\r\n";
    std::string response = sendAndReceive(client, request);
    
    assert(response.find("200 OK") != std::string::npos);
    assert(response.find("Connection: keep-alive") != std::string::npos);
    
    client.close();
    std::cout << "[PASS] testHttp10KeepAlive\n";
}

void testTimeoutClosure(std::uint16_t port) {
    Socket client;
    connectSocket(client, port);
    
    std::string request = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
    std::string response = sendAndReceive(client, request);
    assert(response.find("200 OK") != std::string::npos);
    
    // Server timeout is 5s. Sleep 6s.
    std::this_thread::sleep_for(std::chrono::seconds(6));
    
    try {
        client.sendAll("GET / HTTP/1.1\r\nHost: localhost\r\n\r\n");
        std::string res2 = client.receive(4096);
        assert(res2.empty());
    } catch (...) {}
    
    client.close();
    std::cout << "[PASS] testTimeoutClosure\n";
}

int main() {
#if defined(_WIN32) || defined(_WIN64)
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    Logger::getInstance().setLogLevel(LogLevel::NONE);
    Config::getInstance().loadFromFile("config.json");
    
    ThreadPool threadPool(4);
    Router router;
    std::uint16_t port = 8092;
    TcpServer server(threadPool, router, port);
    
    setupServer(server, router);
    
    testPipelinedRequests(port);
    testExplicitClose(port);
    testHttp10DefaultClose(port);
    testHttp10KeepAlive(port);
    testKeepAliveLimit(port);
    // testTimeoutClosure(port); // Optional: disabled by default to avoid slow tests

    server.stop();
#if defined(_WIN32) || defined(_WIN64)
    WSACleanup();
#endif
    return 0;
}
