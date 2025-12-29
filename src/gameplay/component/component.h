#pragma once


#include <string>

#include "gameplay/tick/tick_function.h"
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
        virtual void onRender(render::command::ICommandList& cmd) {}

        void attachTo(SObject* owner) { m_Owner = owner; }
        [[nodiscard]]  SObject* getOwner() const { return m_Owner; }

    protected:
        SObject* m_Owner { nullptr };

        std::string  _ComponentName;
        util::FGuid  _guid;
    };
}

