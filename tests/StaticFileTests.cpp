#undef NDEBUG
#include "StaticFileHandler.h"
#include "Config.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include <iostream>
#include <cassert>
#include <fstream>
#include <filesystem>
#include <thread>
#include <vector>

void setupTestFiles() {
    std::filesystem::create_directories("./public");
    
    // Existing file
    std::ofstream("public/index.html") << "<h1>Index</h1>";
    std::ofstream("public/style.css") << "body { color: red; }";
    
    // Empty file
    std::ofstream("public/empty.txt");
    
    // Binary file
    std::ofstream binFile("public/data.bin", std::ios::binary);
    char binData[] = {0x00, 0x01, 0x02, '\xFF'};
    binFile.write(binData, sizeof(binData));
    binFile.close();
    
    // Large file (~1MB)
    std::ofstream largeFile("public/large.bin", std::ios::binary);
    std::string largeData(1024 * 1024, 'A');
    largeFile.write(largeData.data(), largeData.size());
    largeFile.close();
}

void teardownTestFiles() {
    std::filesystem::remove_all("./public");
}

void testExistingFile() {
    HttpRequest req;
    req.setMethod(HttpMethod::GET);
    req.setPath("/style.css");
    
    HttpResponse res = StaticFileHandler::handle(req);
    assert(res.getStatusCode() == 200);
    assert(res.getBody() == "body { color: red; }");
    assert(res.getHeader("Content-Type").value() == "text/css");
    std::cout << "[PASS] testExistingFile\n";
}

void testMissingFile() {
    HttpRequest req;
    req.setMethod(HttpMethod::GET);
    req.setPath("/missing.html");
    
    HttpResponse res = StaticFileHandler::handle(req);
    assert(res.getStatusCode() == 404);
    std::cout << "[PASS] testMissingFile\n";
}

void testRootPath() {
    HttpRequest req;
    req.setMethod(HttpMethod::GET);
    req.setPath("/");
    
    HttpResponse res = StaticFileHandler::handle(req);
    assert(res.getStatusCode() == 200);
    assert(res.getBody() == "<h1>Index</h1>");
    assert(res.getHeader("Content-Type").value() == "text/html");
    std::cout << "[PASS] testRootPath\n";
}

void testMimeTypeLookup() {
    HttpRequest req;
    req.setMethod(HttpMethod::GET);
    req.setPath("/data.bin"); // .bin is not in our known list, should fallback
    
    HttpResponse res = StaticFileHandler::handle(req);
    assert(res.getStatusCode() == 200);
    assert(res.getHeader("Content-Type").value() == "application/octet-stream");
    std::cout << "[PASS] testMimeTypeLookup\n";
}

void testPathTraversalAttack() {
    HttpRequest req;
    req.setMethod(HttpMethod::GET);
    // Path traversal to exit public directory
    req.setPath("/../../CMakeLists.txt");
    
    HttpResponse res = StaticFileHandler::handle(req);
    // Should be forbidden
    assert(res.getStatusCode() == 403);
    std::cout << "[PASS] testPathTraversalAttack\n";
}

void testBinaryFileServing() {
    HttpRequest req;
    req.setMethod(HttpMethod::GET);
    req.setPath("/data.bin");
    
    HttpResponse res = StaticFileHandler::handle(req);
    assert(res.getStatusCode() == 200);
    const std::string& body = res.getBody();
    assert(body.size() == 4);
    assert(body[0] == 0x00 && body[1] == 0x01 && body[2] == 0x02 && body[3] == '\xFF');
    std::cout << "[PASS] testBinaryFileServing\n";
}

void testEmptyFile() {
    HttpRequest req;
    req.setMethod(HttpMethod::GET);
    req.setPath("/empty.txt");
    
    HttpResponse res = StaticFileHandler::handle(req);
    assert(res.getStatusCode() == 200);
    assert(res.getBody().empty());
    assert(res.getHeader("Content-Length").value() == "0");
    std::cout << "[PASS] testEmptyFile\n";
}

void testLargeFile() {
    HttpRequest req;
    req.setMethod(HttpMethod::GET);
    req.setPath("/large.bin");
    
    HttpResponse res = StaticFileHandler::handle(req);
    assert(res.getStatusCode() == 200);
    assert(res.getBody().size() == 1024 * 1024);
    assert(res.getHeader("Content-Length").value() == std::to_string(1024 * 1024));
    std::cout << "[PASS] testLargeFile\n";
}

void testConcurrentRequests() {
    auto worker = []() {
        for(int i = 0; i < 20; ++i) {
            HttpRequest req;
            req.setMethod(HttpMethod::GET);
            req.setPath("/style.css");
            HttpResponse res = StaticFileHandler::handle(req);
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
    
    std::cout << "[PASS] testConcurrentRequests\n";
}

int main() {
    std::cout << "Starting Static File Tests...\n";
    // Using default Config values (publicDirectory = "./public")
    
    setupTestFiles();
    try {
        testExistingFile();
        testMissingFile();
        testRootPath();
        testMimeTypeLookup();
        testPathTraversalAttack();
        testBinaryFileServing();
        testEmptyFile();
        testLargeFile();
        testConcurrentRequests();
        
        std::cout << "All Static File tests passed successfully!\n";
    } catch(const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        teardownTestFiles();
        return 1;
    }
    teardownTestFiles();
    return 0;
}
