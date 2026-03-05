#include "gameplay/component/ScriptComponent.h"

#include "EngineCore/engine_context.h"

namespace shine::gameplay::component
{
    ScriptComponent::ScriptComponent(shine::STextView scriptPath)
        : scriptPath_(scriptPath)
    {
    }

    ScriptComponent::~ScriptComponent()
    {
        ReleaseScript();
    }

    void ScriptComponent::setScriptPath(shine::STextView scriptPath)
    {
        scriptPath_ = shine::SString(scriptPath);
        ReleaseScript();
        loadAttempted_ = false;
        LoadScriptIfNeeded();
    }

    const shine::SString& ScriptComponent::getScriptPath() const noexcept
    {
        return scriptPath_;
    }

    bool ScriptComponent::isLoaded() const noexcept
    {
        return scriptHandle_.IsValid();
    }

    void ScriptComponent::setTickEnabled(bool enabled)
    {
        TickableComponent::setTickEnabled(enabled);
        if (!scriptHandle_.IsValid() || !EngineContext::IsInitialized())
        {
            return;
        }

        if (auto* scriptSystem = EngineContext::Get().GetSystem<shine::script::ScriptSystem>())
        {
            scriptSystem->SetScriptRuntimeEnabled(scriptHandle_, enabled, getOwner());
        }
    }

    void ScriptComponent::onAttached()
    {
        TickableComponent::onAttached();
        LoadScriptIfNeeded();
    }

    void ScriptComponent::onDetached()
    {
        ReleaseScript();
        TickableComponent::onDetached();
    }

    void ScriptComponent::onTick(float deltaTime)
    {
        if (!scriptHandle_.IsValid())
        {
            LoadScriptIfNeeded();
            return;
        }
        if (!EngineContext::IsInitialized())
        {
            return;
        }

        auto* scriptSystem = EngineContext::Get().GetSystem<shine::script::ScriptSystem>();
        if (!scriptSystem)
        {
            return;
        }
        scriptSystem->InvokeUpdate(scriptHandle_, deltaTime, getOwner());
    }

    void ScriptComponent::LoadScriptIfNeeded()
    {
        if (loadAttempted_ || scriptPath_.empty())
        {
            return;
        }
        if (!EngineContext::IsInitialized())
        {
            return;
        }

        auto* scriptSystem = EngineContext::Get().GetSystem<shine::script::ScriptSystem>();
        if (!scriptSystem)
        {
            return;
        }

        loadAttempted_ = true;
        scriptSystem->LoadScript(scriptPath_.view(), scriptHandle_);
    }

    void ScriptComponent::ReleaseScript()
    {
        if (!scriptHandle_.IsValid())
        {
            return;
        }

        if (EngineContext::IsInitialized())
        {
            if (auto* scriptSystem = EngineContext::Get().GetSystem<shine::script::ScriptSystem>())
            {
                scriptSystem->UnloadScript(scriptHandle_);
            }
        }

        scriptHandle_ = {};
    }
}
