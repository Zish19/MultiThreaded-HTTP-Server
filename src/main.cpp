#include "Logger.h"
#include "Metrics.h"
#include "Config.h"
#include <thread>
#include <chrono>

int main() {
    Logger::getInstance().info("Starting Server Initialization...");
    
    // Test Config
    if (Config::getInstance().loadFromFile("config.json")) {
        Logger::getInstance().info("Port: " + std::to_string(Config::getInstance().getServerConfig().port));
        Logger::getInstance().info("Threads: " + std::to_string(Config::getInstance().getServerConfig().threadCount));
    } else {
        Logger::getInstance().warn("Using default port: " + std::to_string(Config::getInstance().getServerConfig().port));
    }

    // Test Metrics
    Metrics::getInstance().incrementRequests();
    Metrics::getInstance().incrementActiveConnections();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Simulate some work
    
    Logger::getInstance().info("Current Metrics: \n" + Metrics::getInstance().toJSON());

    Metrics::getInstance().decrementActiveConnections();
    
    Logger::getInstance().info("Phase 1 initialization complete.");
    return 0;
}
