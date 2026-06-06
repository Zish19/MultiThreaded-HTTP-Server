#include "HttpMethod.h"
#include <algorithm>
#include <cctype>

namespace HttpMethodUtils {

std::string toString(HttpMethod method) {
    switch (method) {
        case HttpMethod::GET: return "GET";
        case HttpMethod::POST: return "POST";
        case HttpMethod::PUT: return "PUT";
        case HttpMethod::DELETE_: return "DELETE";
        case HttpMethod::HEAD: return "HEAD";
        case HttpMethod::OPTIONS: return "OPTIONS";
        default: return "UNKNOWN";
    }
}

HttpMethod fromString(std::string_view methodStr) {
    // Case-insensitive comparison is recommended for robustness, though HTTP methods are typically uppercase
    std::string upperStr;
    upperStr.reserve(methodStr.size());
    for (char c : methodStr) {
        upperStr.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
    }

    if (upperStr == "GET") return HttpMethod::GET;
    if (upperStr == "POST") return HttpMethod::POST;
    if (upperStr == "PUT") return HttpMethod::PUT;
    if (upperStr == "DELETE") return HttpMethod::DELETE_;
    if (upperStr == "HEAD") return HttpMethod::HEAD;
    if (upperStr == "OPTIONS") return HttpMethod::OPTIONS;
    
    return HttpMethod::UNKNOWN;
}

} // namespace HttpMethodUtils
