#include "MimeTypes.h"
#include <unordered_map>

std::string MimeTypes::getType(const std::string& extension) {
    static const std::unordered_map<std::string, std::string> mimeTypes = {
        {".html", "text/html"},
        {".css", "text/css"},
        {".js", "application/javascript"},
        {".json", "application/json"},
        {".txt", "text/plain"},
        {".png", "image/png"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".svg", "image/svg+xml"},
        {".ico", "image/x-icon"},
        {".webp", "image/webp"}
    };

    auto it = mimeTypes.find(extension);
    if (it != mimeTypes.end()) {
        return it->second;
    }
    
    // Default fallback
    return "application/octet-stream";
}
