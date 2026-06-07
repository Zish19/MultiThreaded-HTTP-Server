#pragma once
#include "Middleware.h"
#include <vector>

class MiddlewarePipeline {
public:
    MiddlewarePipeline() = default;
    
    // Disallow copying to ensure safe reference captures if necessary
    MiddlewarePipeline(const MiddlewarePipeline&) = delete;
    MiddlewarePipeline& operator=(const MiddlewarePipeline&) = delete;
    MiddlewarePipeline(MiddlewarePipeline&&) = default;
    MiddlewarePipeline& operator=(MiddlewarePipeline&&) = default;

    void use(Middleware mw);

    // Executes the pipeline chain, terminating at finalHandler
    HttpResponse execute(const HttpRequest& req, const RouteHandler& finalHandler) const;

private:
    std::vector<Middleware> m_middlewares;

    // Helper context for execution
    class ExecutionContext {
    public:
        ExecutionContext(const std::vector<Middleware>& pipeline, const RouteHandler& finalHandler);
        HttpResponse dispatch(std::size_t index, const HttpRequest& req) const;

    private:
        const std::vector<Middleware>& m_pipeline;
        const RouteHandler& m_finalHandler;
    };
};
