#pragma once
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "RouteHandler.h"
#include "Logger.h"
#include "Metrics.h"
#include <functional>
#include <chrono>
#include <random>
#include <sstream>
#include <iomanip>

using NextHandler = std::function<HttpResponse(const HttpRequest&)>;
using Middleware = std::function<HttpResponse(const HttpRequest&, const NextHandler&)>;

class BuiltInMiddleware {
public:
    static HttpResponse LoggingMiddleware(const HttpRequest& req, const NextHandler& next) {
        auto start = std::chrono::high_resolution_clock::now();
        
        Logger::getInstance().info("Request: " + HttpMethodUtils::toString(req.getMethod()) + " " + req.getPath());
        
        HttpResponse res = next(req);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        Logger::getInstance().info("Response: " + std::to_string(res.getStatusCode()) + " in " + std::to_string(duration) + "ms");
        
        return res;
    }

    static HttpResponse MetricsMiddleware(const HttpRequest& req, const NextHandler& next) {
        auto start = std::chrono::high_resolution_clock::now();
        
        HttpResponse res = next(req);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        
        Metrics::getInstance().addProcessingTimeMicros(duration);
        // Note: incrementRequests() is already handled in TcpServer.cpp, 
        // to avoid double counting we don't increment it here, or we can move it. 
        // The prompt says "Do not modify TcpServer", so we leave incrementRequests there.
        
        return res;
    }

    static HttpResponse RequestIdMiddleware(const HttpRequest& req, const NextHandler& next) {
        // Generate a simple 8-character hex request ID
        static thread_local std::mt19937 generator(std::random_device{}());
        std::uniform_int_distribution<uint32_t> distribution(0x10000000, 0xFFFFFFFF);
        uint32_t randomId = distribution(generator);
        
        std::stringstream ss;
        ss << std::hex << std::setw(8) << std::setfill('0') << randomId;
        std::string reqId = ss.str();
        
        Logger::getInstance().debug("[Request-ID: " + reqId + "] Started");
        
        HttpResponse res = next(req);
        res.addHeader("X-Request-ID", reqId);
        
        Logger::getInstance().debug("[Request-ID: " + reqId + "] Completed");
        
        return res;
    }
};
