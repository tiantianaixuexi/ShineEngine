#include "object.h"

#include <atomic>

// 实现模块内容
namespace shine::gameplay
{
    namespace
    {
        std::atomic<uint32_t> g_ObjectIdCounter{1};
    }

    SObject::SObject()
    {
        objectId_ = g_ObjectIdCounter.fetch_add(1, std::memory_order_relaxed);
    }

    SObject::~SObject()
    {
        detachAllComponents();
        m_Components.clear();
    }

    void SObject::OnInit()
    {
        // OnInit实现
        _guid = util::FGuid::NewGuid();
    }

    void SObject::onBeginPlay()
    {
        // onBeginPlay实现
    }

    void SObject::detachAllComponents() noexcept
    {
        for (const auto& comp : m_Components)
        {
            if (!comp)
            {
                continue;
            }
            comp->detachFromOwner();
        }
    }


};


