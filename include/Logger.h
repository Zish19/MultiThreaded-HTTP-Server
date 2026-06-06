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

    // Delete copy constructor and assignment operator for Singleton
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void log(LogLevel level, const std::string& message);
    
    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);
    void debug(const std::string& message);

private:
    Logger() = default;
    ~Logger() = default;

    std::string levelToString(LogLevel level);
    std::string getCurrentTimestamp();

    std::mutex m_mutex;
};
