#include "HttpResponse.h"
#include <cctype>

void HttpResponse::setStatusCode(int code) noexcept {
    m_statusCode = code;
}

void HttpResponse::setStatusText(std::string text) {
    m_statusText = std::move(text);
}

void HttpResponse::addHeader(const std::string& key, const std::string& value) {
    // Unlike Request, Response headers don't strictly *need* to be internally lowercased for serialization,
    // but it is good practice for consistent lookup if we ever need to inspect our own outgoing headers.
    std::string lowerKey;
    lowerKey.reserve(key.size());
    for (char c : key) {
        lowerKey.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    // Note: We'll serialize using the original casing ideally, but HTTP/1.1 headers are case-insensitive.
    // For simplicity, we just store lowercase. When serializing, lowercase headers are valid.
    // For "Content-Length", we'll always append a properly cased "Content-Length" during toString()
    m_headers[std::move(lowerKey)] = value;
}

void HttpResponse::setBody(std::string body) {
    m_body = std::move(body);
}

int HttpResponse::getStatusCode() const noexcept {
    return m_statusCode;
}

const std::string& HttpResponse::getStatusText() const noexcept {
    return m_statusText;
}

const std::string& HttpResponse::getBody() const noexcept {
    return m_body;
}

std::optional<std::string> HttpResponse::getHeader(const std::string& key) const {
    std::string lowerKey;
    lowerKey.reserve(key.size());
    for (char c : key) {
        lowerKey.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    auto it = m_headers.find(lowerKey);
    if (it != m_headers.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::string HttpResponse::toString() const {
    std::string response;
    // Estimate size to avoid reallocations
    std::size_t estimatedSize = 32 + m_body.size() + (m_headers.size() * 40);
    response.reserve(estimatedSize);

    // Status Line
    response.append("HTTP/1.1 ");
    response.append(std::to_string(m_statusCode));
    response.append(" ");
    response.append(m_statusText);
    response.append("\r\n");

    // Headers
    for (const auto& [key, value] : m_headers) {
        // Skip Content-Length if the user accidentally added it, we will forcefully inject it
        if (key == "content-length") {
            continue;
        }
        
        // Capitalize the first letter of each part for nicer output (e.g., content-type -> Content-Type)
        std::string displayKey = key;
        bool capitalizeNext = true;
        for (char& c : displayKey) {
            if (capitalizeNext && std::isalpha(static_cast<unsigned char>(c))) {
                c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
                capitalizeNext = false;
            } else if (c == '-') {
                capitalizeNext = true;
            }
        }
        
        response.append(displayKey);
        response.append(": ");
        response.append(value);
        response.append("\r\n");
    }

    // Always inject correct Content-Length based on body size
    response.append("Content-Length: ");
    response.append(std::to_string(m_body.size()));
    response.append("\r\n");

    // Empty line separating headers and body
    response.append("\r\n");

    // Body
    response.append(m_body);

    return response;
}

HttpResponse HttpResponse::ok(std::string body) {
    HttpResponse res;
    res.setStatusCode(200);
    res.setStatusText("OK");
    res.setBody(std::move(body));
    return res;
}

HttpResponse HttpResponse::badRequest(std::string body) {
    HttpResponse res;
    res.setStatusCode(400);
    res.setStatusText("Bad Request");
    res.setBody(std::move(body));
    return res;
}

HttpResponse HttpResponse::notFound(std::string body) {
    HttpResponse res;
    res.setStatusCode(404);
    res.setStatusText("Not Found");
    res.setBody(std::move(body));
    return res;
}

HttpResponse HttpResponse::methodNotAllowed(std::string body) {
    HttpResponse res;
    res.setStatusCode(405);
    res.setStatusText("Method Not Allowed");
    res.setBody(std::move(body));
    return res;
}

HttpResponse HttpResponse::unprocessableEntity(std::string body) {
    HttpResponse res;
    res.setStatusCode(422);
    res.setStatusText("Unprocessable Entity");
    res.setBody(std::move(body));
    return res;
}

HttpResponse HttpResponse::internalServerError(std::string body) {
    HttpResponse res;
    res.setStatusCode(500);
    res.setStatusText("Internal Server Error");
    res.setBody(std::move(body));
    return res;
}
