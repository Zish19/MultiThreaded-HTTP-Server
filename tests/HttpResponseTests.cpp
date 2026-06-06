#include "HttpResponse.h"
#include <iostream>
#include <cassert>

void testHttpResponseGettersSetters() {
    HttpResponse res;
    res.setStatusCode(201);
    res.setStatusText("Created");
    res.setBody("Success");
    
    assert(res.getStatusCode() == 201);
    assert(res.getStatusText() == "Created");
    assert(res.getBody() == "Success");
    
    std::cout << "[PASS] testHttpResponseGettersSetters\n";
}

void testHttpResponseToStringAndContentLength() {
    HttpResponse res;
    res.setStatusCode(200);
    res.setStatusText("OK");
    res.setBody("Hello");
    // Ensure user-supplied content-length is overridden
    res.addHeader("Content-Length", "999");
    
    std::string expected = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "Hello";
        
    assert(res.toString() == expected);
    
    std::cout << "[PASS] testHttpResponseToStringAndContentLength\n";
}

void testHttpResponseFactories() {
    // 200 OK
    HttpResponse okRes = HttpResponse::ok("Data");
    std::string expectedOk = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 4\r\n"
        "\r\n"
        "Data";
    assert(okRes.toString() == expectedOk);
    
    // 400 Bad Request
    HttpResponse badRes = HttpResponse::badRequest();
    std::string expectedBad = 
        "HTTP/1.1 400 Bad Request\r\n"
        "Content-Length: 11\r\n"
        "\r\n"
        "Bad Request";
    assert(badRes.toString() == expectedBad);
    
    // 404 Not Found
    HttpResponse notFoundRes = HttpResponse::notFound();
    std::string expectedNotFound = 
        "HTTP/1.1 404 Not Found\r\n"
        "Content-Length: 9\r\n"
        "\r\n"
        "Not Found";
    assert(notFoundRes.toString() == expectedNotFound);
    
    // 405 Method Not Allowed
    HttpResponse methodNotAllowedRes = HttpResponse::methodNotAllowed();
    std::string expectedMethodNotAllowed = 
        "HTTP/1.1 405 Method Not Allowed\r\n"
        "Content-Length: 18\r\n"
        "\r\n"
        "Method Not Allowed";
    assert(methodNotAllowedRes.toString() == expectedMethodNotAllowed);
    
    // 500 Internal Server Error
    HttpResponse internalRes = HttpResponse::internalServerError();
    std::string expectedInternal = 
        "HTTP/1.1 500 Internal Server Error\r\n"
        "Content-Length: 21\r\n"
        "\r\n"
        "Internal Server Error";
    assert(internalRes.toString() == expectedInternal);
    
    std::cout << "[PASS] testHttpResponseFactories\n";
}

void testHttpResponseMoveSemantics() {
    HttpResponse res = HttpResponse::ok(std::string(1024 * 1024, 'B'));
    
    HttpResponse res2 = std::move(res);
    assert(res2.getStatusCode() == 200);
    assert(res2.getBody().size() == 1024 * 1024);
    
    assert(res.getBody().empty());
    
    HttpResponse res3;
    res3 = std::move(res2);
    assert(res3.getStatusCode() == 200);
    assert(res3.getBody().size() == 1024 * 1024);
    
    assert(res2.getBody().empty());
    
    std::cout << "[PASS] testHttpResponseMoveSemantics\n";
}

int main() {
    std::cout << "Starting HttpResponse Tests...\n";
    
    testHttpResponseGettersSetters();
    testHttpResponseToStringAndContentLength();
    testHttpResponseFactories();
    testHttpResponseMoveSemantics();
    
    std::cout << "All HttpResponse tests passed successfully!\n";
    return 0;
}
