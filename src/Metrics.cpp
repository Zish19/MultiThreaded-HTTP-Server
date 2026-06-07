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

void Metrics::addProcessingTimeMicros(std::uint64_t micros) noexcept {
    m_totalProcessingTimeMicros.fetch_add(micros, std::memory_order_relaxed);
}

void Metrics::recordCacheHit() noexcept {
    m_cacheHits.fetch_add(1, std::memory_order_relaxed);
}

void Metrics::recordCacheMiss() noexcept {
    m_cacheMisses.fetch_add(1, std::memory_order_relaxed);
}

void Metrics::recordCacheEviction() noexcept {
    m_cacheEvictions.fetch_add(1, std::memory_order_relaxed);
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

double Metrics::getAverageProcessingTimeMs() const noexcept {
    std::uint64_t reqs = m_totalRequests.load(std::memory_order_relaxed);
    if (reqs == 0) return 0.0;
    std::uint64_t totalMicros = m_totalProcessingTimeMicros.load(std::memory_order_relaxed);
    return (static_cast<double>(totalMicros) / reqs) / 1000.0;
}

std::uint64_t Metrics::getCacheHits() const noexcept {
    return m_cacheHits.load(std::memory_order_relaxed);
}

std::uint64_t Metrics::getCacheMisses() const noexcept {
    return m_cacheMisses.load(std::memory_order_relaxed);
}

std::uint64_t Metrics::getCacheEvictions() const noexcept {
    return m_cacheEvictions.load(std::memory_order_relaxed);
}

double Metrics::getCacheHitRatio() const noexcept {
    std::uint64_t hits = getCacheHits();
    std::uint64_t misses = getCacheMisses();
    std::uint64_t total = hits + misses;
    if (total == 0) return 0.0;
    return (static_cast<double>(hits) / static_cast<double>(total)) * 100.0;
}

std::string Metrics::toJSON() const {
    std::stringstream ss;
    ss << "{\n"
       << "  \"total_requests\": " << getTotalRequests() << ",\n"
       << "  \"total_errors\": " << m_totalErrors.load(std::memory_order_relaxed) << ",\n"
       << "  \"active_connections\": " << getActiveConnections() << ",\n"
       << "  \"total_connections\": " << getTotalConnections() << ",\n"
       << "  \"bytes_received\": " << getBytesReceived() << ",\n"
       << "  \"bytes_sent\": " << getBytesSent() << ",\n"
       << "  \"average_processing_time_ms\": " << getAverageProcessingTimeMs() << ",\n"
       << "  \"cache_hits\": " << m_cacheHits.load(std::memory_order_relaxed) << ",\n"
       << "  \"cache_misses\": " << m_cacheMisses.load(std::memory_order_relaxed) << ",\n"
       << "  \"cache_evictions\": " << m_cacheEvictions.load(std::memory_order_relaxed) << ",\n"
       << "  \"cache_hit_ratio\": " << getCacheHitRatio() << ",\n"
       << "  \"uptime_seconds\": " << getUptimeSeconds() << "\n"
       << "}";
    return ss.str();
}
