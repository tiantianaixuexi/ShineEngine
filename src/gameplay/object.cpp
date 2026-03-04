﻿#include "object.h"


// 实现模块内容
namespace shine::gameplay
{

    SObject::SObject()
    {
        // 构造函数实现
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


