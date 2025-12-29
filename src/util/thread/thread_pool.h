#pragma once

#include <vector>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <future>
#include <memory>
#include "util/shine_define.h"

namespace shine::util
{
#if SHINE_HAS_STD_JTHREAD
    class ThreadPool {
    public:
        explicit ThreadPool(u32 numThreads = 0);
        ~ThreadPool();

        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;

        template<typename F, typename... Args>
        auto Enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>;

        u32 GetThreadCount() const { return _workers.size(); }

        void WaitAll();

        static ThreadPool& Get();

    private:
        std::vector<std::thread> _workers;
        std::queue<std::function<void()>> _tasks;

        std::mutex _queueMutex;
        std::condition_variable _condition;
        std::atomic<bool> _stop{false};
        std::atomic<u32> _activeTasks{0};
        std::condition_variable _finished;
    };

    template<typename F, typename... Args>
    auto ThreadPool::Enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type> {
        using ReturnType = typename std::result_of<F(Args...)>::type;

        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<ReturnType> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(_queueMutex);
            if (_stop) {
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }

            _tasks.emplace([task]() { (*task)(); });
            _activeTasks++;
        }
        _condition.notify_one();
        return res;
    }
#else
    class ThreadPool {
    public:
        explicit ThreadPool(u32 numThreads = 0) {}
        ~ThreadPool() = default;

        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;

        template<typename F, typename... Args>
        auto Enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type> {
            using ReturnType = typename std::result_of<F(Args...)>::type;
            std::promise<ReturnType> promise;
            auto future = promise.get_future();
            
            try {
                if constexpr (std::is_void_v<ReturnType>) {
                    std::forward<F>(f)(std::forward<Args>(args)...);
                    promise.set_value();
                } else {
                    promise.set_value(std::forward<F>(f)(std::forward<Args>(args)...));
                }
            } catch (...) {
                promise.set_exception(std::current_exception());
            }
            
            return future;
        }

        u32 GetThreadCount() const { return 1; }

        void WaitAll() {}

        static ThreadPool& Get() {
            static ThreadPool instance;
            return instance;
        }
    };
#endif
}
