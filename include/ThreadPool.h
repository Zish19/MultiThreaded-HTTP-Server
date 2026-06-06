#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <cstddef>

class ThreadPool {
public:
    explicit ThreadPool(std::size_t threadCount);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    template<typename F>
    void enqueue(F&& task) {
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_tasks.emplace(std::forward<F>(task));
            m_pendingTasks.fetch_add(1, std::memory_order_relaxed);
        }
        m_condition.notify_one();
    }

    std::size_t pendingTasks() const;
    std::size_t completedTasks() const;
    std::size_t workerCount() const;

private:
    void workerLoop();

private:
    std::vector<std::thread> m_workers;
    std::queue<std::function<void()>> m_tasks;

    mutable std::mutex m_queueMutex;
    std::condition_variable m_condition;

    std::atomic<bool> m_stop{false};

    std::atomic<std::size_t> m_pendingTasks{0};
    std::atomic<std::size_t> m_completedTasks{0};
};
