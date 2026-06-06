#include "HttpParser.h"
#include "HttpParseException.h"
#include <vector>
#include <cctype>

std::string_view HttpParser::trimWhitespace(std::string_view str) {
    auto start = str.find_first_not_of(" \t");
    if (start == std::string_view::npos) return "";
    auto end = str.find_last_not_of(" \t");
    return str.substr(start, end - start + 1);
}

std::size_t HttpParser::extractExpectedBodySize(std::string_view headerSection) {
    std::size_t pos = 0;
    while (pos < headerSection.size()) {
        std::size_t nextLine = headerSection.find('\n', pos);
        std::string_view line;
        if (nextLine == std::string_view::npos) {
            line = headerSection.substr(pos);
            pos = headerSection.size();
        } else {
            line = headerSection.substr(pos, nextLine - pos);
            pos = nextLine + 1;
        }

        if (!line.empty() && line.back() == '\r') {
            line.remove_suffix(1);
        }

        auto colonPos = line.find(':');
        if (colonPos != std::string_view::npos) {
            std::string_view key = trimWhitespace(line.substr(0, colonPos));
            
            if (key.size() == 14) {
                bool match = true;
                const char* expected = "content-length";
                for (size_t i = 0; i < 14; ++i) {
                    if (std::tolower(static_cast<unsigned char>(key[i])) != expected[i]) {
                        match = false;
                        break;
                    }
                }
                if (match) {
                    std::string_view val = trimWhitespace(line.substr(colonPos + 1));
                    if (val.empty() || val.find_first_not_of("0123456789") != std::string_view::npos) {
                        throw HttpParseException("Malformed request: Invalid Content-Length format");
                    }
                    try {
                        std::size_t cl = std::stoull(std::string(val));
                        if (cl > MAX_BODY_SIZE) {
                            throw HttpParseException("Request rejected: Body exceeds maximum size limit");
                        }
                        return cl;
                    } catch (...) {
                        throw HttpParseException("Malformed request: Content-Length value out of range");
                    }
                }
            }
        }
    }
    return 0;
}

