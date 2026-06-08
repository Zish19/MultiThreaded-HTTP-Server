#pragma once
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "FileCache.h"
#include <filesystem>

class StaticFileHandler {
public:
    StaticFileHandler(FileCache& cache, const std::filesystem::path& publicDir);
    HttpResponse handle(const HttpRequest& req);

private:
    FileCache& m_cache;
    std::filesystem::path m_publicDir;
};
