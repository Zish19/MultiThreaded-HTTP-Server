#pragma once

#include "HttpMethod.h"
#include <string>
#include <unordered_map>
#include <cstdint>
#include <optional>

class HttpRequest {
public:
    HttpRequest() = default;

    // Move semantics (very important for large bodies)
    HttpRequest(HttpRequest&&) noexcept = default;
    HttpRequest& operator=(HttpRequest&&) noexcept = default;

    // Copy semantics allowed but might be expensive
    HttpRequest(const HttpRequest&) = default;
    HttpRequest& operator=(const HttpRequest&) = default;

    // Setters
    void setMethod(HttpMethod method) noexcept;
    void setPath(std::string path);
    void setVersion(std::string version);
    void addHeader(const std::string& key, const std::string& value);
    void setBody(std::string body);

    // Getters
    HttpMethod getMethod() const noexcept;
    const std::string& getPath() const noexcept;
    const std::string& getVersion() const noexcept;
    const std::string& getBody() const noexcept;

    // Header inspection
    bool hasHeader(const std::string& key) const;
    std::optional<std::string> getHeader(const std::string& key) const;
    const std::unordered_map<std::string, std::string>& getHeaders() const noexcept;

    // HTTP specific helpers
    bool isKeepAlive() const;
    std::size_t contentLength() const;

private:
    HttpMethod m_method{HttpMethod::UNKNOWN};
    std::string m_path;
    std::string m_version;
    // Keys will be stored in lowercase to ensure case-insensitive lookup
    std::unordered_map<std::string, std::string> m_headers;
    std::string m_body;
};
