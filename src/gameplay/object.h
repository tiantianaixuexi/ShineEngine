#pragma once


#include <string>
#include <vector>
#include <memory>
#include <type_traits>
#include <utility>
#include <cstdint>

#include "objectFlag.h"
#include "util/guid.h"
#include "gameplay/component/component.h"
#include "memory/object_pool.h"





namespace shine::gameplay
{


    namespace component
    {
        class UComponent;
    }


    class SObject
    {
    public:
        SObject();
        virtual ~SObject();

        virtual void OnInit();
        virtual void onBeginPlay();
        virtual const char* getClassName() const noexcept { return "SObject"; }
        // 不直接需要渲染逻辑，组件自行在 onRender 中提供

    public:
        // getter 方法
        [[nodiscard]] bool isActive() const noexcept { return m_isActive; }
        [[nodiscard]] bool isVisible() const noexcept { return m_isVisible; }
        [[nodiscard]] bool shouldRender() const noexcept { return m_isRender; }

        // setter 方法
        void setActive(bool active) noexcept { m_isActive = active; }
        void setVisible(bool visible) noexcept { m_isVisible = visible; }
        void setRender(bool render) noexcept { m_isRender = render; }

        void setFlag(EObjectFlags flag, bool value) noexcept
        {
            if (value)
            {
                m_Flags |= flag;
            }
            else
            {
                m_Flags &= ~flag;
            }
        }

   
        // 名字管理
        [[nodiscard]] const std::string& getName() const noexcept { return _name; }
        void setName(const std::string& name) { _name = name; }
        [[nodiscard]] uint32_t getObjectId() const noexcept { return objectId_; }

    private:
        // 事件 Flag
        EObjectFlags m_Flags;

        // 用 m_ 前缀标记成员变量，避免与函数名冲突
        u32  m_isVisible:1  = true;
        u32  m_isRender:1   = true;
        u32  m_isActive:1   = true;

        // 组件管理
    public:

        using ComponentDeleter = void(*)(component::UComponent*);
        using ComponentPtr = std::unique_ptr<component::UComponent, ComponentDeleter>;

        template <class TComponent>
        static void destroyComponent(component::UComponent* comp) {
            component::DestroyComponent(static_cast<TComponent*>(comp));
        }

        template <class TComponent, class... TArgs>
        TComponent* addComponent(TArgs&&... args)
        {
            auto* raw = component::NewComponent<TComponent>(std::forward<TArgs>(args)...);
            raw->attachTo(this);
            ComponentPtr comp(raw, &destroyComponent<TComponent>);
            m_Components.emplace_back(std::move(comp));
            return raw;
        }

        const std::vector<ComponentPtr>& getComponents() const noexcept { return m_Components; }

        template <typename T>
        T* getComponent() const
        {
            for (const auto& comp : m_Components)
            {
                if (auto ptr = dynamic_cast<T*>(comp.get()))
                {
                    return ptr;
                }
            }
            return nullptr;
        }

    private:
        void detachAllComponents() noexcept;

        std::vector<ComponentPtr> m_Components;

        std::string _name;
        util::FGuid _guid;
        uint32_t objectId_ = 0;

    };

    template <typename T, typename... TArgs>
    requires std::is_base_of_v<SObject, T>
    T* NewObject(TArgs&&... args) {
        return shine::co::PooledCreate<T>(std::forward<TArgs>(args)...);
    }

    template <typename T>
    requires std::is_base_of_v<SObject, T>
    void DestroyObject(T* obj) {
        shine::co::PooledDestroy(obj);
    }
}

namespace shine::co {
    template <typename T>
    struct ObjectPoolConfig<T, std::enable_if_t<std::is_base_of_v<shine::gameplay::SObject, T>>> {
        static constexpr MemoryTag Tag = MemoryTag::Gameplay;
        static constexpr size_t BlockCount = 256;
    };
}
