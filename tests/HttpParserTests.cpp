#include "HttpParser.h"
#include "HttpParseException.h"
#include <iostream>
#include <cassert>

void expectThrow(std::string_view reqStr, const char* testName) {
    bool thrown = false;
    try {
        HttpParser::parse(reqStr);
    } catch (const HttpParseException&) {
        thrown = true;
    }
    assert(thrown && "Expected exception was not thrown");
    std::cout << "[PASS] " << testName << "\n";
}

void testHttpParserValidGet() {
    std::string reqStr = "GET /index.html HTTP/1.1\r\nHost: localhost\r\n\r\n";
    HttpRequest req = HttpParser::parse(reqStr);
    assert(req.getMethod() == HttpMethod::GET);
    assert(req.getPath() == "/index.html");
    assert(req.getVersion() == "HTTP/1.1");
    auto host = req.getHeader("host");
    assert(host && host.value() == "localhost");
    assert(req.getBody().empty());
    std::cout << "[PASS] testHttpParserValidGet\n";
}

void testHttpParserValidPostWithLF() {
    // Using \n instead of \r\n
    std::string reqStr = "POST /api/login HTTP/1.0\nHost: example.com\nContent-Length: 5\n\nadmin";
    HttpRequest req = HttpParser::parse(reqStr);
    assert(req.getMethod() == HttpMethod::POST);
    assert(req.getPath() == "/api/login");
    assert(req.getVersion() == "HTTP/1.0");
    assert(req.contentLength() == 5);
    assert(req.getBody() == "admin");
    std::cout << "[PASS] testHttpParserValidPostWithLF\n";
}

void testHttpParserWhitespaceTrim() {
    std::string reqStr = "GET    /spaced    HTTP/1.1\r\nHeader:   padded_value   \r\n\r\n";
    HttpRequest req = HttpParser::parse(reqStr);
    assert(req.getMethod() == HttpMethod::GET);
    assert(req.getPath() == "/spaced");
    assert(req.getVersion() == "HTTP/1.1");
    auto headerOpt = req.getHeader("header");
    assert(headerOpt && headerOpt.value() == "padded_value");
    std::cout << "[PASS] testHttpParserWhitespaceTrim\n";
}

void testHttpParserEmptyRequest() {
    expectThrow("", "testHttpParserEmptyRequest");
}

void testHttpParserMissingSeparator() {
    expectThrow("GET / HTTP/1.1\r\nHost: localhost\r\n", "testHttpParserMissingSeparator");
}

void testHttpParserInvalidMethod() {
    expectThrow("UNKNOWN_METHOD / HTTP/1.1\r\n\r\n", "testHttpParserInvalidMethod");
}

void testHttpParserInvalidVersion() {
    expectThrow("GET / HTTP/2.0\r\n\r\n", "testHttpParserInvalidVersion");
    expectThrow("GET / ABC\r\n\r\n", "testHttpParserInvalidVersion (ABC)");
}

void testHttpParserMissingColon() {
    expectThrow("GET / HTTP/1.1\r\nHost localhost\r\n\r\n", "testHttpParserMissingColon");
}

void testHttpParserDuplicateHeader() {
    expectThrow("GET / HTTP/1.1\r\nHost: a\r\nHost: b\r\n\r\n", "testHttpParserDuplicateHeader");
}

void testHttpParserHugeHeaderSize() {
    std::string hugeHeader(HttpParser::MAX_SINGLE_HEADER_SIZE + 1, 'x');
    std::string reqStr = "GET / HTTP/1.1\r\nHeader: " + hugeHeader + "\r\n\r\n";
    expectThrow(reqStr, "testHttpParserHugeHeaderSize");
}

void testHttpParserTooManyHeaders() {
    std::string reqStr = "GET / HTTP/1.1\r\n";
    for(size_t i = 0; i < HttpParser::MAX_HEADER_COUNT + 1; ++i) {
        reqStr += "H" + std::to_string(i) + ": val\r\n";
    }
    reqStr += "\r\n";
    expectThrow(reqStr, "testHttpParserTooManyHeaders");
}

void testHttpParserTruncatedBody() {
    std::string reqStr = "POST / HTTP/1.1\r\nContent-Length: 10\r\n\r\n12345";
    expectThrow(reqStr, "testHttpParserTruncatedBody");
}

void testHttpParserInvalidContentLength() {
    expectThrow("POST / HTTP/1.1\r\nContent-Length: -1\r\n\r\n", "testHttpParserInvalidContentLength (negative)");
    expectThrow("POST / HTTP/1.1\r\nContent-Length: abc\r\n\r\n", "testHttpParserInvalidContentLength (abc)");
    expectThrow("POST / HTTP/1.1\r\nContent-Length: 9999999999999999999999999\r\n\r\n", "testHttpParserInvalidContentLength (overflow)");
}

void testHttpParserInvalidPath() {
    expectThrow("GET no_slash HTTP/1.1\r\n\r\n", "testHttpParserInvalidPath");
}

void testHttpParserRoundTripCanonical() {
    std::string rawRequest = 
        "POST /login HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "admin";
        
    HttpRequest req = HttpParser::parse(rawRequest);
    
    assert(req.getMethod() == HttpMethod::POST);
    assert(req.getPath() == "/login");
    assert(req.getVersion() == "HTTP/1.1");
    auto host = req.getHeader("host");
    assert(host && host.value() == "localhost");
    assert(req.contentLength() == 5);
    assert(req.getBody() == "admin");
    
    std::cout << "[PASS] testHttpParserRoundTripCanonical\n";
}

void testHttpParserMixedNewlines() {
    // The parser checks separator. If it finds \r\n\r\n, it assumes \r\n. If headers use mixed, it should fail or correctly identify tokens.
    // Given the simple parser, if we use \r\n\r\n but a header has \n, the string operations will fail or leave trailing \r.
    std::string reqStr = "GET / HTTP/1.1\nHost: a\r\n\n";
    expectThrow(reqStr, "testHttpParserMixedNewlines");
}

int main() {
    std::cout << "Starting HttpParser Tests...\n";
    
    testHttpParserValidGet();
    testHttpParserValidPostWithLF();
    testHttpParserWhitespaceTrim();
    testHttpParserEmptyRequest();
    testHttpParserMissingSeparator();
    testHttpParserInvalidMethod();
    testHttpParserInvalidVersion();
    testHttpParserMissingColon();
    testHttpParserDuplicateHeader();
    testHttpParserHugeHeaderSize();
    testHttpParserTooManyHeaders();
    testHttpParserTruncatedBody();
    testHttpParserInvalidContentLength();
    testHttpParserInvalidPath();
    testHttpParserRoundTripCanonical();
    testHttpParserMixedNewlines();
    
    std::cout << "All HttpParser tests passed successfully!\n";
    return 0;
}
