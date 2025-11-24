#include "object.h"


// 实现模块内容
namespace shine::gameplay
{

    SObject::SObject()
    {
        // 构造函数实现
    }

    SObject::~SObject()
    {
        // 析构函数实现
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

    void SObject::onTick(float deltaTime)
    {
        if (!m_isTickable) return;

        //for (auto& comp : m_Components)
        //{
        //    comp->onTick(deltaTime);
        //}
    }

};


