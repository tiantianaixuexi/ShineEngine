#pragma once


#include <string>
#include <vector>
#include <memory>

#include "util/guid.h"
#include "gameplay/component/component.h"


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
        virtual void onTick(float deltaTime);
        // 不直接需要渲染逻辑，组件自行在 onRender 中提供

    public:
        // getter 方法
        [[nodiscard]] bool isActive() const noexcept { return m_isActive; }
        [[nodiscard]] bool isVisible() const noexcept { return m_isVisible; }
        [[nodiscard]] bool shouldRender() const noexcept { return m_isRender; }
        [[nodiscard]] bool canTick() const noexcept { return m_isTickable; }

        // setter 方法
        void setActive(bool active) noexcept { m_isActive = active; }
        void setVisible(bool visible) noexcept { m_isVisible = visible; }
        void setRender(bool render) noexcept { m_isRender = render; }
        void setTickable(bool tickable) noexcept { m_isTickable = tickable; }

   
        // 名字管理
        [[nodiscard]] const std::string& getName() const noexcept { return _name; }
        void setName(const std::string& name) { _name = name; }

    private:
        // 事件 Flag
        unsigned int m_Flags;

        // 用 m_ 前缀标记成员变量，避免与函数名冲突
        unsigned int m_isTickable:1 = true;
        unsigned int m_isVisible:1  = true;
        unsigned int m_isRender:1   = true;
        unsigned int m_isActive:1   = true;

        // 组件管理
    public:

        template <class TComponent, class... TArgs>
        TComponent* addComponent(TArgs&&... args)
        {
            auto comp = std::make_unique<TComponent>(std::forward<TArgs>(args)...);
            comp->attachTo(this);
            TComponent* raw = comp.get();
            m_Components.emplace_back(std::move(comp));
            return raw;
        }

        const std::vector<std::unique_ptr<component::UComponent>>& getComponents() const noexcept { return m_Components; }

    private:

        std::vector<std::unique_ptr<component::UComponent>> m_Components;

        std::string _name;
        util::FGuid _guid;

    };
}

