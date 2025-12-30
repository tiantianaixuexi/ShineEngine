# Tick 系统使用指南

## 概述

ShineEngine 的 Tick 系统提供了灵活的游戏循环管理，支持：
- 固定时间步长执行（确保物理模拟稳定）
- 可变时间步长执行（用于渲染等）
- 多线程并行执行（提升性能）
- Tick 分组和依赖管理
- 组件自动注册

## 基础使用

### 1. 创建 Tick 组件

```cpp
#include "gameplay/tick/TickenableComponent.h"

class MyActor : public shine::gameplay::TickableComponent<MyActor> {
private:
    shine::gameplay::tick::TickFunction _tickFn;
    
public:
    void RegisterTicks() override {
        _tickFn.fn = [](void* userdata, float dt) {
            auto* self = static_cast<MyActor*>(userdata);
            self->Update(dt);
        };
        _tickFn.userdata = this;
        _tickFn.group = shine::gameplay::tick::ETickGroup::PostPhysics;
        
        RegisterTick(_tickFn);
    }
    
    void Update(float dt) {
        // 每帧更新逻辑
    }
};
```

### 2. 配置 Tick Manager

```cpp
auto& tickManager = shine::gameplay::tick::TickManager::get();

// 设置固定时间步长（用于物理）
tickManager.SetFixedTimestep(1.0f / 60.0f);

// 设置执行模式
tickManager.SetExecutionMode(shine::gameplay::tick::EExecutionMode::MultiThreaded);

// 在游戏循环中调用
void GameLoop() {
    float deltaTime = CalculateDeltaTime();
    tickManager.ExecuteAll(deltaTime);
}
```

## Tick 分组

系统提供了四个 Tick 阶段：

```cpp
enum class ETickGroup {
    PrePhysics,   // 物理前处理
    Physics,      // 物理模拟（固定时间步长）
    PostPhysics,  // 物理后处理
    Late          // 渲染前处理
};
```

### 使用示例

```cpp
class PhysicsActor : public TickableComponent<PhysicsActor> {
private:
    TickFunction _prePhysicsTick;
    TickFunction _physicsTick;
    TickFunction _postPhysicsTick;
    
public:
    void RegisterTicks() override {
        // 物理前：收集输入
        _prePhysicsTick.fn = [](void* userdata, float dt) {
            auto* self = static_cast<PhysicsActor*>(userdata);
            self->GatherInput();
        };
        _prePhysicsTick.userdata = this;
        _prePhysicsTick.group = ETickGroup::PrePhysics;
        RegisterTick(_prePhysicsTick);
        
        // 物理：更新位置
        _physicsTick.fn = [](void* userdata, float dt) {
            auto* self = static_cast<PhysicsActor*>(userdata);
            self->UpdatePhysics(dt);
        };
        _physicsTick.userdata = this;
        _physicsTick.group = ETickGroup::Physics;
        RegisterTick(_physicsTick);
        
        // 物理后：更新渲染数据
        _postPhysicsTick.fn = [](void* userdata, float dt) {
            auto* self = static_cast<PhysicsActor*>(userdata);
            self->UpdateRenderData();
        };
        _postPhysicsTick.userdata = this;
        _postPhysicsTick.group = ETickGroup::PostPhysics;
        RegisterTick(_postPhysicsTick);
    }
};
```

## Tick 依赖

可以设置 Tick 函数之间的依赖关系，确保执行顺序：

```cpp
class ComplexActor : public TickableComponent<ComplexActor> {
private:
    TickFunction _tickA;
    TickFunction _tickB;
    TickFunction _tickC;
    
public:
    void RegisterTicks() override {
        // 设置 Tick A
        _tickA.fn = [](void* userdata, float dt) {
            auto* self = static_cast<ComplexActor*>(userdata);
            self->ProcessA();
        };
        _tickA.userdata = this;
        _tickA.group = ETickGroup::PostPhysics;
        RegisterTick(_tickA);
        
        // 设置 Tick B，依赖于 A
        _tickB.fn = [](void* userdata, float dt) {
            auto* self = static_cast<ComplexActor*>(userdata);
            self->ProcessB();
        };
        _tickB.userdata = this;
        _tickB.group = ETickGroup::PostPhysics;
        _tickB.dependencies.push_back(&_tickA);
        RegisterTick(_tickB);
        
        // 设置 Tick C，依赖于 B
        _tickC.fn = [](void* userdata, float dt) {
            auto* self = static_cast<ComplexActor*>(userdata);
            self->ProcessC();
        };
        _tickC.userdata = this;
        _tickC.group = ETickGroup::PostPhysics;
        _tickC.dependencies.push_back(&_tickB);
        RegisterTick(_tickC);
    }
};
```

