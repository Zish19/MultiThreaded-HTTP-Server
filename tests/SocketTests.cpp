#include "WSAContext.h"
#include "Socket.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>

void testSocketCreation() {
    Socket s;
    assert(s.valid());
    std::cout << "[PASS] testSocketCreation\n";
}

void testSocketMoveSemantics() {
    Socket s1;
    assert(s1.valid());
    
    Socket s2(std::move(s1));
    assert(!s1.valid());
    assert(s2.valid());
    
    Socket s3;
    s3 = std::move(s2);
    assert(!s2.valid());
    assert(s3.valid());
    
    std::cout << "[PASS] testSocketMoveSemantics\n";
}

void testSocketBindListen() {
    Socket s;
    s.enableReuseAddress();
    s.bind(8081);
    s.listen(10);
    assert(s.valid());
    std::cout << "[PASS] testSocketBindListen\n";
}

int main() {
    std::cout << "Starting Socket Tests...\n";
    
    try {
        WSAContext ctx;
        
        testSocketCreation();
        testSocketMoveSemantics();
        testSocketBindListen();
        
        std::cout << "All Socket tests passed successfully!\n";
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
