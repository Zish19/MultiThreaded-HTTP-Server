#include "Router.h"
#include <stdexcept>

void Router::addRoute(HttpMethod method, std::string path, RouteHandler handler) {
    auto& methodMap = m_routes[path];
    if (methodMap.find(method) != methodMap.end()) {
        throw std::runtime_error("Route already registered");
    }
    methodMap[method] = std::move(handler);
}

void Router::get(std::string path, RouteHandler handler) {
    addRoute(HttpMethod::GET, std::move(path), std::move(handler));
}

void Router::post(std::string path, RouteHandler handler) {
    addRoute(HttpMethod::POST, std::move(path), std::move(handler));
}

void Router::put(std::string path, RouteHandler handler) {
    addRoute(HttpMethod::PUT, std::move(path), std::move(handler));
}

void Router::delete_(std::string path, RouteHandler handler) {
    addRoute(HttpMethod::DELETE_, std::move(path), std::move(handler));
}

void Router::head(std::string path, RouteHandler handler) {
    addRoute(HttpMethod::HEAD, std::move(path), std::move(handler));
}

void Router::options(std::string path, RouteHandler handler) {
    addRoute(HttpMethod::OPTIONS, std::move(path), std::move(handler));
}

HttpResponse Router::dispatch(const HttpRequest& req) const {
    auto pathIt = m_routes.find(req.getPath());
    if (pathIt == m_routes.end()) {
        return HttpResponse::notFound();
    }

    auto methodIt = pathIt->second.find(req.getMethod());
    if (methodIt == pathIt->second.end()) {
        return HttpResponse::methodNotAllowed();
    }

    try {
        return methodIt->second(req);
    } catch (...) {
        return HttpResponse::internalServerError();
    }
}
