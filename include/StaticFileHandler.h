#pragma once
#include "HttpRequest.h"
#include "HttpResponse.h"

class StaticFileHandler {
public:
    static HttpResponse handle(const HttpRequest& req);
};
