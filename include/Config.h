#pragma once

#include <string>
#include <cstdint>

struct ServerConfig {
    std::uint16_t port = 8080;
    std::uint32_t threadCount = 4;
    std::string publicDirectory = "./public";
    bool keepAlive = true;
};

class Config {
public:
    static Config& getInstance();
    
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    bool loadFromFile(const std::string& filepath);

    const ServerConfig& getServerConfig() const;

private:
    Config() = default;
    ~Config() = default;

    ServerConfig m_config;
    
    std::string extractJsonValue(const std::string& json, const std::string& key);
};
