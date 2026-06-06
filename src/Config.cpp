#include "Config.h"
#include "Logger.h"
#include <fstream>
#include <sstream>

Config& Config::getInstance() {
    static Config instance;
    return instance;
}

const ServerConfig& Config::getServerConfig() const {
    return m_config;
}

// A highly simplified JSON extraction function (assumes flat JSON structure).
// Interview note: Writing a full JSON parser is a separate project. 
// This demonstrates manual string parsing for basic needs without dependencies.
std::string Config::extractJsonValue(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";
    std::size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) return "";

    std::size_t colonPos = json.find(':', keyPos);
    if (colonPos == std::string::npos) return "";

    // Find the start of the value
    std::size_t valueStart = json.find_first_not_of(" \t\n\r", colonPos + 1);
    if (valueStart == std::string::npos) return "";

    std::string result;
    if (json[valueStart] == '\"') {
        // It's a string value
        std::size_t valueEnd = json.find('\"', valueStart + 1);
        if (valueEnd != std::string::npos) {
            result = json.substr(valueStart + 1, valueEnd - valueStart - 1);
        }
    } else {
        // It's a boolean or number
        std::size_t valueEnd = json.find_first_of(", \t\n\r}", valueStart);
        if (valueEnd != std::string::npos) {
            result = json.substr(valueStart, valueEnd - valueStart);
        } else {
            result = json.substr(valueStart);
        }
    }
    return result;
}

bool Config::loadFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        Logger::getInstance().warn("Config file not found at " + filepath + ". Using default configuration.");
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string jsonStr = buffer.str();

    std::string portStr = extractJsonValue(jsonStr, "port");
    if (!portStr.empty()) {
        m_config.port = static_cast<std::uint16_t>(std::stoi(portStr));
    }

    std::string threadsStr = extractJsonValue(jsonStr, "threadCount");
    if (!threadsStr.empty()) {
        m_config.threadCount = static_cast<std::uint32_t>(std::stoi(threadsStr));
    }

    std::string pubDir = extractJsonValue(jsonStr, "publicDirectory");
    if (!pubDir.empty()) {
        m_config.publicDirectory = pubDir;
    }
    
    std::string keepAliveStr = extractJsonValue(jsonStr, "keepAlive");
    if (!keepAliveStr.empty()) {
        m_config.keepAlive = (keepAliveStr == "true" || keepAliveStr == "1");
    }

    Logger::getInstance().info("Configuration loaded from " + filepath);
    return true;
}
