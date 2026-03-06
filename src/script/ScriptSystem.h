#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <chrono>

#include "EngineCore/log/LogSystem.h"
#include "EngineCore/reflection/Script/ScriptValue.h"
#include "EngineCore/subsystem.h"
#include "string/shine_string.h"
#include "string/shine_text_view.h"
#include "util/function/EventHandle.h"
#include "third/quickjs/quickjs.h"

namespace shine::gameplay
{
    class SObject;
}

namespace shine::util
{
    class EngineDirectoryService;
}

namespace shine::util::watcher
{
    struct FileChangeEvent;
    class FileWatchService;
}

namespace shine::script
{

    REGISTER_LOG_GROUP(ScriptSystemLog)

    class ScriptSystem : public shine::Subsystem
    {
    public:
        struct ScriptPropertyInspectorInfo
        {
            shine::SString name;
            shine::SString type;
            shine::SString access;
            shine::SString group;
            bool visible = true;
        };

        struct ScriptHandle
        {
            uint32_t id = 0;

            [[nodiscard]] bool IsValid() const noexcept
            {
                return id != 0;
            }
        };

        bool Init(EngineContext& ctx) override;
        void Shutdown(EngineContext& ctx) override;

        bool LoadScript(STextView scriptPath, ScriptHandle& outHandle);
        bool ReloadScript(ScriptHandle handle);
        bool UnloadScript(ScriptHandle handle);
        bool InvokeUpdate(ScriptHandle handle, float deltaSeconds, shine::gameplay::SObject* owner);
        bool SetScriptRuntimeEnabled(ScriptHandle handle, bool enabled, shine::gameplay::SObject* owner);
        bool GetScriptPropertyInfos(ScriptHandle handle, std::vector<ScriptPropertyInspectorInfo>& outProperties) const;
        [[nodiscard]] uint64_t GetScriptPropertyLayoutVersion(ScriptHandle handle) const;
        bool GetScriptPropertyValue(ScriptHandle handle, shine::STextView propertyName, reflection::ScriptValue& outValue) const;
        bool SetScriptPropertyValue(ScriptHandle handle, shine::STextView propertyName, const reflection::ScriptValue& value);

        [[nodiscard]] bool IsReady() const noexcept;

    private:
        struct ScriptEntry
        {
            struct FunctionMeta
            {
                shine::SString name;
            };

            struct PropertyMeta
            {
                shine::SString name;
                shine::SString type;
                shine::SString access;
                shine::SString group;
                bool visible = true;
            };

            shine::SString className;
            shine::SString path;
            JSValue initFunc = JS_UNDEFINED;
            JSValue startFunc = JS_UNDEFINED;
            JSValue updateFunc = JS_UNDEFINED;
            JSValue destroyFunc = JS_UNDEFINED;
            JSValue onEnableFunc = JS_UNDEFINED;
            JSValue onDisableFunc = JS_UNDEFINED;
            JSValue scriptObject = JS_UNDEFINED;
            bool startCalled = false;
            bool enableNotified = false;
            bool scriptEnabled = true;
            uint32_t nextTimerId = 1;
            struct TimerEntry
            {
                uint32_t id = 0;
                float remainingSeconds = 0.0f;
                float intervalSeconds = 0.0f;
                bool repeat = false;
                bool cancelled = false;
                JSValue callback = JS_UNDEFINED;
            };
            std::vector<TimerEntry> timers;
            std::vector<FunctionMeta> functions;
            std::vector<PropertyMeta> properties;
            uint64_t propertyLayoutVersion = 1;
            bool tickingTimers = false;
        };

        struct InvokeScope
        {
            ScriptSystem* system = nullptr;
            ScriptHandle handle{};
            shine::gameplay::SObject* owner = nullptr;
        };

        static JSValue JsLog(JSContext* ctx, JSValueConst thisVal, int argc, JSValueConst* argv);
        static JSValue JsSetTimeout(JSContext* ctx, JSValueConst thisVal, int argc, JSValueConst* argv);
        static JSValue JsSetInterval(JSContext* ctx, JSValueConst thisVal, int argc, JSValueConst* argv);
        static JSValue JsClearTimer(JSContext* ctx, JSValueConst thisVal, int argc, JSValueConst* argv);
        static JSValue JsSetScriptEnabled(JSContext* ctx, JSValueConst thisVal, int argc, JSValueConst* argv);
        static JSValue JsSetScriptTickInterval(JSContext* ctx, JSValueConst thisVal, int argc, JSValueConst* argv);
        static JSValue JsReflectGetField(JSContext* ctx, JSValueConst thisVal, int argc, JSValueConst* argv);
        static JSValue JsReflectSetField(JSContext* ctx, JSValueConst thisVal, int argc, JSValueConst* argv);
        static JSValue JsReflectCallMethod(JSContext* ctx, JSValueConst thisVal, int argc, JSValueConst* argv);
        [[nodiscard]] shine::gameplay::SObject* FindActorById(int id) const;
        bool InvokeNoArg(ScriptHandle handle, ScriptEntry& entry, JSValue func, STextView stage);
        void TickTimers(ScriptHandle handle, ScriptEntry& entry, float deltaSeconds);
        bool InstallBindings();
        void ParseScriptMetadata(ScriptEntry& entry, JSValueConst globalObj);
        bool EvaluateScriptText(STextView scriptPath, std::string_view scriptText, ScriptEntry& outEntry);
        void ReleaseScriptEntry(ScriptEntry& entry);
        void ReportException(STextView stage, STextView scriptPath) const;
        [[nodiscard]] std::filesystem::path ResolveScriptPath(STextView scriptPath) const;
        void OnFileChanged(const util::watcher::FileChangeEvent& event);
        bool CompileTypeScript();
        void ReloadAllLoadedScripts();

    private:
        JSRuntime* runtime_ = nullptr;
        JSContext* context_ = nullptr;
        uint32_t nextHandleId_ = 1;
        std::unordered_map<uint32_t, ScriptEntry> loadedScripts_;
        InvokeScope invokeScope_{};
        util::watcher::FileWatchService* fileWatchService_ = nullptr;
        util::EngineDirectoryService* engineDirectoryService_ = nullptr;
        util::EventHandle<const util::watcher::FileChangeEvent&>::Handle fileWatchHandle_;
        std::filesystem::path scriptSourceRoot_;
        std::chrono::steady_clock::time_point lastTsCompileAt_{};
        bool decoratorLibLoaded_ = false;
        std::chrono::steady_clock::time_point lastReloadRequestAt_{};
        std::wstring lastReloadPath_{};
        std::unordered_map<std::wstring, uint64_t> sourceHashes_;
    };
}
