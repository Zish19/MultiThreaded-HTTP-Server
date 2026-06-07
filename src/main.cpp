#include "Logger.h"
#include "Metrics.h"
#include "Config.h"
#include "ThreadPool.h"
#include "TcpServer.h"
#include "Middleware.h"
#include "FileCache.h"
#include "StaticFileHandler.h"
#include <iostream>

#if defined(_WIN32) || defined(_WIN64)
#include <winsock2.h>
#endif

int main() {
#if defined(_WIN32) || defined(_WIN64)
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed.\n";
        return 1;
    }
#endif

    Logger::getInstance().info("Starting Server Initialization...");
    
    std::uint16_t port = 8080;
    std::size_t threads = 4;
    std::size_t maxCacheSizeMb = 64;
    
    if (Config::getInstance().loadFromFile("config.json")) {
        port = Config::getInstance().getServerConfig().port;
        threads = Config::getInstance().getServerConfig().threadCount;
        maxCacheSizeMb = Config::getInstance().getMaxCacheSizeMb();
    }

    try {
        Router router;
        
        // Register Middleware
        router.use(BuiltInMiddleware::RequestIdMiddleware);
        router.use(BuiltInMiddleware::LoggingMiddleware);
        router.use(BuiltInMiddleware::MetricsMiddleware);
        
        FileCache cache(maxCacheSizeMb * 1024 * 1024);
        StaticFileHandler staticHandler(cache);
        
        // Static Files
        router.get("/", [&staticHandler](const HttpRequest& req) { return staticHandler.handle(req); });
        router.get("/index.html", [&staticHandler](const HttpRequest& req) { return staticHandler.handle(req); });
        router.get("/style.css", [&staticHandler](const HttpRequest& req) { return staticHandler.handle(req); });
        router.get("/app.js", [&staticHandler](const HttpRequest& req) { return staticHandler.handle(req); });
        router.get("/favicon.ico", [&staticHandler](const HttpRequest& req) { return staticHandler.handle(req); });
        
        router.get("/health", [](const HttpRequest&) {
            return HttpResponse::ok("OK");
        });
        
        router.get("/about", [](const HttpRequest&) {
            return HttpResponse::ok("Built with C++20");
        });
        
        router.get("/metrics", [](const HttpRequest&) {
            HttpResponse res = HttpResponse::ok(Metrics::getInstance().toJSON());
            res.addHeader("Content-Type", "application/json");
            return res;
        });

        ThreadPool pool(threads);
        TcpServer server(pool, router, port);
        
        server.start();
        Logger::getInstance().info("Server is running on port " + std::to_string(port) + ". Press Enter to stop.");
        
        std::cin.get();
        
        Logger::getInstance().info("Stopping server...");
        server.stop();
    } catch (const std::exception& e) {
        Logger::getInstance().error(std::string("Fatal error: ") + e.what());
    }
    
#if defined(_WIN32) || defined(_WIN64)
    WSACleanup();
#endif

    return 0;
}
