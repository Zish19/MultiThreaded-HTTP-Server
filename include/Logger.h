#pragma once

#include <iostream>
#include <string>
#include <mutex>

enum class LogLevel {
    INFO,
    WARNING,
    ERROR,
    DEBUG
};

class Logger {
public:
    static Logger& getInstance();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void log(LogLevel level, const std::string& message) const;
    
    void info(const std::string& message) const;
    void warn(const std::string& message) const;
    void error(const std::string& message) const;
    void debug(const std::string& message) const;

private:
    Logger() = default;
    ~Logger() = default;

    std::string levelToString(LogLevel level) const;
    std::string getCurrentTimestamp() const;

    mutable std::mutex m_mutex;
};
