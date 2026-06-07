#include <iostream>
#include <chrono>
#include <string>
#include <vector>
#include "HttpParser.h"
#include "Router.h"
#include "ThreadPool.h"
#include "Logger.h"

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
            int volatile dummy = 0;
            for(int j=0; j<10; ++j) dummy++;
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

int main() {
    Logger::getInstance().setLogLevel(LogLevel::NONE); // Disable logs for benchmarking
    
    std::cout << "Starting Benchmarks...\n";
    runParserBenchmarks();
    runRouterBenchmarks();
    runThreadPoolBenchmarks();
    std::cout << "\nBenchmarks Complete.\n";
    
    return 0;
}
