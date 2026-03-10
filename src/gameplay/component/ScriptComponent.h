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
        [[nodiscard]] const shine::SString& getScriptPath() const noexcept;  // 返回编译后的 .js 路径
        [[nodiscard]] const shine::SString& getSourcePath() const noexcept; // 返回源 .ts 路径

        [[nodiscard]] bool isLoaded() const noexcept;
        [[nodiscard]] shine::script::ScriptSystem::ScriptHandle getScriptHandle() const noexcept { return scriptHandle_; }
        void setTickEnabled(bool enabled);

        void onAttached() override;
        void onDetached() override;

    protected:
        void onTick(float deltaTime) override;

    private:
        void LoadScriptIfNeeded();
        void ReleaseScript();
        void SetScriptPaths(shine::STextView inputPath);  // 根据输入路径自动设置 sourcePath_ 和 scriptPath_

    private:
        shine::SString scriptPath_;   // 编译后的 .js 文件路径（用于执行）
        shine::SString sourcePath_;   // 源 .ts 文件路径（用于编辑器显示、调试）
        shine::script::ScriptSystem::ScriptHandle scriptHandle_{};
        bool loadAttempted_ = false;
    };
}
