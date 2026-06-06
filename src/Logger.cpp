#include "Logger.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

std::string Logger::levelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::DEBUG: return "DEBUG";
        default: return "UNKNOWN";
    }
}

std::string Logger::getCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    std::tm buf;
#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&buf, &in_time_t);
#else
    localtime_r(&in_time_t, &buf);
#endif
    ss << std::put_time(&buf, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

void Logger::log(LogLevel level, const std::string& message) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::cout << "[" << getCurrentTimestamp() << "] " 
              << "[" << levelToString(level) << "] " 
              << message << std::endl;
}

void Logger::info(const std::string& message) const { log(LogLevel::INFO, message); }
void Logger::warn(const std::string& message) const { log(LogLevel::WARNING, message); }
void Logger::error(const std::string& message) const { log(LogLevel::ERROR, message); }
void Logger::debug(const std::string& message) const { log(LogLevel::DEBUG, message); }
