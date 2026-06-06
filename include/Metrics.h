#pragma once

#include <atomic>
#include <chrono>
#include <string>
#include <cstdint>

class Metrics {
public:
    static Metrics& getInstance();

    Metrics(const Metrics&) = delete;
    Metrics& operator=(const Metrics&) = delete;

    void incrementRequests();
    void incrementActiveConnections();
    void decrementActiveConnections();

    std::uint64_t getTotalRequests() const;
    std::int32_t getActiveConnections() const;
    double getUptimeSeconds() const;
    
    std::string toJSON() const;

private:
    Metrics();
    ~Metrics() = default;

    std::atomic<std::uint64_t> m_totalRequests{0};
    std::atomic<std::int32_t> m_activeConnections{0};
    std::chrono::time_point<std::chrono::steady_clock> m_startTime;
};
