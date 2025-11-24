#pragma once


#include <string>


#include "util/guid.h"

namespace shine::render::command
{
	class ICommandList;
}

namespace shine::gameplay
{
	class SObject;
}

namespace shine::gameplay::component
{

    class UComponent
    {
    public:
        UComponent() = default;
        virtual ~UComponent() = default;

        virtual void onBeginPlay() {}
        virtual void onTick(float /*deltaTime*/) {}
        // 组件渲染，将渲染命令提交到 ICommandList
        virtual void onRender(shine::render::command::ICommandList& cmd) {}

        void attachTo(shine::gameplay::SObject* owner) { m_Owner = owner; }
        [[nodiscard]]  shine::gameplay::SObject* getOwner() const { return m_Owner; }

    protected:
        shine::gameplay::SObject* m_Owner { nullptr };

        // 组件名称
        std::string  _ComponentName;
        util::FGuid  _guid;
    };
}

