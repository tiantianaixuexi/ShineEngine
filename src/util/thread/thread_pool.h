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

    class ThreadPool {
    public:
        explicit ThreadPool(u32 numThreads = 0);
        ~ThreadPool();

        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;

        template<typename F, typename... Args>
    	requires std::invocable<F, Args...>
        auto Enqueue(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F,Args...>>;

        u32 GetThreadCount() const { return static_cast<u32>(_workers.size()); }

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
        requires std::invocable<F, Args...>
    auto ThreadPool::Enqueue(F&& f, Args&&... args)   -> std::future<std::invoke_result_t<F, Args...>>
	{
        using ReturnType =  std::invoke_result_t<F,Args...>;

        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            [fn = std::forward<F>(f),
            ... as = std::forward<Args>(args)]() mutable -> ReturnType
            {
                if constexpr (std::is_void_v<ReturnType>) {
                    std::invoke(std::move(fn), std::move(as)...);
                }
                else {
                    return std::invoke(std::move(fn), std::move(as)...);
                }
            }
        );

        auto future = task->get_future();
        {
            std::unique_lock lock(_queueMutex);
            if (_stop)
                throw std::runtime_error("enqueue on stopped ThreadPool");

            _tasks.emplace([task] { (*task)(); });
            ++_activeTasks;
        }
        _condition.notify_one();
        return future;
    }
}
