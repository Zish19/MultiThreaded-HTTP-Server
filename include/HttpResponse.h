#pragma once

#include <string>
#include <unordered_map>
#include <cstdint>
#include <optional>

class HttpResponse {
public:
    HttpResponse() = default;

    // Move semantics for performance with large bodies
    HttpResponse(HttpResponse&&) noexcept = default;
    HttpResponse& operator=(HttpResponse&&) noexcept = default;

    // Copy semantics allowed
    HttpResponse(const HttpResponse&) = default;
    HttpResponse& operator=(const HttpResponse&) = default;

    // Setters
    void setStatusCode(int code) noexcept;
    void setStatusText(std::string text);
    void addHeader(const std::string& key, const std::string& value);
    void setBody(std::string body);

    // Getters
    int getStatusCode() const noexcept;
    const std::string& getStatusText() const noexcept;
    const std::string& getBody() const noexcept;
    std::optional<std::string> getHeader(const std::string& key) const;

    // Serialization
    std::string toString() const;

    // Factory methods
    static HttpResponse ok(std::string body = "");
    static HttpResponse badRequest(std::string body = "Bad Request");
    static HttpResponse notFound(std::string body = "Not Found");
    static HttpResponse methodNotAllowed(std::string body = "Method Not Allowed");
    static HttpResponse unprocessableEntity(std::string body = "Unprocessable Entity");
    static HttpResponse internalServerError(std::string body = "Internal Server Error");

private:
    int m_statusCode{200};
    std::string m_statusText{"OK"};
    std::unordered_map<std::string, std::string> m_headers;
    std::string m_body;
};
