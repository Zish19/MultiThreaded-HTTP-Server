#pragma once

#include <string>
#include <string_view>

enum class HttpMethod {
    GET,
    POST,
    PUT,
    DELETE_, // Trailing underscore to avoid macro collision with Windows DELETE macro
    HEAD,
    OPTIONS,
    UNKNOWN
};

namespace HttpMethodUtils {
    std::string toString(HttpMethod method);
    HttpMethod fromString(std::string_view methodStr);
}
