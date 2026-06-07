#include "StaticFileHandler.h"
#include "Config.h"
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

StaticFileHandler::StaticFileHandler(FileCache& cache) : m_cache(cache) {}

HttpResponse StaticFileHandler::handle(const HttpRequest& req) {
    std::string path = req.getPath();
    
    if (path == "/") {
        path = "/index.html";
    }

    if (!path.empty() && path[0] == '/') {
        path = path.substr(1); // Remove leading slash for safe joining
    }

    std::string publicDirStr = Config::getInstance().getPublicDirectory();
    std::filesystem::path publicDir;
    try {
        publicDir = std::filesystem::weakly_canonical(publicDirStr);
    } catch (const std::exception& e) {
        Logger::getInstance().error("Failed to resolve public directory: " + std::string(e.what()));
        return HttpResponse::internalServerError();
    }

    std::filesystem::path requestedPath;
    try {
        requestedPath = std::filesystem::weakly_canonical(publicDir / path);
    } catch (const std::exception&) {
        HttpResponse res;
        res.setStatusCode(400);
        res.setStatusText("Bad Request");
        res.setBody("Bad Request");
        return res;
    }

    if (!isSubPath(publicDir, requestedPath)) {
        HttpResponse res;
        res.setStatusCode(403);
        res.setStatusText("Forbidden");
        res.setBody("Forbidden");
        return res;
    }

    if (!std::filesystem::exists(requestedPath) || !std::filesystem::is_regular_file(requestedPath)) {
        return HttpResponse::notFound();
    }

    auto cachedFileOpt = m_cache.getFile(requestedPath);
    if (!cachedFileOpt) {
        HttpResponse res;
        res.setStatusCode(403);
        res.setStatusText("Forbidden");
        res.setBody("Forbidden");
        return res;
    }

    HttpResponse res = HttpResponse::ok(cachedFileOpt->content);
    res.addHeader("Content-Type", cachedFileOpt->mimeType);
    res.addHeader("Content-Length", std::to_string(cachedFileOpt->size));

    return res;
}
