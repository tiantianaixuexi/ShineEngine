#include "thread_pool.h"
#include <thread>

namespace shine::util
{

    ThreadPool::ThreadPool(u32 numThreads) {
        if (numThreads == 0) {
            numThreads = std::thread::hardware_concurrency();
            if (numThreads == 0) numThreads = 4;
        }

        for (u32 i = 0; i < numThreads; ++i) {
            _workers.emplace_back([this] {
                for (;;) {
                    std::function<void()> task;

                    {
                        std::unique_lock<std::mutex> lock(this->_queueMutex);
                        this->_condition.wait(lock, [this] {
                            return this->_stop || !this->_tasks.empty();
                        });

                        if (this->_stop && this->_tasks.empty()) {
                            return;
                        }

                        task = std::move(this->_tasks.front());
                        this->_tasks.pop();
                    }

                    task();
                    
                    {
                        std::unique_lock<std::mutex> lock(this->_queueMutex);
                        _activeTasks--;
                        if (_activeTasks == 0) {
                            _finished.notify_all();
                        }
                    }
                }
            });
        }
    }

    ThreadPool::~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(_queueMutex);
            _stop = true;
        }
        _condition.notify_all();

        for (std::thread& worker : _workers) {
            worker.join();
        }
    }

    void ThreadPool::WaitAll() {
        std::unique_lock<std::mutex> lock(_queueMutex);
        _finished.wait(lock, [this] {
            return _tasks.empty() && _activeTasks == 0;
        });
    }

    ThreadPool& ThreadPool::Get() {
        static ThreadPool instance;
        return instance;
    }
}
