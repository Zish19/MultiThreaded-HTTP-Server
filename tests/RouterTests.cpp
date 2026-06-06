#include "Router.h"
#include "HttpRequest.h"
#include <iostream>
#include <cassert>
#include <stdexcept>
#include <thread>
#include <vector>

void testExistingRoute() {
    Router router;
    router.get("/health", [](const HttpRequest&) {
        return HttpResponse::ok("OK");
    });

    HttpRequest req;
    req.setMethod(HttpMethod::GET);
    req.setPath("/health");

    HttpResponse res = router.dispatch(req);
    assert(res.getStatusCode() == 200);
    
    std::cout << "[PASS] testExistingRoute\n";
}

void testMissingRoute() {
    Router router;
    router.get("/health", [](const HttpRequest&) {
        return HttpResponse::ok("OK");
    });

    HttpRequest req;
    req.setMethod(HttpMethod::GET);
    req.setPath("/missing");

    HttpResponse res = router.dispatch(req);
    assert(res.getStatusCode() == 404);
    
    std::cout << "[PASS] testMissingRoute\n";
}

void testWrongMethod() {
    Router router;
    router.get("/health", [](const HttpRequest&) {
        return HttpResponse::ok("OK");
    });

    HttpRequest req;
    req.setMethod(HttpMethod::POST);
    req.setPath("/health");

    HttpResponse res = router.dispatch(req);
    assert(res.getStatusCode() == 405);
    
    std::cout << "[PASS] testWrongMethod\n";
}

void testMultipleRoutes() {
    Router router;
    router.get("/route1", [](const HttpRequest&) { return HttpResponse::ok("1"); });
    router.post("/route2", [](const HttpRequest&) { return HttpResponse::ok("2"); });

    HttpRequest req1;
    req1.setMethod(HttpMethod::GET);
    req1.setPath("/route1");
    assert(router.dispatch(req1).getStatusCode() == 200);

    HttpRequest req2;
    req2.setMethod(HttpMethod::POST);
    req2.setPath("/route2");
    assert(router.dispatch(req2).getStatusCode() == 200);
    
    std::cout << "[PASS] testMultipleRoutes\n";
}

void testRouteOverwritePrevention() {
    Router router;
    router.get("/", [](const HttpRequest&) { return HttpResponse::ok(); });
    
    bool threw = false;
    try {
        router.get("/", [](const HttpRequest&) { return HttpResponse::ok(); });
    } catch (const std::runtime_error&) {
        threw = true;
    }
    assert(threw);
    
    std::cout << "[PASS] testRouteOverwritePrevention\n";
}

void testHandlerExecution() {
    Router router;
    router.get("/crash", [](const HttpRequest&) -> HttpResponse {
        throw std::runtime_error("simulated crash");
    });

    HttpRequest req;
    req.setMethod(HttpMethod::GET);
    req.setPath("/crash");

    HttpResponse res = router.dispatch(req);
    assert(res.getStatusCode() == 500);
    
    std::cout << "[PASS] testHandlerExecution\n";
}

void testConcurrentDispatch() {
    Router router;
    router.get("/health", [](const HttpRequest&) {
        return HttpResponse::ok("OK");
    });

    constexpr int NUM_THREADS = 8;
    constexpr int REQUESTS_PER_THREAD = 1000;

    auto worker = [&router]() {
        for (int i = 0; i < REQUESTS_PER_THREAD; ++i) {
            HttpRequest req;
            req.setMethod(HttpMethod::GET);
            req.setPath("/health");
            HttpResponse res = router.dispatch(req);
            assert(res.getStatusCode() == 200);
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back(worker);
    }

    for (auto& t : threads) {
        t.join();
    }
    
    std::cout << "[PASS] testConcurrentDispatch\n";
}

int main() {
    std::cout << "Starting Router Tests...\n";

    testExistingRoute();
    testMissingRoute();
    testWrongMethod();
    testMultipleRoutes();
    testRouteOverwritePrevention();
    testHandlerExecution();
    testConcurrentDispatch();

    std::cout << "All Router tests passed successfully!\n";
    return 0;
}
