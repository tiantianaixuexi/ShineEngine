#include "task_scheduler.h"
#include "job_executor.h"
#include <algorithm>

namespace shine::util
{
    TaskScheduler::TaskScheduler()
        : _threadPool(ThreadPool::Get())
    {
    }

    TaskHandle TaskScheduler::CreateTask(job::Job job) {
        std::lock_guard<std::mutex> lock(_mutex);
        
        auto node = std::make_unique<TaskNode>();
        node->job = std::move(job);
        node->remainingDeps = 0;
        
        u32 id = _nextId++;
        if (id >= _tasks.size()) {
            _tasks.resize(id + 1);
        }
        _tasks[id] = std::move(node);
        
        return TaskHandle{id, true};
    }

    void TaskScheduler::AddDependency(TaskHandle task, TaskHandle dependsOn) {
        if (!task.valid || !dependsOn.valid) return;
        
        std::lock_guard<std::mutex> lock(_mutex);
        
        if (task.id >= _tasks.size() || dependsOn.id >= _tasks.size()) return;
        
        auto& taskNode = _tasks[task.id];
        auto& depNode = _tasks[dependsOn.id];
        
        if (!taskNode || !depNode) return;
        
        taskNode->dependencies.push_back(dependsOn.id);
        depNode->dependents.push_back(task.id);
        ++taskNode->remainingDeps;
    }

    void TaskScheduler::Run(TaskHandle task) {
        if (!task.valid) return;
        
        std::lock_guard<std::mutex> lock(_mutex);
        
        if (task.id >= _tasks.size() || !_tasks[task.id]) return;
        
        auto& node = _tasks[task.id];
        if (node->remainingDeps == 0 && !node->completed) {
            // Change: Submit JobExecuteTaskNode
            _threadPool.Submit(job::JobExecuteTaskNode{task.id});
        }
    }

    void TaskScheduler::RunAll() {
        std::lock_guard<std::mutex> lock(_mutex);
        
        for (u32 id = 0; id < _tasks.size(); ++id) {
            if (_tasks[id] && _tasks[id]->remainingDeps == 0 && !_tasks[id]->completed) {
                // Change: Submit JobExecuteTaskNode
                _threadPool.Submit(job::JobExecuteTaskNode{id});
            }
        }
    }

    void TaskScheduler::Wait(TaskHandle task) {
        if (!task.valid) return;
        
        while (true) {
            {
                std::lock_guard<std::mutex> lock(_mutex);
                if (task.id < _tasks.size() && _tasks[task.id] && _tasks[task.id]->completed) {
                    return;
                }
            }
            std::this_thread::yield();
        }
    }

    void TaskScheduler::WaitAll() {
        _threadPool.WaitAll();
    }

    void TaskScheduler::ExecuteTask(u32 id) {
        // This function is running on a worker thread (invoked by JobExecutor)
        // or potentially on main thread if we had a sync run mode (not here).

        job::Job* currentJobPtr = nullptr;
        
        // Scope to retrieve job data
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (id >= _tasks.size() || !_tasks[id]) return;
            // We shouldn't move the node out, just access the job.
            // But to avoid holding lock during execution, we should copy the job if possible
            // or execute while holding lock? No, executing while holding lock is bad.
            // But std::variant is value type. We can copy it if it's copyable.
            // Assuming jobs are lightweight.
            currentJobPtr = &(_tasks[id]->job);
        }
        
        // Execute the job
        // Note: We use a fresh executor here to visit the *inner* job.
        if (currentJobPtr) {
            job::JobExecutor executor;
            std::visit(executor, *currentJobPtr);
        }
        
        // Update completion status and trigger dependents
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (!_tasks[id]) return;
            
            _tasks[id]->completed = true;
            
            auto& currentNode = _tasks[id];
            for (u32 depId : currentNode->dependents) {
                if (depId < _tasks.size() && _tasks[depId]) {
                    --_tasks[depId]->remainingDeps;
                    if (_tasks[depId]->remainingDeps == 0 && !_tasks[depId]->completed) {
                        // Submit next job
                        _threadPool.Submit(job::JobExecuteTaskNode{depId});
                    }
                }
            }
        }
    }

    TaskScheduler& TaskScheduler::Get() {
        static TaskScheduler instance;
        return instance;
    }
}
