#pragma once

#include <string>
#include <type_traits>
#include <utility>

#include "gameplay/tick/tick_function.h"
#include "memory/object_pool.h"
#include "util/guid.h"

namespace shine::render {
class CommandBuffer;
}

namespace shine::gameplay {
class SObject;
}

namespace shine::gameplay::component {

class UComponent
{

public:
    UComponent()          = default;
    virtual ~UComponent() = default;

    virtual void onBeginPlay() {}
    virtual void onRender(render::CommandBuffer &cmd) {}

    void                   attachTo(SObject *owner) noexcept { m_Owner = owner; }
    [[nodiscard]] SObject *getOwner() const noexcept { return m_Owner; }

protected:
    SObject *m_Owner{nullptr};

    std::string _ComponentName;
    util::FGuid _guid;
};

template <typename T, typename... TArgs>
requires std::is_base_of_v<UComponent, T>
T* NewComponent(TArgs&&... args) {
    return shine::co::PooledCreate<T>(std::forward<TArgs>(args)...);
}

template <typename T>
requires std::is_base_of_v<UComponent, T>
void DestroyComponent(T* comp) {
    shine::co::PooledDestroy(comp);
}
} // namespace shine::gameplay::component

namespace shine::co {
    template <typename T>
    struct ObjectPoolConfig<T, std::enable_if_t<std::is_base_of_v<shine::gameplay::component::UComponent, T>>> {
        static constexpr MemoryTag Tag = MemoryTag::Gameplay;
        static constexpr size_t BlockCount = 256;
    };
}
