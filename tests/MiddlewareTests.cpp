#include "Router.h"
#include "Middleware.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <thread>

void testSingleMiddleware() {
    Router router;
    bool middlewareExecuted = false;
    
    router.use([&](const HttpRequest& req, const NextHandler& next) {
        middlewareExecuted = true;
        return next(req);
    });
    
    router.get("/test", [](const HttpRequest&) {
        return HttpResponse::ok("Success");
    });
    
    HttpRequest req;
    req.setMethod(HttpMethod::GET);
    req.setPath("/test");
    HttpResponse res = router.dispatch(req);
    
    assert(middlewareExecuted);
    assert(res.getStatusCode() == 200);
    std::cout << "[PASS] testSingleMiddleware\n";
}

void testMiddlewareOrdering() {
    Router router;
    std::vector<std::string> executionOrder;
    
    router.use([&](const HttpRequest& req, const NextHandler& next) {
        executionOrder.push_back("A Before");
        HttpResponse res = next(req);
        executionOrder.push_back("A After");
        return res;
    });
    
    router.use([&](const HttpRequest& req, const NextHandler& next) {
        executionOrder.push_back("B Before");
        HttpResponse res = next(req);
        executionOrder.push_back("B After");
        return res;
    });
    
    router.get("/order", [&](const HttpRequest&) {
        executionOrder.push_back("Handler");
        return HttpResponse::ok("Order Test");
    });
    
    HttpRequest req;
    req.setMethod(HttpMethod::GET);
    req.setPath("/order");
    router.dispatch(req);
    
    assert(executionOrder.size() == 5);
    assert(executionOrder[0] == "A Before");
    assert(executionOrder[1] == "B Before");
    assert(executionOrder[2] == "Handler");
    assert(executionOrder[3] == "B After");
    assert(executionOrder[4] == "A After");
    std::cout << "[PASS] testMiddlewareOrdering\n";
}

void testMiddlewareModifyingResponse() {
    Router router;
    
    router.use([](const HttpRequest& req, const NextHandler& next) {
        HttpResponse res = next(req);
        res.addHeader("X-Custom-Header", "Modified");
        return res;
    });
    
    router.get("/mod", [](const HttpRequest&) {
        return HttpResponse::ok("Content");
    });
    
    HttpRequest req;
    req.setMethod(HttpMethod::GET);
    req.setPath("/mod");
    HttpResponse res = router.dispatch(req);
    
    assert(res.getHeaders().at("X-Custom-Header") == "Modified");
    std::cout << "[PASS] testMiddlewareModifyingResponse\n";
}

void testMiddlewareShortCircuit() {
    Router router;
    bool handlerExecuted = false;
    
    router.use([](const HttpRequest&, const NextHandler&) {
        // Does not call next()
        HttpResponse res;
        res.setStatusCode(401);
        res.setBody("Unauthorized");
        return res;
    });
    
    router.get("/secret", [&](const HttpRequest&) {
        handlerExecuted = true;
        return HttpResponse::ok("Secret Content");
    });
    
    HttpRequest req;
    req.setMethod(HttpMethod::GET);
    req.setPath("/secret");
    HttpResponse res = router.dispatch(req);
    
    assert(!handlerExecuted);
    assert(res.getStatusCode() == 401);
    std::cout << "[PASS] testMiddlewareShortCircuit\n";
}

void testExceptionPropagation() {
    Router router;
    
    router.use([](const HttpRequest& req, const NextHandler& next) {
        return next(req);
    });
    
    router.get("/crash", [](const HttpRequest&) -> HttpResponse {
        throw std::runtime_error("Crash!");
    });
    
    HttpRequest req;
    req.setMethod(HttpMethod::GET);
    req.setPath("/crash");
    HttpResponse res = router.dispatch(req);
    
    assert(res.getStatusCode() == 500);
    std::cout << "[PASS] testExceptionPropagation\n";
}

void testConcurrentMiddleware() {
    Router router;
    
    // Use thread-local variable to ensure no data races and independent execution
    router.use([](const HttpRequest& req, const NextHandler& next) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        return next(req);
    });
    
    router.get("/concurrent", [](const HttpRequest&) {
        return HttpResponse::ok("Concurrent");
    });
    
    auto worker = [&]() {
        for(int i = 0; i < 50; ++i) {
            HttpRequest req;
            req.setMethod(HttpMethod::GET);
            req.setPath("/concurrent");
            HttpResponse res = router.dispatch(req);
            assert(res.getStatusCode() == 200);
        }
    };
    
    std::vector<std::thread> threads;
    for(int i = 0; i < 4; ++i) {
        threads.emplace_back(worker);
    }
    
    for(auto& t : threads) {
        t.join();
    }
    
    std::cout << "[PASS] testConcurrentMiddleware\n";
}

int main() {
    std::cout << "Starting Middleware Tests...\n";
    try {
        testSingleMiddleware();
        testMiddlewareOrdering();
        testMiddlewareModifyingResponse();
        testMiddlewareShortCircuit();
        testExceptionPropagation();
        testConcurrentMiddleware();
        
        std::cout << "All Middleware tests passed successfully!\n";
    } catch(const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