## Tick 间隔控制

可以设置 Tick 的执行间隔：

```cpp
class SlowUpdateActor : public TickableComponent<SlowUpdateActor> {
private:
    TickFunction _slowTick;
    
public:
    void RegisterTicks() override {
        // 每 0.5 秒执行一次
        _slowTick.fn = [](void* userdata, float dt) {
            auto* self = static_cast<SlowUpdateActor*>(userdata);
            self->SlowUpdate();
        };
        _slowTick.userdata = this;
        _slowTick.group = ETickGroup::PostPhysics;
        _slowTick.interval = 0.5f;  // 500ms
        RegisterTick(_slowTick);
    }
};
```

## Tick 启用/禁用

使用 `TickEnableState` 控制 Tick 是否执行：

```cpp
class ToggleableActor : public TickableComponent<ToggleableActor> {
private:
    TickFunction _tickFn;
    TickEnableState _tickEnable;
    
public:
    void RegisterTicks() override {
        _tickFn.fn = [](void* userdata, float dt) {
            auto* self = static_cast<ToggleableActor*>(userdata);
            self->Update();
        };
        _tickFn.userdata = this;
        _tickFn.group = ETickGroup::PostPhysics;
        _tickFn.enable = &_tickEnable;
        RegisterTick(_tickFn);
    }
    
    void EnableTick() {
        _tickEnable.enabled = true;
    }
    
    void DisableTick() {
        _tickEnable.enabled = false;
    }
};
```

## 多线程执行

### 启用多线程

```cpp
// 在非 WASM 平台启用多线程
#if !defined(SHINE_PLATFORM_WASM) && !defined(__EMSCRIPTEN__)
    tickManager.SetExecutionMode(EExecutionMode::MultiThreaded);
#endif
```

### 多线程注意事项

1. **线程安全**：确保 Tick 函数中的操作是线程安全的
2. **共享数据**：使用互斥锁保护共享数据
3. **依赖关系**：正确设置依赖关系，避免数据竞争

```cpp
class ThreadSafeActor : public TickableComponent<ThreadSafeActor> {
private:
    TickFunction _tickFn;
    std::mutex _dataMutex;
    std::vector<int> _sharedData;
    
public:
    void RegisterTicks() override {
        _tickFn.fn = [](void* userdata, float dt) {
            auto* self = static_cast<ThreadSafeActor*>(userdata);
            
            std::lock_guard<std::mutex> lock(self->_dataMutex);
            self->UpdateData(dt);
        };
        _tickFn.userdata = this;
        _tickFn.group = ETickGroup::PostPhysics;
        RegisterTick(_tickFn);
    }
    
    void UpdateData(float dt) {
        // 安全地更新共享数据
        for (auto& item : _sharedData) {
            item += static_cast<int>(dt * 100);
        }
    }
};
```

## 性能监控

使用性能监控工具分析 Tick 性能：

```cpp
#include "util/profiler/profiler.h"

class MonitoredActor : public TickableComponent<MonitoredActor> {
private:
    TickFunction _tickFn;
    shine::util::Profiler& _profiler = shine::util::Profiler::Get();
    u32 _tickProfilerId = 0;
    
public:
    void RegisterTicks() override {
        _tickProfilerId = _profiler.CreateEntry("MonitoredActor Tick");
        
        _tickFn.fn = [](void* userdata, float dt) {
            auto* self = static_cast<MonitoredActor*>(userdata);
            
            shine::util::ScopedProfiler scope(self->_profiler, self->_tickProfilerId);
            self->Update(dt);
        };
        _tickFn.userdata = this;
        _tickFn.group = ETickGroup::PostPhysics;
        RegisterTick(_tickFn);
    }
    
    void PrintPerformanceReport() {
        auto reports = _profiler.GetTopSlowest(10);
        for (const auto& report : reports) {
            printf("%s: %.3fms (avg: %.3fms, max: %.3fms)\n",
                   report.name.c_str(),
                   report.lastDuration,
                   report.avgDuration,
                   report.maxDuration);
        }
    }
};
```

