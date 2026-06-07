#include "FileCache.h"
#include "Metrics.h"
#include <iostream>
#include <cassert>
#include <fstream>
#include <filesystem>
#include <thread>
#include <vector>

void createTestFile(const std::string& path, const std::string& content) {
    std::ofstream out(path, std::ios::binary);
    out << content;
    out.flush();
}

void testCacheHitMiss() {
    FileCache cache(1024);
    std::string path = "test_hit_miss.txt";
    createTestFile(path, "hello world");

    // Miss
    auto f1 = cache.getFile(path);
    assert(f1.has_value());
    assert(f1->content == "hello world");
    assert(cache.getCurrentSizeBytes() == 11);

    // Hit
    auto f2 = cache.getFile(path);
    assert(f2.has_value());
    assert(f2->content == "hello world");

    std::filesystem::remove(path);
    std::cout << "[PASS] testCacheHitMiss\n";
}

void testStaleEntry() {
    FileCache cache(1024);
    std::string path = "test_stale.txt";
    createTestFile(path, "v1");

    cache.getFile(path); // caches v1

    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // sleep to ensure file_time_type changes

    createTestFile(path, "v2"); // overwrite
    
    // Should detect change, miss, and reload
    auto f2 = cache.getFile(path);
    assert(f2.has_value());
    assert(f2->content == "v2");

    std::filesystem::remove(path);
    std::cout << "[PASS] testStaleEntry\n";
}

void testLRUEviction() {
    // 10 bytes cache
    FileCache cache(10);
    
    createTestFile("f1.txt", "12345"); // 5 bytes
    createTestFile("f2.txt", "12345"); // 5 bytes
    createTestFile("f3.txt", "12");    // 2 bytes

    cache.getFile("f1.txt"); // size: 5
    cache.getFile("f2.txt"); // size: 10
    
    // Getting f3 requires 2 bytes. Cache is 10. Max is 10.
    // LRU is f1. f1 should be evicted.
    cache.getFile("f3.txt"); // size: 10 - 5 + 2 = 7

    assert(cache.getCurrentSizeBytes() == 7);

    std::filesystem::remove("f1.txt");
    std::filesystem::remove("f2.txt");
    std::filesystem::remove("f3.txt");
    std::cout << "[PASS] testLRUEviction\n";
}

void testFileTooLarge() {
    FileCache cache(10); // max 10 bytes
    createTestFile("large.txt", "123456789012345"); // 15 bytes

    auto f1 = cache.getFile("large.txt");
    assert(f1.has_value());
    assert(f1->content == "123456789012345");
    // Should not cache it
    assert(cache.getCurrentSizeBytes() == 0);

    std::filesystem::remove("large.txt");
    std::cout << "[PASS] testFileTooLarge\n";
}

void testConcurrency() {
    FileCache cache(1024);
    createTestFile("conc.txt", "data");

    auto worker = [&cache]() {
        for(int i = 0; i < 50; ++i) {
            auto f = cache.getFile("conc.txt");
            assert(f.has_value());
        }
    };

    std::vector<std::thread> threads;
    for(int i = 0; i < 8; ++i) {
        threads.emplace_back(worker);
    }
    for(auto& t : threads) {
        t.join();
    }

    std::filesystem::remove("conc.txt");
    std::cout << "[PASS] testConcurrency\n";
}

int main() {
    std::cout << "Starting FileCache Tests...\n";
    try {
        testCacheHitMiss();
        testStaleEntry();
        testLRUEviction();
        testFileTooLarge();
        testConcurrency();
        std::cout << "All FileCache tests passed successfully!\n";
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
