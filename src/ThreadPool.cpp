#include "ThreadPool.h"
#include "Logger.h"
#include "Metrics.h"
#include <string>

ThreadPool::ThreadPool(std::size_t threadCount) {
    for (std::size_t i = 0; i < threadCount; ++i) {
        m_workers.emplace_back(&ThreadPool::workerLoop, this);
    }
    
    Logger::getInstance().info("ThreadPool initialized with " + std::to_string(threadCount) + " workers");
}

ThreadPool::~ThreadPool() {
    Logger::getInstance().info("ThreadPool shutting down...");
    
    {
        std::unique_lock<std::mutex> lock(m_queueMutex);
        m_stop.store(true, std::memory_order_release);
    }
    
    m_condition.notify_all();

    for (std::thread& worker : m_workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    
    Logger::getInstance().info("Worker threads joined successfully");
}

void ThreadPool::workerLoop() {
    while (true) {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            
            m_condition.wait(lock, [this]() {
                return m_stop.load(std::memory_order_acquire) || !m_tasks.empty();
            });

            if (m_stop.load(std::memory_order_acquire) && m_tasks.empty()) {
                return;
            }

            task = std::move(m_tasks.front());
            m_tasks.pop();
        }

        task();
        
        m_pendingTasks.fetch_sub(1, std::memory_order_relaxed);
        m_completedTasks.fetch_add(1, std::memory_order_relaxed);
    }
}

std::size_t ThreadPool::pendingTasks() const {
    return m_pendingTasks.load(std::memory_order_relaxed);
}

std::size_t ThreadPool::completedTasks() const {
    return m_completedTasks.load(std::memory_order_relaxed);
}

std::size_t ThreadPool::workerCount() const {
    return m_workers.size();
}
