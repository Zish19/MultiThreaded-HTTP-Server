#pragma once

#include "RouteHandler.h"
#include "HttpMethod.h"
#include "MiddlewarePipeline.h"
#include <string>
#include <unordered_map>

class Router {
public:
    Router() = default;

    // Delete copy and move semantics to ensure stable references
    Router(const Router&) = delete;
    Router& operator=(const Router&) = delete;
    Router(Router&&) = delete;
    Router& operator=(Router&&) = delete;

    // Pipeline Registration
    void use(Middleware mw);

    // Route Registration
    void get(std::string path, RouteHandler handler);
    void post(std::string path, RouteHandler handler);
    void put(std::string path, RouteHandler handler);
    void delete_(std::string path, RouteHandler handler);
    void head(std::string path, RouteHandler handler);
    void options(std::string path, RouteHandler handler);

    // Core Dispatch
    HttpResponse dispatch(const HttpRequest& req) const;

private:
    void addRoute(HttpMethod method, std::string path, RouteHandler handler);

    // Path -> (Method -> Handler)
    // Ensures strictly O(1) lookups while easily distinguishing 404 vs 405 errors.
    std::unordered_map<std::string, std::unordered_map<HttpMethod, RouteHandler>> m_routes;
    
    MiddlewarePipeline m_pipeline;
};
