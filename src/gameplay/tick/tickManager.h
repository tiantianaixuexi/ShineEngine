#pragma once

#include <array>
#include <vector>
#include <algorithm>
#include <cassert>
#include <mutex>

#include "tick_function.h"
#include "tick_types.h"

#if defined(SHINE_PLATFORM_WASM) || defined(__EMSCRIPTEN__)
    #include "util/thread/thread_pool.h"
#else
    #include "util/thread/task_scheduler.h"
#endif

namespace shine::gameplay::tick
{
    enum class EExecutionMode {
        SingleThreaded,
        MultiThreaded
    };

    class TickManager: public Subsystem {
    public:
        TickManager()
            : _fixedTimestep(1.0f / 60.0f)
            , _accumulator(0.0f)
            , _executionMode(EExecutionMode::SingleThreaded)
        {
        }

        void SetFixedTimestep(float dt) {
            _fixedTimestep = dt;
        }

        float GetFixedTimestep() const {
            return _fixedTimestep;
        }

        void SetExecutionMode(EExecutionMode mode) {
            _executionMode = mode;
        }

        EExecutionMode GetExecutionMode() const {
            return _executionMode;
        }

        void Register(TickFunction* fn) {
            std::lock_guard<std::mutex> lock(_mutex);
            auto& vec = _groups[static_cast<u32>(fn->group)];
            fn->execIndex = static_cast<u32>(vec.size());
            vec.push_back(fn);
            fn->SetRegistered(true);
            _dirty = true;
        }

        void Unregister(TickFunction* fn) {
            std::lock_guard<std::mutex> lock(_mutex);
            auto& vec = _groups[static_cast<u32>(fn->group)];
            const u32 idx = fn->execIndex;
            const u32 last = static_cast<u32>(vec.size()) - 1;

            if (idx != last) {
                vec[idx] = vec[last];
                vec[idx]->execIndex = idx;
            }
            vec.pop_back();
            fn->SetRegistered(false);
            _dirty = true;
        }

        void ExecutePhase(ETickGroup group, float dt) {
            std::lock_guard<std::mutex> lock(_mutex);
            if (_dirty)
                BuildExecOrder();

            const auto& order = _execOrder[static_cast<u32>(group)];
            
            if (_executionMode == EExecutionMode::SingleThreaded) {
                ExecutePhaseSingleThreaded(order, dt);
            } else {
                ExecutePhaseMultiThreaded(order, dt);
            }
        }

        void ExecuteAll(float dt) {
            _accumulator += dt;

            while (_accumulator >= _fixedTimestep) {
                ExecutePhase(ETickGroup::PrePhysics, _fixedTimestep);
                ExecutePhase(ETickGroup::Physics, _fixedTimestep);
                _accumulator -= _fixedTimestep;
            }

            ExecutePhase(ETickGroup::PostPhysics, dt);
            ExecutePhase(ETickGroup::Late, dt);
        }

    private:
        void ExecutePhaseSingleThreaded(const std::vector<TickFunction*>& order, float dt) {
            for (TickFunction* fn : order) {
                if (!fn->fn)
                    continue;

                if (fn->enable && !fn->enable->enabled)
                    continue;

                fn->accTime += dt;
                if (fn->interval > 0.f && fn->accTime < fn->interval)
                    continue;

                fn->accTime = 0.f;
                fn->fn(fn->userdata, dt);
            }
        }

#if defined(SHINE_PLATFORM_WASM) || defined(__EMSCRIPTEN__)
        void ExecutePhaseMultiThreaded(const std::vector<TickFunction*>& order, float dt) {
            ExecutePhaseSingleThreaded(order, dt);
        }
#else
        void ExecutePhaseMultiThreaded(const std::vector<TickFunction*>& order, float dt) {
            auto& pool = util::ThreadPool::Get();
            
            std::vector<std::vector<TickFunction*>> parallelGroups;
            std::vector<u32> indegree(order.size(), 0);

            for (size_t i = 0; i < order.size(); ++i) {
                for (const auto* dep : order[i]->dependencies) {
	                if (auto it = std::ranges::find(order, dep); it != order.end()) {
                        indegree[i]++;
                    }
                }
            }

            while (true) {
                std::vector<TickFunction*> readyGroup;
                std::vector<size_t> readyIndices;

                for (size_t i = 0; i < order.size(); ++i) {
                    if (indegree[i] == 0 && order[i]->fn && !(order[i]->enable && !order[i]->enable->enabled)) {
                        order[i]->accTime += dt;
                        if (order[i]->interval <= 0.f || order[i]->accTime >= order[i]->interval) {
                            order[i]->accTime = 0.f;
                            readyGroup.push_back(order[i]);
                            readyIndices.push_back(i);
                        }
                    }
                }

                if (readyGroup.empty()) {
                    bool allProcessed = true;
                    for (size_t i = 0; i < order.size(); ++i) {
                        if (indegree[i] != -1) {
                            allProcessed = false;
                            break;
                        }
                    }
                    if (allProcessed) break;
                    continue;
                }

                std::vector<std::future<void>> futures;
                for (auto* fn : readyGroup) {
                    futures.push_back(pool.Enqueue([fn, dt]() {
                        fn->fn(fn->userdata, dt);
                    }));
                }

                for (auto& future : futures) {
                    future.wait();
                }

                for (auto idx : readyIndices) {
                    indegree[idx] = -1;
                    for (size_t i = 0; i < order.size(); ++i) {
                        for (const auto* dep : order[i]->dependencies) {
                            if (dep == order[idx]) {
                                indegree[i]--;
                            }
                        }
                    }
                }
            }
        }
#endif

        void BuildExecOrder() {
            for (u32 g = 0; g < static_cast<u32>(ETickGroup::COUNT); ++g)
                BuildGroupExecOrder(static_cast<ETickGroup>(g));

            _dirty = false;
        }

        void BuildGroupExecOrder(ETickGroup group) {
            auto& src = _groups[static_cast<u32>(group)];
            auto& dst = _execOrder[static_cast<u32>(group)];

            dst.clear();
            dst.reserve(src.size());

            std::vector<TickFunction*> queue;
            std::vector<u32> indegree(src.size(), 0);

            for (u32 i = 0; i < src.size(); ++i) {
                for (const auto* dep : src[i]->dependencies) {
                    if (dep->group == group)
                        indegree[i]++;
                }
            }

            for (u32 i = 0; i < src.size(); ++i)
                if (indegree[i] == 0)
                    queue.push_back(src[i]);

            while (!queue.empty()) {
                TickFunction* fn = queue.back();
                queue.pop_back();
                dst.push_back(fn);

                for (u32 i = 0; i < src.size(); ++i) {
	                for (auto* other = src[i]; const auto* dep : other->dependencies) {
                        if (dep == fn && dep->group == group) {
                            if (--indegree[i] == 0)
                                queue.push_back(other);
                        }
                    }
                }
            }

            assert(dst.size() == src.size() && "Tick dependency cycle detected");
        }

    private:
        std::array<std::vector<TickFunction*>,
            static_cast<u32>(ETickGroup::COUNT)> _groups;

        std::array<std::vector<TickFunction*>,
            static_cast<u32>(ETickGroup::COUNT)> _execOrder;

        bool _dirty = false;

        float _fixedTimestep;
        float _accumulator;

        EExecutionMode _executionMode;
        std::mutex _mutex;
    };
}
