#pragma once

#include "gameplay/component/tickableComponent.h"
#include "script/ScriptSystem.h"
#include "string/shine_string.h"
#include "string/shine_text_view.h"

namespace shine::gameplay::component
{
    class ScriptComponent : public TickableComponent
    {
    public:
        ScriptComponent() = default;
        explicit ScriptComponent(shine::STextView scriptPath);
        ~ScriptComponent() override;

        void setScriptPath(shine::STextView scriptPath);
        [[nodiscard]] const shine::SString& getScriptPath() const noexcept;

        [[nodiscard]] bool isLoaded() const noexcept;
        void setTickEnabled(bool enabled);

        void onAttached() override;
        void onDetached() override;

    protected:
        void onTick(float deltaTime) override;

    private:
        void LoadScriptIfNeeded();
        void ReleaseScript();

    private:
        shine::SString scriptPath_;
        shine::script::ScriptSystem::ScriptHandle scriptHandle_{};
        bool loadAttempted_ = false;
    };
}
