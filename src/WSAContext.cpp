#include "WSAContext.h"
#include <stdexcept>
#include <string>

#if defined(_WIN32) || defined(_WIN64)
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#endif

WSAContext::WSAContext() {
#if defined(_WIN32) || defined(_WIN64)
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        throw std::runtime_error("WSAStartup failed with error: " + std::to_string(result));
    }
#endif
}

WSAContext::~WSAContext() noexcept {
#if defined(_WIN32) || defined(_WIN64)
    WSACleanup();
#endif
}