## 最佳实践

1. **合理分组**：将 Tick 放入合适的分组，确保执行顺序
2. **设置依赖**：使用依赖关系管理 Tick 执行顺序
3. **控制频率**：使用 `interval` 控制不常更新的逻辑
4. **线程安全**：在多线程模式下注意数据竞争
5. **性能监控**：定期分析 Tick 性能，优化瓶颈
6. **资源管理**：使用 `TickableComponent` 自动管理 Tick 生命周期

## WASM 兼容性

在 WASM 平台上，多线程会自动降级为单线程：

```cpp
// 自动适配平台
tickManager.SetExecutionMode(EExecutionMode::MultiThreaded);
// 在 WASM 上会自动使用单线程执行
```

## 完整示例

```cpp
#include "gameplay/tick/TickenableComponent.h"
#include "util/profiler/profiler.h"

class Player : public shine::gameplay::TickableComponent<Player> {
private:
    shine::gameplay::tick::TickFunction _inputTick;
    shine::gameplay::tick::TickFunction _physicsTick;
    shine::gameplay::tick::TickFunction _animationTick;
    shine::gameplay::tick::TickEnableState _tickEnable;
    
    shine::util::Profiler& _profiler = shine::util::Profiler::Get();
    u32 _inputProfilerId = 0;
    u32 _physicsProfilerId = 0;
    u32 _animationProfilerId = 0;
    
    std::mutex _positionMutex;
    float _position[3] = {0, 0, 0};
    
public:
    void RegisterTicks() override {
        _inputProfilerId = _profiler.CreateEntry("Player Input");
        _physicsProfilerId = _profiler.CreateEntry("Player Physics");
        _animationProfilerId = _profiler.CreateEntry("Player Animation");
        
        // 输入处理
        _inputTick.fn = [](void* userdata, float dt) {
            auto* self = static_cast<Player*>(userdata);
            shine::util::ScopedProfiler scope(self->_profiler, self->_inputProfilerId);
            self->ProcessInput(dt);
        };
        _inputTick.userdata = this;
        _inputTick.group = shine::gameplay::tick::ETickGroup::PrePhysics;
        _inputTick.enable = &_tickEnable;
        RegisterTick(_inputTick);
        
        // 物理更新
        _physicsTick.fn = [](void* userdata, float dt) {
            auto* self = static_cast<Player*>(userdata);
            shine::util::ScopedProfiler scope(self->_profiler, self->_physicsProfilerId);
            self->UpdatePhysics(dt);
        };
        _physicsTick.userdata = this;
        _physicsTick.group = shine::gameplay::tick::ETickGroup::Physics;
        _physicsTick.dependencies.push_back(&_inputTick);
        _physicsTick.enable = &_tickEnable;
        RegisterTick(_physicsTick);
        
        // 动画更新
        _animationTick.fn = [](void* userdata, float dt) {
            auto* self = static_cast<Player*>(userdata);
            shine::util::ScopedProfiler scope(self->_profiler, self->_animationProfilerId);
            self->UpdateAnimation(dt);
        };
        _animationTick.userdata = this;
        _animationTick.group = shine::gameplay::tick::ETickGroup::PostPhysics;
        _animationTick.dependencies.push_back(&_physicsTick);
        _animationTick.enable = &_tickEnable;
        RegisterTick(_animationTick);
    }
    
    void ProcessInput(float dt) {
        // 处理输入
    }
    
    void UpdatePhysics(float dt) {
        std::lock_guard<std::mutex> lock(_positionMutex);
        // 更新物理位置
    }
    
    void UpdateAnimation(float dt) {
        std::lock_guard<std::mutex> lock(_positionMutex);
        // 更新动画
    }
    
    void Enable() {
        _tickEnable.enabled = true;
    }
    
    void Disable() {
        _tickEnable.enabled = false;
    }
};
```
