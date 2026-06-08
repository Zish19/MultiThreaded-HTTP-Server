#include <iostream>
#include <chrono>
#include <string>
#include <vector>
#include "HttpParser.h"
#include "Router.h"
#include "ThreadPool.h"
#include "Logger.h"
#include "FileCache.h"
#include "StaticFileHandler.h"
#include <fstream>
#include <filesystem>

using namespace std::chrono;

void runParserBenchmarks() {
    std::cout << "\n--- Parser Benchmarks ---\n";
    
    std::string tinyRequest = "GET / HTTP/1.1\r\n\r\n"; // ~18 bytes
    
    std::string smallRequest = "GET /health HTTP/1.1\r\n"
                               "Host: localhost:8080\r\n"
                               "User-Agent: curl/7.68.0\r\n"
                               "Accept: */*\r\n"
                               "Connection: keep-alive\r\n\r\n"; // ~115 bytes
                               
    std::string mediumRequest = "POST /data HTTP/1.1\r\n"
                                "Host: localhost:8080\r\n"
                                "User-Agent: BenchmarkRunner/1.0\r\n"
                                "Accept: application/json\r\n"
                                "Content-Type: application/json\r\n"
                                "Content-Length: 3900\r\n";
    for (int i = 0; i < 50; ++i) {
        mediumRequest += "X-Custom-Header-" + std::to_string(i) + ": " + std::string(30, 'a') + "\r\n";
    }
    mediumRequest += "\r\n";
    mediumRequest += std::string(3900, 'x'); // ~ 6 KB total

    std::string largeRequest = "POST /upload HTTP/1.1\r\n"
                               "Host: localhost:8080\r\n"
                               "Content-Length: 64000\r\n\r\n";
    largeRequest += std::string(64000, 'x'); // ~ 64 KB total

    const int iterations = 100000;

    auto bench = [&](const std::string& name, const std::string& req, int iters) {
        auto start = high_resolution_clock::now();
        for (int i = 0; i < iters; ++i) {
            HttpRequest parsed = HttpParser::parse(req);
        }
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(end - start).count();
        std::cout << name << " (" << req.size() << " bytes) - " 
                  << iters << " iters: " << duration << " ms (" 
                  << (iters * 1000.0 / (duration > 0 ? duration : 1)) << " req/s)\n";
    };

    bench("Tiny Request", tinyRequest, iterations);
    bench("Small Request", smallRequest, iterations);
    bench("Medium Request", mediumRequest, 10000);
    bench("Large Request", largeRequest, 10000);
}

void runRouterBenchmarks() {
    std::cout << "\n--- Router Benchmarks ---\n";
    Router router;
    router.get("/health", [](const HttpRequest&) { return HttpResponse::ok("OK"); });
    router.get("/metrics", [](const HttpRequest&) { return HttpResponse::ok("{}"); });
    router.get("/about", [](const HttpRequest&) { return HttpResponse::ok("About"); });
    router.get("/", [](const HttpRequest&) { return HttpResponse::ok("Home"); });
    
    HttpRequest req;
    req.setMethod(HttpMethod::GET);
    req.setPath("/health");

    const int iterations = 1000000;
    auto start = high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        HttpResponse res = router.dispatch(req);
    }
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start).count();
    
    std::cout << "Router Dispatch - " << iterations << " iters: " 
              << duration << " ms (" 
              << (iterations * 1000.0 / (duration > 0 ? duration : 1)) << " dispatch/s)\n";
}

void runThreadPoolBenchmarks() {
    std::cout << "\n--- ThreadPool Benchmarks ---\n";
    const int numTasks = 100000;
    ThreadPool pool(4); // 4 workers
    
    std::atomic<int> counter{0};
    
    auto start = high_resolution_clock::now();
    for (int i = 0; i < numTasks; ++i) {
        pool.enqueue([&counter]() {
            counter.fetch_add(1, std::memory_order_relaxed);
            // Simulate tiny amount of work
            std::atomic<int> dummy{0};
            for(int j=0; j<10; ++j) dummy.fetch_add(1, std::memory_order_relaxed);
        });
    }
    
    // Wait for completion
    while (pool.completedTasks() < numTasks) {
        std::this_thread::yield();
    }
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start).count();
    
    std::cout << "ThreadPool Enqueue/Dequeue - " << numTasks << " tasks: " 
              << duration << " ms (" 
              << (numTasks * 1000.0 / (duration > 0 ? duration : 1)) << " tasks/s)\n";
    std::cout << "Max Observed Queue Depth: " << pool.maxObservedQueueDepth() << "\n";
}

