#include "gameplay/component/ScriptComponent.h"

#include <algorithm>
#include <cctype>

#include "EngineCore/engine_context.h"

namespace shine::gameplay::component
{
    namespace
    {
        // 判断路径是否以指定扩展名结尾（不区分大小写）
        bool HasExtension(std::string_view path, std::string_view ext)
        {
            if (path.size() < ext.size()) return false;
            auto suffix = path.substr(path.size() - ext.size());
            return std::equal(suffix.begin(), suffix.end(), ext.begin(),
                [](char a, char b) { return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b)); });
        }

        // 从 TypeScript 源路径计算编译后的 JavaScript 路径
        // script/game.ts -> build/script/game.js
        shine::SString ComputeJsPathFromTs(std::string_view tsPath)
        {
            std::string pathStr(tsPath);
            // 替换 .ts 为 .js
            if (pathStr.size() >= 3)
            {
                pathStr.resize(pathStr.size() - 3);
                pathStr += ".js";
            }
            // 如果路径以 script/ 开头，替换为 build/script/
            const std::string scriptPrefix = "script/";
            const std::string buildPrefix = "build/script/";
            if (pathStr.find(scriptPrefix) == 0)
            {
                pathStr = buildPrefix + pathStr.substr(scriptPrefix.size());
            }
            return shine::SString(pathStr);
        }

        // 从 JavaScript 路径计算 TypeScript 源路径
        // build/script/game.js -> script/game.ts
        shine::SString ComputeTsPathFromJs(std::string_view jsPath)
        {
            std::string pathStr(jsPath);
            // 替换 .js 为 .ts
            if (pathStr.size() >= 3)
            {
                pathStr.resize(pathStr.size() - 3);
                pathStr += ".ts";
            }
            // 如果路径以 build/script/ 开头，替换为 script/
            const std::string buildPrefix = "build/script/";
            const std::string scriptPrefix = "script/";
            if (pathStr.find(buildPrefix) == 0)
            {
                pathStr = scriptPrefix + pathStr.substr(buildPrefix.size());
            }
            return shine::SString(pathStr);
        }
    }

    ScriptComponent::ScriptComponent(shine::STextView scriptPath)
    {
        SetScriptPaths(scriptPath);
    }

    ScriptComponent::~ScriptComponent()
    {
        ReleaseScript();
    }

    void ScriptComponent::setScriptPath(shine::STextView scriptPath)
    {
        SetScriptPaths(scriptPath);
        ReleaseScript();
        loadAttempted_ = false;
        LoadScriptIfNeeded();
    }

    void ScriptComponent::SetScriptPaths(shine::STextView inputPath)
    {
        if (HasExtension(inputPath, ".ts"))
        {
            // 输入是 TypeScript 源文件
            sourcePath_ = shine::SString(inputPath);
            scriptPath_ = ComputeJsPathFromTs(inputPath);
        }
        else if (HasExtension(inputPath, ".js"))
        {
            // 输入是 JavaScript 文件
            scriptPath_ = shine::SString(inputPath);
            sourcePath_ = ComputeTsPathFromJs(inputPath);
        }
        else
        {
            // 未知扩展名，当作 JS 处理
            scriptPath_ = shine::SString(inputPath);
            sourcePath_.clear();
        }
    }

    const shine::SString& ScriptComponent::getScriptPath() const noexcept
    {
        return scriptPath_;
    }

    const shine::SString& ScriptComponent::getSourcePath() const noexcept
    {
        return sourcePath_;
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
