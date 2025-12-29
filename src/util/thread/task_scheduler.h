#pragma once

#include <vector>
#include <functional>
#include <memory>
#include <atomic>
#include <mutex>
#include "util/shine_define.h"
#include "thread_pool.h"

namespace shine::util
{
    struct TaskHandle {
        u32 id;
        bool valid = false;
    };

    class TaskScheduler {
    public:
        TaskScheduler();
        ~TaskScheduler() = default;

        TaskHandle CreateTask(std::function<void()> task);
        void AddDependency(TaskHandle task, TaskHandle dependsOn);
        void Run(TaskHandle task);
        void RunAll();
        void Wait(TaskHandle task);
        void WaitAll();

        static TaskScheduler& Get();

    private:
        void ExecuteTask(u32 id);

        struct TaskNode {
            std::function<void()> task;
            std::vector<u32> dependencies;
            std::vector<u32> dependents;
            std::atomic<u32> remainingDeps{0};
            std::atomic<bool> completed{false};
        };

        u32 _nextId{0};
        std::vector<std::unique_ptr<TaskNode>> _tasks;
        std::mutex _mutex;
        ThreadPool& _threadPool;
    };
}
