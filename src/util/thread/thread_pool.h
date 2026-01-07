#pragma once

#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include "util/shine_define.h"
#include "jobs.h"

namespace shine::util
{
    class ThreadPool {
    public:
        explicit ThreadPool(u32 numThreads = 0);
        ~ThreadPool();

        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;

        // Submit a concrete job to the pool
        void Submit(job::Job job);

        u32 GetThreadCount() const { return static_cast<u32>(_workers.size()); }

        void WaitAll();

        static ThreadPool& Get();

    private:
        std::vector<std::thread> _workers;
        // Queue holds concrete variant jobs, not std::function
        std::queue<job::Job> _tasks;

        std::mutex _queueMutex;
        std::condition_variable _condition;
        std::atomic<bool> _stop{false};
        std::atomic<u32> _activeTasks{0};
        std::condition_variable _finished;
    };
}
