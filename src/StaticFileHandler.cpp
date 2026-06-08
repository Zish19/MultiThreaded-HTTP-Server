#include "StaticFileHandler.h"
#include "MimeTypes.h"
#include "Logger.h"
#include <filesystem>
#include <fstream>
#include <iostream>

namespace {
    bool isSubPath(const std::filesystem::path& base, const std::filesystem::path& path) {
        auto baseIt = base.begin();
        auto pathIt = path.begin();
        while (baseIt != base.end() && pathIt != path.end()) {
            if (*baseIt != *pathIt) {
                return false;
            }
            ++baseIt;
            ++pathIt;
        }
        return baseIt == base.end();
    }
}

StaticFileHandler::StaticFileHandler(FileCache& cache, const std::filesystem::path& publicDir)
    : m_cache(cache), m_publicDir(std::filesystem::weakly_canonical(std::filesystem::absolute(publicDir))) {
    Logger::getInstance().info("[StaticFileHandler] Resolved public directory: " + m_publicDir.string());
}

HttpResponse StaticFileHandler::handle(const HttpRequest& req) {
    std::string path = req.getPath();
    
    if (path == "/") {
        path = "/index.html";
    }

    if (!path.empty() && path[0] == '/') {
        path = path.substr(1); // Remove leading slash for safe joining
    }

    std::filesystem::path requestedPath;
    try {
        requestedPath = std::filesystem::weakly_canonical(m_publicDir / path);
    } catch (const std::exception&) {
        Logger::getInstance().warn("[StaticFileHandler] Bad path: " + path);
        HttpResponse res;
        res.setStatusCode(400);
        res.setStatusText("Bad Request");
        res.setBody("Bad Request");
        return res;
    }

    Logger::getInstance().info("[StaticFileHandler] Path: " + path
        + " -> " + requestedPath.string()
        + " exists=" + (std::filesystem::exists(requestedPath) ? "true" : "false")
        + " is_file=" + (std::filesystem::is_regular_file(requestedPath) ? "true" : "false"));

    if (!isSubPath(m_publicDir, requestedPath)) {
        Logger::getInstance().warn("[StaticFileHandler] Path traversal blocked: " + requestedPath.string());
        HttpResponse res;
        res.setStatusCode(403);
        res.setStatusText("Forbidden");
        res.setBody("Forbidden");
        return res;
    }

    if (!std::filesystem::exists(requestedPath) || !std::filesystem::is_regular_file(requestedPath)) {
        Logger::getInstance().info("[StaticFileHandler] File not found: " + requestedPath.string());
        return HttpResponse::notFound();
    }

    auto cachedFileOpt = m_cache.getFile(requestedPath);
    if (!cachedFileOpt) {
        Logger::getInstance().warn("[StaticFileHandler] Cache returned nullopt for: " + requestedPath.string());
        HttpResponse res;
        res.setStatusCode(403);
        res.setStatusText("Forbidden");
        res.setBody("Forbidden");
        return res;
    }

    Logger::getInstance().info("[StaticFileHandler] Serving: " + requestedPath.string()
        + " (" + std::to_string(cachedFileOpt->size) + " bytes, " + cachedFileOpt->mimeType + ")");

    HttpResponse res = HttpResponse::ok(cachedFileOpt->content);
    res.addHeader("Content-Type", cachedFileOpt->mimeType);
    res.addHeader("Content-Length", std::to_string(cachedFileOpt->size));

    return res;
}
