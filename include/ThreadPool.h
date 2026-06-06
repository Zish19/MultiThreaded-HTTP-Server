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
            if (m_stop.load(std::memory_order_acquire)) {
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }
            m_tasks.emplace(std::forward<F>(task));
            m_pendingTasks.fetch_add(1, std::memory_order_relaxed);
        }
        m_condition.notify_one();
    }

    std::size_t pendingTasks() const noexcept;
    std::size_t completedTasks() const noexcept;
    std::size_t workerCount() const noexcept;

private:
    void workerLoop();

private:
    std::vector<std::thread> m_workers;
    std::queue<std::function<void()>> m_tasks;

    mutable std::mutex m_queueMutex;
    std::condition_variable m_condition;

    std::atomic<bool> m_stop{false};

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4324)
#endif
    alignas(64) std::atomic<std::size_t> m_pendingTasks{0};
    alignas(64) std::atomic<std::size_t> m_completedTasks{0};
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
};
