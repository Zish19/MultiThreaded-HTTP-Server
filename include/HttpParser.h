#pragma once

#include "HttpRequest.h"
#include <string_view>
#include <cstddef>

class HttpParser {
public:
    // Security limits
    static constexpr std::size_t MAX_HEADER_COUNT = 100;
    static constexpr std::size_t MAX_SINGLE_HEADER_SIZE = 8192;
    static constexpr std::size_t MAX_TOTAL_HEADERS_SIZE = 65536;
    static constexpr std::size_t MAX_BODY_SIZE = 1048576; // 1 MB

    /**
     * Parses a raw HTTP request string into an HttpRequest object.
     * Throws HttpParseException if the request is malformed or exceeds security limits.
     *
     * @param rawRequest The complete raw HTTP request data.
     * @return A parsed HttpRequest.
     */
    static HttpRequest parse(std::string_view rawRequest);

private:
    static std::string_view trimWhitespace(std::string_view str);
};
