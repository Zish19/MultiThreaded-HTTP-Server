#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <cstddef>
#include <memory>
#include <utility>
#include <stdexcept>

class MoveOnlyTask {
    struct Concept {
        virtual ~Concept() = default;
        virtual void invoke() = 0;
    };

    template <typename F>
    struct Model : Concept {
        F f;
        Model(F&& func) : f(std::move(func)) {}
        void invoke() override { f(); }
    };

    std::unique_ptr<Concept> m_ptr;

public:
    MoveOnlyTask() = default;

    template <typename F>
    MoveOnlyTask(F&& f) : m_ptr(std::make_unique<Model<std::decay_t<F>>>(std::forward<F>(f))) {}

    MoveOnlyTask(MoveOnlyTask&&) noexcept = default;
    MoveOnlyTask& operator=(MoveOnlyTask&&) noexcept = default;

    MoveOnlyTask(const MoveOnlyTask&) = delete;
    MoveOnlyTask& operator=(const MoveOnlyTask&) = delete;

    void operator()() {
        if (m_ptr) m_ptr->invoke();
    }
};

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
            
            std::size_t currentDepth = m_tasks.size();
            std::size_t maxDepth = m_maxQueueDepth.load(std::memory_order_relaxed);
            while (currentDepth > maxDepth && !m_maxQueueDepth.compare_exchange_weak(maxDepth, currentDepth, std::memory_order_relaxed)) {
                // Loop until exchange succeeds or maxDepth is higher
            }

            m_pendingTasks.fetch_add(1, std::memory_order_relaxed);
        }
        m_condition.notify_one();
    }

    std::size_t pendingTasks() const noexcept;
    std::size_t completedTasks() const noexcept;
    std::size_t workerCount() const noexcept;
    std::size_t maxObservedQueueDepth() const noexcept;

private:
    void workerLoop();

private:
    std::vector<std::thread> m_workers;
    std::queue<MoveOnlyTask> m_tasks;

    mutable std::mutex m_queueMutex;
    std::condition_variable m_condition;

    std::atomic<bool> m_stop{false};

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4324)
#endif
    alignas(64) std::atomic<std::size_t> m_pendingTasks{0};
    alignas(64) std::atomic<std::size_t> m_completedTasks{0};
    alignas(64) std::atomic<std::size_t> m_maxQueueDepth{0};
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
};