void runStaticFileBenchmarks() {
    std::cout << "\n--- Static File Server Benchmarks ---\n";
    std::filesystem::create_directories("./public");
    std::string testFile = "public/benchmark.html";
    std::ofstream out(testFile, std::ios::binary);
    std::string content(10 * 1024, 'A'); // 10 KB file
    out << content;
    out.close();

    HttpRequest req;
    req.setMethod(HttpMethod::GET);
    req.setPath("/benchmark.html");

    const int iterations = 10000;

    auto bench = [&](const std::string& name, FileCache& cache) {
        StaticFileHandler handler(cache, "./public");
        auto start = high_resolution_clock::now();
        for (int i = 0; i < iterations; ++i) {
            HttpResponse res = handler.handle(req);
        }
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(end - start).count();
        std::cout << name << " - " << iterations << " requests: " 
                  << duration << " ms (" 
                  << (iterations * 1000.0 / (duration > 0 ? duration : 1)) << " req/s)\n";
    };

    FileCache coldCache(0); // 0 bytes max size -> forces cache miss every time
    bench("Without Cache (Cold)", coldCache);

    FileCache warmCache(64 * 1024 * 1024); // 64 MB max size -> cache hit every time
    // pre-warm
    {
        StaticFileHandler warmup(warmCache, "./public");
        warmup.handle(req);
    }
    bench("With Cache (Warm)", warmCache);

    std::filesystem::remove(testFile);
}

#include "TcpServer.h"
#include "Socket.h"

#if defined(_WIN32) || defined(_WIN64)
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

namespace {
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
}


void runKeepAliveBenchmarks() {
    std::cout << "\n--- Keep-Alive Benchmarks ---\n";

    ThreadPool pool(4);
    Router router;
    router.get("/health", [](const HttpRequest&) {
        HttpResponse res;
        res.setStatusCode(200);
        res.setBody("OK");
        return res;
    });

    std::uint16_t port = 8095;
    TcpServer server(pool, router, port);
    server.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    const int ITERATIONS = 1000; // lower to 1000 to be safe
    std::string request = "GET /health HTTP/1.1\r\nHost: localhost\r\n\r\n";

    auto bench = [&](bool keepAlive) {
        auto start = std::chrono::high_resolution_clock::now();
        if (keepAlive) {
            Socket client;
            connectSocket(client, port);
            for (int i = 0; i < ITERATIONS; ++i) {
                client.sendAll(request);
                std::string response;
                while (response.find("OK") == std::string::npos) {
                    response += client.receive(4096);
                }
            }
            client.close();
        } else {
            for (int i = 0; i < ITERATIONS; ++i) {
                Socket client;
                connectSocket(client, port);
                client.sendAll("GET /health HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n");
                std::string response;
                while (response.find("OK") == std::string::npos) {
                    std::string chunk = client.receive(4096);
                    if (chunk.empty()) break;
                    response += chunk;
                }
                client.close();
            }
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        double reqPerSec = (static_cast<double>(ITERATIONS) / durationMs) * 1000.0;
        
        std::cout << (keepAlive ? "With Keep-Alive" : "Without Keep-Alive") 
                  << " - " << ITERATIONS << " requests: " 
                  << durationMs << " ms (" << reqPerSec << " req/s)\n";
        return reqPerSec;
    };

    try {
        double rpsWithout = bench(false);
        double rpsWith = bench(true);

        if (rpsWithout > 0) {
            double improvement = ((rpsWith - rpsWithout) / rpsWithout) * 100.0;
            std::cout << "Improvement: " << improvement << "%\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "Benchmark failed: " << e.what() << "\n";
    }

    server.stop();
}

int main() {
#if defined(_WIN32) || defined(_WIN64)
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    Logger::getInstance().setLogLevel(LogLevel::NONE); // Disable logs for benchmarking
    
    std::cout << "Starting Benchmarks...\n";
    runParserBenchmarks();
    runRouterBenchmarks();
    runThreadPoolBenchmarks();
    runStaticFileBenchmarks();
    runKeepAliveBenchmarks();
    std::cout << "\nBenchmarks Complete.\n";
    
#if defined(_WIN32) || defined(_WIN64)
    WSACleanup();
#endif
    return 0;
}
