#pragma once

#include <array>
#include <vector>
#include <algorithm>
#include <cassert>
#include <mutex>
#include <iterator>

#include "../../EngineCore/subsystem.h"
#include "../../EngineCore/engine_context.h"
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

    class TickManager: public shine::Subsystem {
    public:
        static constexpr size_t GetStaticID() { return shine::HashString("TickManager"); }

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
        void BuildExecOrder() {
            // Topological sort based on prerequisites
            // For now, just copy group content
            for (u32 i = 0; i < static_cast<u32>(ETickGroup::Count); ++i) {
                _execOrder[i] = _groups[i];
            }
            _dirty = false;
        }

        void ExecutePhaseSingleThreaded(const std::vector<TickFunction*>& funcs, float dt) {
            for (auto* fn : funcs) {
                if (fn->IsEnabled()) {
                    fn->Execute(dt);
                }
            }
        }

        void ExecutePhaseMultiThreaded(const std::vector<TickFunction*>& funcs, float dt) {
            // Simple parallel for loop if task scheduler available
#if !defined(SHINE_PLATFORM_WASM) && !defined(__EMSCRIPTEN__)
            util::TaskScheduler::Get().ParallelFor(0, funcs.size(), [&](u32 i) {
                if (funcs[i]->IsEnabled()) {
                    funcs[i]->Execute(dt);
                }
            });
#else
            ExecutePhaseSingleThreaded(funcs, dt);
#endif
        }

    private:
        std::array<std::vector<TickFunction*>, static_cast<u32>(ETickGroup::Count)> _groups;
        std::array<std::vector<TickFunction*>, static_cast<u32>(ETickGroup::Count)> _execOrder;
        std::mutex _mutex;
        
        float _fixedTimestep;
        float _accumulator;
        EExecutionMode _executionMode;
        bool _dirty = false;
    };

    inline TickFunction::~TickFunction() {
        if (_registered) {
            if (g_EngineContext) {
                auto* manager = g_EngineContext->Get<TickManager>();
                if (manager) {
                    manager->Unregister(this);
                }
            }
        }
    }
}
