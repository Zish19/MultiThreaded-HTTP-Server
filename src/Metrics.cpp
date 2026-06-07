#include "Metrics.h"
#include <sstream>

Metrics::Metrics() : m_startTime(std::chrono::steady_clock::now()) {}

Metrics& Metrics::getInstance() {
    static Metrics instance;
    return instance;
}

void Metrics::incrementRequests() {
    m_totalRequests.fetch_add(1, std::memory_order_relaxed);
}

void Metrics::incrementActiveConnections() {
    m_activeConnections.fetch_add(1, std::memory_order_relaxed);
}

void Metrics::decrementActiveConnections() {
    m_activeConnections.fetch_sub(1, std::memory_order_relaxed);
}

void Metrics::incrementTotalConnections() {
    m_totalConnections.fetch_add(1, std::memory_order_relaxed);
}

void Metrics::addBytesReceived(std::uint64_t bytes) {
    m_bytesReceived.fetch_add(bytes, std::memory_order_relaxed);
}

void Metrics::addBytesSent(std::uint64_t bytes) {
    m_bytesSent.fetch_add(bytes, std::memory_order_relaxed);
}

void Metrics::addProcessingTimeMicros(std::uint64_t micros) {
    m_totalProcessingTimeMicros.fetch_add(micros, std::memory_order_relaxed);
}

std::uint64_t Metrics::getTotalRequests() const noexcept {
    return m_totalRequests.load(std::memory_order_relaxed);
}

std::int32_t Metrics::getActiveConnections() const noexcept {
    return m_activeConnections.load(std::memory_order_relaxed);
}

std::uint64_t Metrics::getTotalConnections() const noexcept {
    return m_totalConnections.load(std::memory_order_relaxed);
}

std::uint64_t Metrics::getBytesReceived() const noexcept {
    return m_bytesReceived.load(std::memory_order_relaxed);
}

std::uint64_t Metrics::getBytesSent() const noexcept {
    return m_bytesSent.load(std::memory_order_relaxed);
}

double Metrics::getUptimeSeconds() const noexcept {
    auto now = std::chrono::steady_clock::now();
    std::chrono::duration<double> uptime = now - m_startTime;
    return uptime.count();
}

double Metrics::getAverageRequestTimeMs() const noexcept {
    std::uint64_t totalReqs = getTotalRequests();
    if (totalReqs == 0) return 0.0;
    std::uint64_t totalMicros = m_totalProcessingTimeMicros.load(std::memory_order_relaxed);
    return static_cast<double>(totalMicros) / 1000.0 / static_cast<double>(totalReqs);
}

std::string Metrics::toJSON() const {
    std::stringstream ss;
    ss << "{\n"
       << "  \"total_requests\": " << getTotalRequests() << ",\n"
       << "  \"active_connections\": " << getActiveConnections() << ",\n"
       << "  \"total_connections\": " << getTotalConnections() << ",\n"
       << "  \"bytes_received\": " << getBytesReceived() << ",\n"
       << "  \"bytes_sent\": " << getBytesSent() << ",\n"
       << "  \"average_request_time_ms\": " << getAverageRequestTimeMs() << ",\n"
       << "  \"uptime_seconds\": " << getUptimeSeconds() << "\n"
       << "}";
    return ss.str();
}
