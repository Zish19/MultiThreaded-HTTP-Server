#include "HttpRequest.h"
#include <iostream>
#include <cassert>

void testHttpMethodConversion() {
    assert(HttpMethodUtils::toString(HttpMethod::GET) == "GET");
    assert(HttpMethodUtils::toString(HttpMethod::DELETE_) == "DELETE");
    
    assert(HttpMethodUtils::fromString("GET") == HttpMethod::GET);
    assert(HttpMethodUtils::fromString("get") == HttpMethod::GET);
    assert(HttpMethodUtils::fromString("pOsT") == HttpMethod::POST);
    assert(HttpMethodUtils::fromString("UNKNOWN_VERB") == HttpMethod::UNKNOWN);
    
    std::cout << "[PASS] testHttpMethodConversion\n";
}

void testHttpRequestGettersSetters() {
    HttpRequest req;
    req.setMethod(HttpMethod::POST);
    req.setPath("/api/data");
    req.setVersion("HTTP/1.1");
    req.setBody("test_body");
    
    assert(req.getMethod() == HttpMethod::POST);
    assert(req.getPath() == "/api/data");
    assert(req.getVersion() == "HTTP/1.1");
    assert(req.getBody() == "test_body");
    
    std::cout << "[PASS] testHttpRequestGettersSetters\n";
}

void testHttpRequestHeaders() {
    HttpRequest req;
    req.addHeader("Content-Type", "application/json");
    req.addHeader("X-Custom-Header", "value1");
    
    // Case insensitivity
    assert(req.hasHeader("content-type") == true);
    assert(req.hasHeader("CONTENT-TYPE") == true);
    assert(req.hasHeader("Content-Type") == true);
    
    auto val = req.getHeader("content-type");
    assert(val.has_value() && val.value() == "application/json");
    
    auto missing = req.getHeader("missing-header");
    assert(!missing.has_value());
    
    std::cout << "[PASS] testHttpRequestHeaders\n";
}

void testHttpRequestKeepAlive() {
    HttpRequest req;
    req.setVersion("HTTP/1.1");
    assert(req.isKeepAlive() == true); // Default for 1.1
    
    req.addHeader("Connection", "close");
    assert(req.isKeepAlive() == false);
    
    HttpRequest req2;
    req2.setVersion("HTTP/1.0");
    assert(req2.isKeepAlive() == false); // Default for 1.0
    
    req2.addHeader("Connection", "Keep-Alive");
    assert(req2.isKeepAlive() == true);
    
    std::cout << "[PASS] testHttpRequestKeepAlive\n";
}

void testHttpRequestContentLength() {
    HttpRequest req;
    assert(req.contentLength() == 0);
    
    req.addHeader("Content-Length", "1024");
    assert(req.contentLength() == 1024);
    
    req.addHeader("Content-Length", "invalid");
    assert(req.contentLength() == 0);
    
    std::cout << "[PASS] testHttpRequestContentLength\n";
}

void testHttpRequestMoveSemantics() {
    HttpRequest req;
    req.setPath("/move");
    req.setBody(std::string(1024 * 1024, 'a')); // Large body to ensure it's not copied
    req.addHeader("X-Test", "value");
    
    // Move Constructor
    HttpRequest req2 = std::move(req);
    assert(req2.getPath() == "/move");
    assert(req2.getBody().size() == 1024 * 1024);
    assert(req2.hasHeader("X-Test") == true);
    
    // req should be moved-from (STL guarantees valid but unspecified state, usually empty)
    assert(req.getPath().empty());
    assert(req.getBody().empty());
    assert(req.getHeaders().empty());
    
    // Move Assignment
    HttpRequest req3;
    req3 = std::move(req2);
    
    assert(req3.getPath() == "/move");
    assert(req3.getBody().size() == 1024 * 1024);
    assert(req3.hasHeader("x-test") == true);
    
    assert(req2.getPath().empty());
    assert(req2.getBody().empty());
    assert(req2.getHeaders().empty());

    std::cout << "[PASS] testHttpRequestMoveSemantics\n";
}

void testHttpRequestEmptyBody() {
    HttpRequest req;
    assert(req.getBody().empty());
    assert(req.contentLength() == 0);
    std::cout << "[PASS] testHttpRequestEmptyBody\n";
}

void testHttpRequestLargeBody() {
    HttpRequest req;
    std::string largeBody(5 * 1024 * 1024, 'X'); // 5MB
    req.setBody(largeBody);
    assert(req.getBody().size() == 5 * 1024 * 1024);
    assert(req.getBody()[0] == 'X');
    assert(req.getBody().back() == 'X');
    std::cout << "[PASS] testHttpRequestLargeBody\n";
}

int main() {
    std::cout << "Starting HttpRequest Tests...\n";
    
    testHttpMethodConversion();
    testHttpRequestGettersSetters();
    testHttpRequestHeaders();
    testHttpRequestKeepAlive();
    testHttpRequestContentLength();
    testHttpRequestMoveSemantics();
    testHttpRequestEmptyBody();
    testHttpRequestLargeBody();
    
    std::cout << "All HttpRequest tests passed successfully!\n";
    return 0;
}
