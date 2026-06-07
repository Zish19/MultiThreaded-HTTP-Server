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

    void incrementTotalConnections();
    void addBytesReceived(std::uint64_t bytes);
    void addBytesSent(std::uint64_t bytes);
    void addProcessingTimeMicros(std::uint64_t micros);

    std::uint64_t getTotalRequests() const noexcept;
    std::int32_t getActiveConnections() const noexcept;
    std::uint64_t getTotalConnections() const noexcept;
    std::uint64_t getBytesReceived() const noexcept;
    std::uint64_t getBytesSent() const noexcept;
    double getUptimeSeconds() const noexcept;
    double getAverageRequestTimeMs() const noexcept;
    
    std::string toJSON() const;

private:
    Metrics();
    ~Metrics() = default;

    std::atomic<std::uint64_t> m_totalRequests{0};
    std::atomic<std::int32_t> m_activeConnections{0};
    std::atomic<std::uint64_t> m_totalConnections{0};
    std::atomic<std::uint64_t> m_bytesReceived{0};
    std::atomic<std::uint64_t> m_bytesSent{0};
    std::atomic<std::uint64_t> m_totalProcessingTimeMicros{0};
    std::chrono::time_point<std::chrono::steady_clock> m_startTime;
};
