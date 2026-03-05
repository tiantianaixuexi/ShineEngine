#pragma once

#include <algorithm>
#include <concepts>
#include <functional>
#include <utility>
#include <vector>

#include "compiler_hints.h"

namespace shine::util {

template <typename... Args>
class EventHandle {
public:

    using CallBack = std::move_only_function<void(Args...)>;


    struct Handle{
        std::size_t id = 0;
        explicit constexpr operator bool() const noexcept {
            return id != 0;
        }
    };

    EventHandle() = default;
    ~EventHandle() = default;

    EventHandle(const EventHandle&) = delete;
    EventHandle& operator=(const EventHandle&) = delete;

        // 移动时更新槽内的反向指针（见 ScopedConnection）
    EventHandle(EventHandle&& other) noexcept 
        : slots_(std::move(other.slots_))
        , next_id_(other.next_id_)
    {
        other.next_id_ = 0;  // 标记为已移动
    }

    EventHandle& operator=(EventHandle&& other) noexcept {
        if (this != &other) {
            slots_ = std::move(other.slots_);
            next_id_ = other.next_id_;
            other.next_id_ = 0;
        }
        return *this;
    }

     // ============================================
    // 绑定 - 完美转发，无拷贝
    // ============================================
    
    template<typename F>
        requires std::invocable<F, Args...> && 
                 std::constructible_from<CallBack, F>
    [[nodiscard]] Handle bind(F&& f) {
        const size_t id = next_id_++;
        // emplace_back 避免临时对象
        slots_.emplace_back(id, std::forward<F>(f));
        return Handle{id};
    }
    
    // 绑定成员函数 - 零开销包装
    template<typename T, typename... BoundArgs>
        requires std::invocable<void(T::*)(Args...), T*, Args...>
    [[nodiscard]] Handle bind(void(T::*mem_fn)(Args...), T* obj) {
        return bind([mem_fn, obj](Args... args) {
            (obj->*mem_fn)(std::forward<Args>(args)...);
        });
    }
    
    // 绑定 const 成员函数
    template<typename T>
    [[nodiscard]] Handle bind(void(T::*mem_fn)(Args...) const, const T* obj) {
        return bind([mem_fn, obj](Args... args) {
            (obj->*mem_fn)(std::forward<Args>(args)...);
        });
    }
    
    // // C++23: 使用 function_ref 绑定临时回调（无所有权）
    // void bind_one_shot(std::function_ref<void(Args...)> f) {
    //     // function_ref 不拥有可调用对象，适合临时绑定
    //     // 注意：需要确保 f 的生命周期覆盖 emit 调用
    //     slots_.emplace_back(next_id_++, [f](Args... args) {
    //         f(std::forward<Args>(args)...);
    //     });
    // }

    // ============================================
    // 触发 - 内联友好，无分配
    // ============================================
    
    void emit(Args... args) {
        // 原地遍历，无拷贝（move_only_function 支持移动但不允许复制）
        // 使用索引遍历允许回调中修改 slots_
        for (size_t i = 0; i < slots_.size(); ) {
            auto& [id, slot] = slots_[i];
            if (slot) {
                slot(std::forward<Args>(args)...);
                ++i;
            } else {
                // 槽被标记为空（通过 ScopedConnection 断开），移除
                slots_.erase(slots_.begin() + i);
            }
        }
    }
    
    // 函数调用运算符 - 强制内联
    FORCEINLINE void operator()(Args... args) {
        emit(std::forward<Args>(args)...);
    }

    // ============================================
    // 移除 - O(n) 但缓存友好
    // ============================================
    
    void unbind(Handle h) {
        if (!h) return;
        
        auto it = std::find_if(slots_.begin(), slots_.end(),
            [id = h.id](const auto& p) { return p.first == id; });
        
        if (it != slots_.end()) {
            // 直接销毁（move_only_function 置空）
            it->second = nullptr;
            // 延迟擦除，下次 emit 时清理
        }
    }
    
    void clear() noexcept {
        slots_.clear();
        next_id_ = 1;
    }

    void compact() {
        slots_.erase(std::remove_if(slots_.begin(), slots_.end(), [](const auto& p) {
            return !p.second;
        }), slots_.end());
    }

    // ============================================
    // 查询
    // ============================================
    
    [[nodiscard]] bool empty() const noexcept { 
        return slots_.empty(); 
    }
    
    [[nodiscard]] size_t size() const noexcept { 
        return slots_.size(); 
    }

    [[nodiscard]] size_t active_size() const noexcept {
        size_t count = 0;
        for (const auto& slot : slots_) {
            if (slot.second) {
                ++count;
            }
        }
        return count;
    }

    [[nodiscard]] bool is_bound(Handle h) const noexcept {
        if (!h) {
            return false;
        }
        for (const auto& [id, cb] : slots_) {
            if (id == h.id && cb) {
                return true;
            }
        }
        return false;
    }

    // ============================================
    // C++23: ScopedConnection - RAII 自动断开
    // ============================================
    
    class ScopedConnection {
        EventHandle* sig_ = nullptr;
        size_t id_ = 0;
        
    public:
        ScopedConnection() = default;
        ScopedConnection(EventHandle& s, Handle h) : sig_(&s), id_(h.id) {}
        
        // 移动构造
        ScopedConnection(ScopedConnection&& other) noexcept
            : sig_(other.sig_), id_(other.id_) {
            other.sig_ = nullptr;
            other.id_ = 0;
        }
        
        ScopedConnection& operator=(ScopedConnection&& other) noexcept {
            if (this != &other) {
                disconnect();
                sig_ = other.sig_;
                id_ = other.id_;
                other.sig_ = nullptr;
                other.id_ = 0;
            }
            return *this;
        }
        
        ~ScopedConnection() { disconnect(); }
        
        void disconnect() {
            if (sig_ && id_) {
                sig_->unbind(Handle{id_});
                sig_ = nullptr;
                id_ = 0;
            }
        }
        
        [[nodiscard]] bool connected() const noexcept {
            return sig_ != nullptr;
        }
        
        // 禁止拷贝
        ScopedConnection(const ScopedConnection&) = delete;
        ScopedConnection& operator=(const ScopedConnection&) = delete;
    };

    // 便捷方法：返回 ScopedConnection
    template<typename F>
    [[nodiscard]] ScopedConnection bind_scoped(F&& f) {
        return ScopedConnection(*this, bind(std::forward<F>(f)));
    }

private:
    // 使用 vector 保证缓存局部性，优于 map/unordered_map
    std::vector<std::pair<size_t, CallBack>> slots_;
    size_t next_id_ = 1;
};

} // namespace shine::util
