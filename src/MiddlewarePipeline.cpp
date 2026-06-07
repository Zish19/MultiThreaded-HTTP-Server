#include "MiddlewarePipeline.h"

void MiddlewarePipeline::use(Middleware mw) {
    m_middlewares.push_back(std::move(mw));
}

HttpResponse MiddlewarePipeline::execute(const HttpRequest& req, const RouteHandler& finalHandler) const {
    ExecutionContext ctx(m_middlewares, finalHandler);
    return ctx.dispatch(0, req);
}

MiddlewarePipeline::ExecutionContext::ExecutionContext(const std::vector<Middleware>& pipeline, const RouteHandler& finalHandler)
    : m_pipeline(pipeline), m_finalHandler(finalHandler) {}

HttpResponse MiddlewarePipeline::ExecutionContext::dispatch(std::size_t index, const HttpRequest& req) const {
    if (index < m_pipeline.size()) {
        // Construct the NextHandler closure
        // Capture only `this` and `index` to ensure we fit inside std::function SOO (Small Object Optimization) buffer
        NextHandler next = [this, index](const HttpRequest& r) {
            return this->dispatch(index + 1, r);
        };
        return m_pipeline[index](req, next);
    } else {
        return m_finalHandler(req);
    }
}
