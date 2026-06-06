#include "HttpRequest.h"
#include <algorithm>
#include <cctype>

void HttpRequest::setMethod(HttpMethod method) noexcept {
    m_method = method;
}

void HttpRequest::setPath(std::string path) {
    m_path = std::move(path);
}

void HttpRequest::setVersion(std::string version) {
    m_version = std::move(version);
}

void HttpRequest::addHeader(const std::string& key, const std::string& value) {
    std::string lowerKey;
    lowerKey.reserve(key.size());
    for (char c : key) {
        lowerKey.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    m_headers[std::move(lowerKey)] = value;
}

void HttpRequest::setBody(std::string body) {
    m_body = std::move(body);
}

HttpMethod HttpRequest::getMethod() const noexcept {
    return m_method;
}

const std::string& HttpRequest::getPath() const noexcept {
    return m_path;
}

const std::string& HttpRequest::getVersion() const noexcept {
    return m_version;
}

const std::string& HttpRequest::getBody() const noexcept {
    return m_body;
}

bool HttpRequest::hasHeader(const std::string& key) const {
    std::string lowerKey;
    lowerKey.reserve(key.size());
    for (char c : key) {
        lowerKey.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return m_headers.find(lowerKey) != m_headers.end();
}

std::optional<std::string> HttpRequest::getHeader(const std::string& key) const {
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

const std::unordered_map<std::string, std::string>& HttpRequest::getHeaders() const noexcept {
    return m_headers;
}

bool HttpRequest::isKeepAlive() const {
    auto connHeader = getHeader("connection");
    if (connHeader) {
        std::string val = *connHeader;
        // Convert to lowercase for comparison
        for (char& c : val) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        if (val == "close") {
            return false;
        }
        if (val == "keep-alive") {
            return true;
        }
    }
    
    // HTTP/1.1 defaults to keep-alive
    if (m_version == "HTTP/1.1") {
        return true;
    }
    
    // HTTP/1.0 defaults to close unless keep-alive is explicitly specified
    return false;
}

std::size_t HttpRequest::contentLength() const {
    auto lenStr = getHeader("content-length");
    if (lenStr) {
        try {
            return std::stoull(*lenStr);
        } catch (...) {
            return 0;
        }
    }
    return 0;
}
