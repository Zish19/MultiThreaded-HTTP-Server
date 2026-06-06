#pragma once

#include "HttpRequest.h"
#include "HttpResponse.h"
#include <functional>

using RouteHandler = std::function<HttpResponse(const HttpRequest&)>;
