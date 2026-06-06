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

std::uint64_t Metrics::getTotalRequests() const {
    return m_totalRequests.load(std::memory_order_relaxed);
}

std::int32_t Metrics::getActiveConnections() const {
    return m_activeConnections.load(std::memory_order_relaxed);
}

double Metrics::getUptimeSeconds() const {
    auto now = std::chrono::steady_clock::now();
    std::chrono::duration<double> uptime = now - m_startTime;
    return uptime.count();
}

std::string Metrics::toJSON() const {
    std::stringstream ss;
    ss << "{\n"
       << "  \"total_requests\": " << getTotalRequests() << ",\n"
       << "  \"active_connections\": " << getActiveConnections() << ",\n"
       << "  \"uptime_seconds\": " << getUptimeSeconds() << "\n"
       << "}";
    return ss.str();
}
