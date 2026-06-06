#include "TcpServer.h"
#include "WSAContext.h"
#include "ThreadPool.h"
#include "Socket.h"
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
#include <arpa/inet.h>
#endif

void testTcpServerStartupAndShutdown() {
    Router router;
    ThreadPool pool(2);
    TcpServer server(pool, router, 8082);
    
    server.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    server.stop();
    
    std::cout << "[PASS] testTcpServerStartupAndShutdown\n";
}

void testTcpServerClientInteraction() {
    Router router;
    ThreadPool pool(2);
    TcpServer server(pool, router, 8083);
    server.start();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Create a client socket
    Socket client;
    
    // Since we don't have connect() in Socket yet, we can use raw sockets or add connect() to Socket.
    // For Phase 3, we just want to verify the server handles client connections.
    // Let's implement a quick connect using platform specific code just for the test.
    
#if defined(_WIN32) || defined(_WIN64)
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8083);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);
    
    int result = connect(client.handle(), reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr));
    assert(result == 0);
#else
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8083);
    serverAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    
    int result = connect(client.handle(), reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr));
    assert(result == 0);
#endif
    
    std::string testMsg = "MALFORMED HTTP REQUEST\r\n\r\n";
    client.send(testMsg);
    
    std::string response = client.receive(4096);
    assert(response.find("400 Bad Request") != std::string::npos);
    
    client.close();
    
    server.stop();
    std::cout << "[PASS] testTcpServerClientInteraction\n";
}

int main() {
    std::cout << "Starting TcpServer Tests...\n";
    
    try {
        WSAContext ctx;
        
        testTcpServerStartupAndShutdown();
        testTcpServerClientInteraction();
        
        std::cout << "All TcpServer tests passed successfully!\n";
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
