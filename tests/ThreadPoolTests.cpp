#include "ThreadPool.h"
#include <iostream>
#include <cassert>
#include <chrono>
#include <atomic>
#include <thread>
#include <vector>

void testSingleTask() {
    ThreadPool pool(2);
    std::atomic<bool> executed{false};
    
    pool.enqueue([&executed]() {
        executed = true;
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    assert(executed == true);
    assert(pool.completedTasks() == 1);
    assert(pool.pendingTasks() == 0);
    std::cout << "[PASS] testSingleTask\n";
}

void testMultipleTasks() {
    ThreadPool pool(4);
    std::atomic<int> counter{0};
    const int taskCount = 10;
    
    for (int i = 0; i < taskCount; ++i) {
        pool.enqueue([&counter]() {
            counter.fetch_add(1, std::memory_order_relaxed);
        });
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    assert(counter == taskCount);
    assert(pool.completedTasks() == taskCount);
    assert(pool.pendingTasks() == 0);
    std::cout << "[PASS] testMultipleTasks\n";
}

void testConcurrentSubmission() {
    ThreadPool pool(4);
    std::atomic<int> counter{0};
    const int submitters = 4;
    const int tasksPerSubmitter = 25;
    
    std::vector<std::thread> submitterThreads;
    for (int i = 0; i < submitters; ++i) {
        submitterThreads.emplace_back([&pool, &counter, tasksPerSubmitter]() {
            for (int j = 0; j < tasksPerSubmitter; ++j) {
                pool.enqueue([&counter]() {
                    counter.fetch_add(1, std::memory_order_relaxed);
                });
            }
        });
    }
    
    for (auto& t : submitterThreads) {
        t.join();
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    assert(counter == submitters * tasksPerSubmitter);
    std::cout << "[PASS] testConcurrentSubmission\n";
}

void testGracefulShutdown() {
    std::atomic<int> counter{0};
    const int taskCount = 100;
    
    {
        ThreadPool pool(4);
        for (int i = 0; i < taskCount; ++i) {
            pool.enqueue([&counter]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(2)); 
                counter.fetch_add(1, std::memory_order_relaxed);
            });
        }
    }
    
    assert(counter == taskCount);
    std::cout << "[PASS] testGracefulShutdown\n";
}

void testStress() {
    ThreadPool pool(8);
    std::atomic<int> counter{0};
    const int taskCount = 10000;
    
    for (int i = 0; i < taskCount; ++i) {
        pool.enqueue([&counter]() {
            counter.fetch_add(1, std::memory_order_relaxed);
        });
    }
    
    while (pool.completedTasks() < taskCount) {
        std::this_thread::yield();
    }
    
    assert(counter == taskCount);
    assert(pool.pendingTasks() == 0);
    std::cout << "[PASS] testStress (" << taskCount << " tasks on 8 workers)\n";
}

int main() {
    std::cout << "Starting ThreadPool Tests...\n";
    
    testSingleTask();
    testMultipleTasks();
    testConcurrentSubmission();
    testGracefulShutdown();
    testStress();
    
    std::cout << "All ThreadPool tests passed successfully!\n";
    return 0;
}
