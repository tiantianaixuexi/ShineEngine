#include "task_scheduler.h"
#include <algorithm>

namespace shine::util
{
    TaskScheduler::TaskScheduler()
        : _threadPool(ThreadPool::Get())
    {
    }

    TaskHandle TaskScheduler::CreateTask(std::function<void()> task) {
        std::lock_guard<std::mutex> lock(_mutex);
        
        auto node = std::make_unique<TaskNode>();
        node->task = std::move(task);
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
            _threadPool.Enqueue([this, id = task.id]() {
                ExecuteTask(id);
            });
        }
    }

    void TaskScheduler::RunAll() {
        std::lock_guard<std::mutex> lock(_mutex);
        
        for (u32 id = 0; id < _tasks.size(); ++id) {
            if (_tasks[id] && _tasks[id]->remainingDeps == 0 && !_tasks[id]->completed) {
                _threadPool.Enqueue([this, id]() {
                    ExecuteTask(id);
                });
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
        std::unique_ptr<TaskNode> node;
        
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (id >= _tasks.size() || !_tasks[id]) return;
            node = std::move(_tasks[id]);
        }
        
        if (node->task) {
            node->task();
        }
        
        node->completed = true;
        
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _tasks[id] = std::move(node);
            
            auto& currentNode = _tasks[id];
            for (u32 depId : currentNode->dependents) {
                if (depId < _tasks.size() && _tasks[depId]) {
                    _tasks[depId]->remainingDeps--;
                    if (_tasks[depId]->remainingDeps == 0 && !_tasks[depId]->completed) {
                        u32 nextId = depId;
                        _threadPool.Enqueue([this, nextId]() {
                            ExecuteTask(nextId);
                        });
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
