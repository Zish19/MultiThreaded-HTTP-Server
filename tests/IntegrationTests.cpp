#include "TcpServer.h"
#include "ThreadPool.h"
#include "Socket.h"
#include "Metrics.h"
#include "Router.h"
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

void testEndToEndGet() {
    Router router;
    router.get("/", [](const HttpRequest&) {
        return HttpResponse::ok("Hello World!");
    });
    ThreadPool pool(2);
    TcpServer server(pool, router, 8080);
    server.start();
    
    // Allow server to start up
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    Socket client;
    // We bind it automatically or connect
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);
    // 127.0.0.1
    serverAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    
    if (::connect(client.handle(), reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) {
        throw std::runtime_error("Failed to connect to test server");
    }

    std::string request = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
    client.sendAll(request);

    std::string response = client.receive(4096);
    
    // Verify 200 OK
    assert(response.find("HTTP/1.1 200 OK") != std::string::npos);
    assert(response.find("Content-Length: 12") != std::string::npos);
    assert(response.find("Hello World!") != std::string::npos);
    
    client.close();
    server.stop();
    
    std::cout << "[PASS] testEndToEndGet\n";
}

void testEndToEndPost() {
    Router router;
    router.post("/login", [](const HttpRequest&) { return HttpResponse::ok(); });
    ThreadPool pool(2);
    TcpServer server(pool, router, 8081);
    server.start();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    Socket client;
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8081);
    serverAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    
    if (::connect(client.handle(), reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) {
        throw std::runtime_error("Failed to connect to test server");
    }

    std::string request = "POST /login HTTP/1.1\r\nHost: localhost\r\nContent-Length: 5\r\n\r\nadmin";
    client.sendAll(request);

    std::string response = client.receive(4096);
    
    // Verify 200 OK
    assert(response.find("HTTP/1.1 200 OK") != std::string::npos);
    
    client.close();
    server.stop();
    
    std::cout << "[PASS] testEndToEndPost\n";
}

void testEndToEndFragmentation() {
    Router router;
    router.get("/", [](const HttpRequest&) { return HttpResponse::ok(); });
    ThreadPool pool(2);
    TcpServer server(pool, router, 8082);
    server.start();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    Socket client;
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8082);
    serverAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    
    if (::connect(client.handle(), reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) {
        throw std::runtime_error("Failed to connect to test server");
    }

    // Force TCP fragmentation by sending headers in pieces
    std::string part1 = "GET / HTTP/1.1\r\n";
    std::string part2 = "Host: localhost\r\n\r\n";
    
    client.sendAll(part1);
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // wait to ensure fragmentation
    client.sendAll(part2);

    std::string response = client.receive(4096);
    
    // Verify 200 OK
    assert(response.find("HTTP/1.1 200 OK") != std::string::npos);
    
    client.close();
    server.stop();
    
    std::cout << "[PASS] testEndToEndFragmentation\n";
}

void testEndToEndBadRequest() {
    Router router;
    ThreadPool pool(2);
    TcpServer server(pool, router, 8083);
    server.start();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    Socket client;
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8083);
    serverAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    
    if (::connect(client.handle(), reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) {
        throw std::runtime_error("Failed to connect to test server");
    }

    std::string request = "MALFORMED REQUEST\r\n\r\n";
    client.sendAll(request);

    std::string response = client.receive(4096);
    
    // Verify 400 Bad Request
    assert(response.find("HTTP/1.1 400 Bad Request") != std::string::npos);
    
    client.close();
    server.stop();
    
    std::cout << "[PASS] testEndToEndBadRequest\n";
}

int main() {
    // We must initialize WSAContext since we're using sockets directly
#if defined(_WIN32) || defined(_WIN64)
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    std::cout << "Starting Integration Tests...\n";
    
    testEndToEndGet();
    testEndToEndPost();
    testEndToEndFragmentation();
    testEndToEndBadRequest();
    
    std::cout << "All Integration tests passed successfully!\n";
    
#if defined(_WIN32) || defined(_WIN64)
    WSACleanup();
#endif
    return 0;
}