HttpRequest HttpParser::parse(std::string_view rawRequest) {
    if (rawRequest.empty()) {
        throw HttpParseException("Malformed request: Empty request");
    }

    // 1. Find Header/Body Separator
    std::size_t separatorLen = 4;
    std::size_t headerEnd = rawRequest.find("\r\n\r\n");

    if (headerEnd == std::string_view::npos) {
        headerEnd = rawRequest.find("\n\n");
        separatorLen = 2;
    }

    if (headerEnd == std::string_view::npos) {
        throw HttpParseException("Malformed request: Missing header-body separator");
    }

    std::string_view headerSection = rawRequest.substr(0, headerEnd);
    std::string_view bodySection = rawRequest.substr(headerEnd + separatorLen);

    if (headerSection.size() > MAX_TOTAL_HEADERS_SIZE) {
        throw HttpParseException("Request rejected: Total header size exceeds maximum limit");
    }

    // Determine line ending used in headers based on the separator found
    std::string_view lineEnding = (separatorLen == 4) ? "\r\n" : "\n";

    // Enforce strict newline style
    if (lineEnding == "\n") {
        if (headerSection.find('\r') != std::string_view::npos) {
            throw HttpParseException("Malformed request: Mixed newline styles detected");
        }
    } else {
        // For \r\n, check if any \n is not preceded by \r
        for (std::size_t i = 0; i < headerSection.size(); ++i) {
            if (headerSection[i] == '\n') {
                if (i == 0 || headerSection[i - 1] != '\r') {
                    throw HttpParseException("Malformed request: Mixed newline styles detected");
                }
            }
        }
    }

    // 2. Parse Request Line
    std::size_t firstLineEnd = headerSection.find(lineEnding);
    if (firstLineEnd == std::string_view::npos) {
        // If there's no line ending but there's a separator, the request line is the whole header section
        firstLineEnd = headerSection.size();
    }

    std::string_view requestLine = headerSection.substr(0, firstLineEnd);
    
    // Split request line by spaces (handling multiple contiguous spaces)
    std::vector<std::string_view> requestTokens;
    std::size_t pos = 0;
    while (pos < requestLine.size()) {
        std::size_t tokenStart = requestLine.find_first_not_of(' ', pos);
        if (tokenStart == std::string_view::npos) break;
        std::size_t tokenEnd = requestLine.find_first_of(' ', tokenStart);
        if (tokenEnd == std::string_view::npos) tokenEnd = requestLine.size();
        
        requestTokens.push_back(requestLine.substr(tokenStart, tokenEnd - tokenStart));
        pos = tokenEnd;
    }

    if (requestTokens.size() != 3) {
        throw HttpParseException("Malformed request: Request line must have exactly 3 tokens");
    }

    HttpRequest req;

    // Parse Method
    HttpMethod method = HttpMethodUtils::fromString(requestTokens[0]);
    if (method == HttpMethod::UNKNOWN) {
        throw HttpParseException("Malformed request: Invalid or unknown HTTP method");
    }
    req.setMethod(method);

    // Parse Path
    std::string_view pathToken = requestTokens[1];
    if (pathToken.empty() || pathToken[0] != '/') {
        throw HttpParseException("Malformed request: Path must start with '/'");
    }
    req.setPath(std::string(pathToken));

    // Parse Version
    std::string_view versionToken = requestTokens[2];
    if (versionToken != "HTTP/1.0" && versionToken != "HTTP/1.1") {
        throw HttpParseException("Malformed request: Unsupported HTTP version");
    }
    req.setVersion(std::string(versionToken));

    // 3. Parse Headers
    std::size_t headerLinesStart = (firstLineEnd == headerSection.size()) ? headerSection.size() : firstLineEnd + lineEnding.size();
    std::string_view headersBlock = headerSection.substr(headerLinesStart);
    
    std::size_t headerCount = 0;
    std::size_t currentLineStart = 0;

    while (currentLineStart < headersBlock.size()) {
        std::size_t nextLineEnd = headersBlock.find(lineEnding, currentLineStart);
        std::string_view headerLine;
        
        if (nextLineEnd == std::string_view::npos) {
            headerLine = headersBlock.substr(currentLineStart);
            currentLineStart = headersBlock.size();
        } else {
            headerLine = headersBlock.substr(currentLineStart, nextLineEnd - currentLineStart);
            currentLineStart = nextLineEnd + lineEnding.size();
        }

        if (headerLine.empty()) continue; // Skip empty lines between headers (if any)
        
        if (headerLine.size() > MAX_SINGLE_HEADER_SIZE) {
            throw HttpParseException("Request rejected: Single header exceeds maximum size limit");
        }

        headerCount++;
        if (headerCount > MAX_HEADER_COUNT) {
            throw HttpParseException("Request rejected: Exceeded maximum header count");
        }

        std::size_t colonPos = headerLine.find(':');
        if (colonPos == std::string_view::npos) {
            throw HttpParseException("Malformed request: Header missing colon");
        }

        std::string_view key = trimWhitespace(headerLine.substr(0, colonPos));
        std::string_view value = trimWhitespace(headerLine.substr(colonPos + 1));

        if (key.empty()) {
            throw HttpParseException("Malformed request: Empty header key");
        }

        // Duplicate header check
        std::string lowerKey;
        lowerKey.reserve(key.size());
        for (char c : key) {
            lowerKey.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }

        if (req.hasHeader(lowerKey)) {
            throw HttpParseException("Malformed request: Duplicate header detected");
        }

        req.addHeader(std::string(key), std::string(value));
    }

    // 4. Body Extraction & Content-Length Validation
    bool hasContentLength = req.hasHeader("content-length");
    std::size_t contentLength = 0;

    if (hasContentLength) {
        contentLength = extractExpectedBodySize(headerSection);
        if (bodySection.size() < contentLength) {
            throw HttpParseException("Malformed request: Body truncated (size < Content-Length)");
        }
        // We take exactly contentLength bytes
        req.setBody(std::string(bodySection.substr(0, contentLength)));
    } else {
        // If there's no Content-Length, the body is empty for basic HTTP/1.1 without Chunked Transfer
        req.setBody("");
    }

    return req;
}
