#pragma once
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "FileCache.h"

class StaticFileHandler {
public:
    explicit StaticFileHandler(FileCache& cache);
    HttpResponse handle(const HttpRequest& req);

private:
    FileCache& m_cache;
};
