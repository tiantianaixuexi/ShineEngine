#include "script/ScriptSystem.h"

#include <array>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cwctype>
#include <string>

#include "EngineCore/engine_context.h"
#include "EngineCore/reflection/Reflection.h"
#include "gameplay/actor.h"
#include "gameplay/component/ScriptComponent.h"
#include "gameplay/component/StaticMeshComponent.h"
#include "gameplay/component/TransformComponent.h"
#include "gameplay/mesh/StaticMesh.h"
#include "gameplay/object.h"
#include "gameplay/world/world_service.h"
#include "util/EngineDirectoryService.h"
#include "util/encoding_util.ixx"
#include "util/file_util.ixx"
#include "util/watcher/FileWatchService.h"

namespace shine::script
{
    namespace
    {
        class QuickJSScriptBridge final : public reflection::ScriptBridge
        {
        public:
            explicit QuickJSScriptBridge(JSContext* context)
                : context_(context)
            {
            }

            reflection::ScriptValue ToScript(const void* nativePtr, reflection::TypeId typeId) const override
            {
                if (!nativePtr)
                {
                    return {};
                }
                if (typeId == reflection::GetTypeId<bool>())
                {
                    return reflection::ScriptValue{*static_cast<const bool*>(nativePtr)};
                }
                if (typeId == reflection::GetTypeId<int>() ||
                    typeId == reflection::GetTypeId<int32_t>())
                {
                    return reflection::ScriptValue{static_cast<int>(*static_cast<const int32_t*>(nativePtr))};
                }
                if (typeId == reflection::GetTypeId<uint32_t>())
                {
                    return reflection::ScriptValue{static_cast<int>(*static_cast<const uint32_t*>(nativePtr))};
                }
                if (typeId == reflection::GetTypeId<float>())
                {
                    return reflection::ScriptValue{*static_cast<const float*>(nativePtr)};
                }
                if (typeId == reflection::GetTypeId<double>())
                {
                    return reflection::ScriptValue{*static_cast<const double*>(nativePtr)};
                }
                if (typeId == reflection::GetTypeId<std::string>())
                {
                    return reflection::ScriptValue{*static_cast<const std::string*>(nativePtr)};
                }
                if (typeId == reflection::GetTypeId<shine::SString>())
                {
                    return reflection::ScriptValue{static_cast<const shine::SString*>(nativePtr)->to_string()};
                }
                return {};
            }

            void FromScript(const reflection::ScriptValue& scriptVal, void* outNativePtr, reflection::TypeId targetType) const override
            {
                if (!outNativePtr)
                {
                    return;
                }

                const auto& value = scriptVal.data;
                if (targetType == reflection::GetTypeId<bool>())
                {
                    *static_cast<bool*>(outNativePtr) = std::holds_alternative<bool>(value) ? std::get<bool>(value) : false;
                    return;
                }
                if (targetType == reflection::GetTypeId<int>() || targetType == reflection::GetTypeId<int32_t>())
                {
                    if (std::holds_alternative<int>(value))
                    {
                        *static_cast<int32_t*>(outNativePtr) = static_cast<int32_t>(std::get<int>(value));
                    }
                    else if (std::holds_alternative<double>(value))
                    {
                        *static_cast<int32_t*>(outNativePtr) = static_cast<int32_t>(std::get<double>(value));
                    }
                    else
                    {
                        *static_cast<int32_t*>(outNativePtr) = 0;
                    }
                    return;
                }
                if (targetType == reflection::GetTypeId<uint32_t>())
                {
                    if (std::holds_alternative<int>(value))
                    {
                        *static_cast<uint32_t*>(outNativePtr) = static_cast<uint32_t>(std::max<int>(0, std::get<int>(value)));
                    }
                    else if (std::holds_alternative<double>(value))
                    {
                        *static_cast<uint32_t*>(outNativePtr) = static_cast<uint32_t>(std::max<double>(0.0, std::get<double>(value)));
                    }
                    else
                    {
                        *static_cast<uint32_t*>(outNativePtr) = 0;
                    }
                    return;
                }
                if (targetType == reflection::GetTypeId<float>())
                {
                    if (std::holds_alternative<int>(value))
                    {
                        *static_cast<float*>(outNativePtr) = static_cast<float>(std::get<int>(value));
                    }
                    else if (std::holds_alternative<float>(value))
                    {
                        *static_cast<float*>(outNativePtr) = std::get<float>(value);
                    }
                    else if (std::holds_alternative<double>(value))
                    {
                        *static_cast<float*>(outNativePtr) = static_cast<float>(std::get<double>(value));
                    }
                    else
                    {
                        *static_cast<float*>(outNativePtr) = 0.0f;
                    }
                    return;
                }
                if (targetType == reflection::GetTypeId<double>())
                {
                    if (std::holds_alternative<int>(value))
                    {
                        *static_cast<double*>(outNativePtr) = static_cast<double>(std::get<int>(value));
                    }
                    else if (std::holds_alternative<float>(value))
                    {
                        *static_cast<double*>(outNativePtr) = static_cast<double>(std::get<float>(value));
                    }
                    else if (std::holds_alternative<double>(value))
                    {
                        *static_cast<double*>(outNativePtr) = std::get<double>(value);
                    }
                    else
                    {
                        *static_cast<double*>(outNativePtr) = 0.0;
                    }
                    return;
                }
                if (targetType == reflection::GetTypeId<std::string>())
                {
                    if (std::holds_alternative<std::string>(value))
                    {
                        *static_cast<std::string*>(outNativePtr) = std::get<std::string>(value);
                    }
                    else
                    {
                        static_cast<std::string*>(outNativePtr)->clear();
                    }
                    return;
                }
                if (targetType == reflection::GetTypeId<shine::SString>())
                {
                    if (std::holds_alternative<std::string>(value))
                    {
                        *static_cast<shine::SString*>(outNativePtr) = shine::SString(std::get<std::string>(value));
                    }
                    else
                    {
                        *static_cast<shine::SString*>(outNativePtr) = shine::SString();
                    }
                }
            }

            reflection::ScriptValue FromJSValue(JSValueConst value) const
            {
                if (JS_IsBool(value))
                {
                    return reflection::ScriptValue{JS_ToBool(context_, value) != 0};
                }
                if (JS_IsNumber(value))
                {
                    double number = 0.0;
                    JS_ToFloat64(context_, &number, value);
                    const double rounded = std::round(number);
                    if (std::abs(number - rounded) < 1e-9)
                    {
                        return reflection::ScriptValue{static_cast<int>(rounded)};
                    }
                    return reflection::ScriptValue{static_cast<float>(number)};
                }
                if (JS_IsString(value))
                {
                    const char* str = JS_ToCString(context_, value);
                    if (!str)
                    {
                        return {};
                    }
                    std::string text(str);
                    JS_FreeCString(context_, str);
                    return reflection::ScriptValue{std::move(text)};
                }
                return {};
            }

            JSValue ToJSValue(const reflection::ScriptValue& value) const
            {
                if (std::holds_alternative<std::monostate>(value.data))
                {
                    return JS_UNDEFINED;
                }
                if (std::holds_alternative<bool>(value.data))
                {
                    return JS_NewBool(context_, std::get<bool>(value.data));
                }
                if (std::holds_alternative<int>(value.data))
                {
                    return JS_NewInt32(context_, std::get<int>(value.data));
                }
                if (std::holds_alternative<float>(value.data))
                {
                    return JS_NewFloat64(context_, static_cast<double>(std::get<float>(value.data)));
                }
                if (std::holds_alternative<double>(value.data))
                {
                    return JS_NewFloat64(context_, std::get<double>(value.data));
                }
                if (std::holds_alternative<std::string>(value.data))
                {
                    return JS_NewString(context_, std::get<std::string>(value.data).c_str());
                }
                return JS_UNDEFINED;
            }

        private:
            JSContext* context_ = nullptr;
        };

    }

    REGISTER_LOG_GROUP_END(ScriptSystemLog)

    bool ScriptSystem::Init(EngineContext& ctx)
    {
        (void)ctx;
        ADD_LOG_CATEGORY_WITH_CONSOLE(ScriptSystemLog, "ScriptInit", true);
        ADD_LOG_CATEGORY_WITH_CONSOLE(ScriptSystemLog, "ScriptRuntime", true);
        ADD_LOG_CATEGORY_WITH_CONSOLE(ScriptSystemLog, "ScriptError", true);

        fileWatchService_ = ctx.GetSystem<util::watcher::FileWatchService>();
        engineDirectoryService_ = ctx.GetSystem<util::EngineDirectoryService>();
        if (engineDirectoryService_)
        {
            scriptSourceRoot_ = engineDirectoryService_->GetProjectRootDirectory() / "script";
        }

        runtime_ = JS_NewRuntime();
        if (!runtime_)
        {
            SHINE_LOG_ERROR(ScriptSystemLog, "ScriptInit", "创建 QuickJS Runtime 失败");
            return false;
        }

        context_ = JS_NewContext(runtime_);
        if (!context_)
        {
            SHINE_LOG_ERROR(ScriptSystemLog, "ScriptInit", "创建 QuickJS Context 失败");
            JS_FreeRuntime(runtime_);
            runtime_ = nullptr;
            return false;
        }
        if (!InstallBindings())
        {
            JS_FreeContext(context_);
            context_ = nullptr;
            JS_FreeRuntime(runtime_);
            runtime_ = nullptr;
            return false;
        }

        if (fileWatchService_ && !scriptSourceRoot_.empty())
        {
            fileWatchService_->AddWatchDirectory(scriptSourceRoot_);
            fileWatchHandle_ = fileWatchService_->OnFileChanged.bind([this](const util::watcher::FileChangeEvent& event) {
                OnFileChanged(event);
            });
        }

        SHINE_LOG_INFO(ScriptSystemLog, "ScriptInit", "ScriptSystem 初始化完成");
        return true;
    }

    void ScriptSystem::Shutdown(EngineContext& ctx)
    {
        (void)ctx;

        for (auto& [handle, entry] : loadedScripts_)
        {
            (void)handle;
            ReleaseScriptEntry(entry);
        }
        loadedScripts_.clear();

        if (context_)
        {
            auto drainJobs = [this]()
            {
                int executed = 0;
                JSContext* pendingContext = nullptr;
                while (JS_IsJobPending(runtime_))
                {
                    const int executeRet = JS_ExecutePendingJob(runtime_, &pendingContext);
                    if (executeRet <= 0)
                    {
                        break;
                    }
                    ++executed;
                }
                return executed;
            };

            for (int i = 0; i < 4; ++i)
            {
                const int executed = drainJobs();
                JS_RunGC(runtime_);
                if (executed == 0)
                {
                    break;
                }
            }
            JS_FreeContext(context_);
            context_ = nullptr;
        }
        if (runtime_)
        {
            for (int i = 0; i < 8; ++i)
            {
                JSContext* pendingContext = nullptr;
                int executed = 0;
                while (JS_IsJobPending(runtime_))
                {
                    const int executeRet = JS_ExecutePendingJob(runtime_, &pendingContext);
                    if (executeRet <= 0)
                    {
                        break;
                    }
                    ++executed;
                }
                JS_RunGC(runtime_);
                if (executed == 0)
                {
                    break;
                }
            }
#if defined(_DEBUG)
            SHINE_LOG_WARN(ScriptSystemLog, "ScriptRuntime", "Debug模式跳过 JS_FreeRuntime 以规避 QuickJS gc_obj_list 断言");
#else
            JS_FreeRuntime(runtime_);
#endif
            runtime_ = nullptr;
        }
        invokeScope_ = {};
        nextHandleId_ = 1;
        if (fileWatchService_ && fileWatchHandle_)
        {
            fileWatchService_->OnFileChanged.unbind(fileWatchHandle_);
            fileWatchHandle_ = {};
        }
        fileWatchService_ = nullptr;
        engineDirectoryService_ = nullptr;
        scriptSourceRoot_.clear();
        lastTsCompileAt_ = {};
    }

    bool ScriptSystem::LoadScript(STextView scriptPath, ScriptHandle& outHandle)
    {
        outHandle = {};
        if (!IsReady())
        {
            return false;
        }

        const std::filesystem::path resolvedPath = ResolveScriptPath(scriptPath);
        const auto textResult = util::read_file_text(shine::SString(resolvedPath.string()).view());
        if (!textResult.has_value())
        {
            SHINE_LOG_ERROR(ScriptSystemLog, "ScriptError", "读取脚本失败: {} error={}", resolvedPath.string(), textResult.error());
            return false;
        }

        ScriptEntry entry;
        if (!EvaluateScriptText(shine::SString(resolvedPath.string()).view(), textResult.value(), entry))
        {
            return false;
        }

        entry.path = shine::SString(resolvedPath.string());

        const uint32_t handleId = nextHandleId_++;
        auto [insertIt, inserted] = loadedScripts_.emplace(handleId, std::move(entry));
        if (!inserted)
        {
            return false;
        }
        ScriptHandle handle{ handleId };
        invokeScope_.owner = nullptr;
        if (!InvokeNoArg(handle, insertIt->second, insertIt->second.initFunc, STextView::from_literal("Init")))
        {
            ReleaseScriptEntry(insertIt->second);
            loadedScripts_.erase(insertIt);
            return false;
        }
        outHandle.id = handleId;

        SHINE_LOG_INFO(ScriptSystemLog, "ScriptRuntime", "加载脚本成功: {} handle={}", resolvedPath.string(), handleId);
        return true;
    }

    bool ScriptSystem::ReloadScript(ScriptHandle handle)
    {
        if (!handle.IsValid())
        {
            return false;
        }
        const auto it = loadedScripts_.find(handle.id);
        if (it == loadedScripts_.end())
        {
            return false;
        }

        const std::filesystem::path resolvedPath = ResolveScriptPath(it->second.path.view());
        const auto textResult = util::read_file_text(shine::SString(resolvedPath.string()).view());
        if (!textResult.has_value())
        {
            SHINE_LOG_ERROR(ScriptSystemLog, "ScriptError", "重载脚本读取失败: {} error={}", resolvedPath.string(), textResult.error());
            return false;
        }

        ScriptEntry newEntry;
        if (!EvaluateScriptText(shine::SString(resolvedPath.string()).view(), textResult.value(), newEntry))
        {
            return false;
        }

        ScriptHandle reloadedHandle{ handle.id };
        invokeScope_.owner = nullptr;
        if (!InvokeNoArg(reloadedHandle, it->second, it->second.destroyFunc, STextView::from_literal("Destroy")))
        {
            return false;
        }
        ReleaseScriptEntry(it->second);
        it->second = std::move(newEntry);
        it->second.path = shine::SString(resolvedPath.string());
        invokeScope_.owner = nullptr;
        if (!InvokeNoArg(reloadedHandle, it->second, it->second.initFunc, STextView::from_literal("Init")))
        {
            return false;
        }
        return true;
    }

    bool ScriptSystem::UnloadScript(ScriptHandle handle)
    {
        if (!handle.IsValid())
        {
            return false;
        }
        const auto it = loadedScripts_.find(handle.id);
        if (it == loadedScripts_.end())
        {
            return false;
        }

        ScriptHandle unloadHandle{ handle.id };
        invokeScope_.owner = nullptr;
        if (!InvokeNoArg(unloadHandle, it->second, it->second.destroyFunc, STextView::from_literal("Destroy")))
        {
            return false;
        }
        ReleaseScriptEntry(it->second);
        loadedScripts_.erase(it);
        return true;
    }

    bool ScriptSystem::InvokeUpdate(ScriptHandle handle, float deltaSeconds, shine::gameplay::SObject* owner)
    {
        if (!IsReady() || !handle.IsValid())
        {
            return false;
        }
        const auto it = loadedScripts_.find(handle.id);
        if (it == loadedScripts_.end())
        {
            return false;
        }
        invokeScope_.system = this;
        invokeScope_.handle = handle;
        invokeScope_.owner = owner;

        if (!it->second.startCalled)
        {
            if (!InvokeNoArg(handle, it->second, it->second.startFunc, STextView::from_literal("Start")))
            {
                invokeScope_.owner = nullptr;
                return false;
            }
            it->second.startCalled = true;
        }

        auto* scriptComp = owner ? owner->getComponent<shine::gameplay::component::ScriptComponent>() : nullptr;
        const bool componentEnabled = scriptComp ? scriptComp->isTickEnabled() : true;
        it->second.scriptEnabled = componentEnabled;
        if (!SetScriptRuntimeEnabled(handle, componentEnabled, owner))
        {
            invokeScope_.owner = nullptr;
            return false;
        }
        if (!it->second.scriptEnabled)
        {
            TickTimers(handle, it->second, deltaSeconds);
            invokeScope_.owner = nullptr;
            return true;
        }

        if (JS_IsFunction(context_, it->second.updateFunc))
        {
            JSValue arg = JS_NewFloat64(context_, static_cast<double>(deltaSeconds));
            JSValue ret = JS_Call(context_, it->second.updateFunc, JS_UNDEFINED, 1, &arg);
            JS_FreeValue(context_, arg);
            if (JS_IsException(ret))
            {
                ReportException(STextView::from_literal("Update"), it->second.path.view());
                JS_FreeValue(context_, ret);
                invokeScope_.owner = nullptr;
                return false;
            }
            JS_FreeValue(context_, ret);
        }

        TickTimers(handle, it->second, deltaSeconds);
        invokeScope_.owner = nullptr;
        return true;
    }

    bool ScriptSystem::IsReady() const noexcept
    {
        return runtime_ != nullptr && context_ != nullptr;
    }

    void ScriptSystem::OnFileChanged(const util::watcher::FileChangeEvent& event)
    {
        if (event.isDirectory || scriptSourceRoot_.empty())
        {
            return;
        }
        if (event.action != FILE_ACTION_ADDED &&
            event.action != FILE_ACTION_MODIFIED &&
            event.action != FILE_ACTION_RENAMED_NEW_NAME)
        {
            return;
        }

        const auto changedPath = std::filesystem::path(event.directory) / event.filename;
        std::error_code pathEc;
        const auto normalizedChanged = std::filesystem::weakly_canonical(changedPath, pathEc);
        const auto normalizedRoot = std::filesystem::weakly_canonical(scriptSourceRoot_, pathEc);
        if (!normalizedRoot.empty())
        {
            auto changedText = normalizedChanged.wstring();
            auto rootText = normalizedRoot.wstring();
            std::transform(changedText.begin(), changedText.end(), changedText.begin(), ::towlower);
            std::transform(rootText.begin(), rootText.end(), rootText.begin(), ::towlower);
            if (changedText.rfind(rootText, 0) != 0)
            {
                return;
            }
        }
        const auto extension = changedPath.extension().wstring();
        const auto utf8Path = util::EncodingUtil::WstringToUTF8(changedPath.wstring());
        SHINE_LOG_INFO(ScriptSystemLog, "ScriptRuntime", "检测到脚本文件变更: {}", utf8Path);

        std::wstring extensionLower = extension;
        std::transform(extensionLower.begin(), extensionLower.end(), extensionLower.begin(), ::towlower);
        if (extensionLower == L".ts")
        {
            if (CompileTypeScript())
            {
                ReloadAllLoadedScripts();
            }
            return;
        }
        if (extensionLower == L".js")
        {
            ReloadAllLoadedScripts();
        }
    }

    bool ScriptSystem::CompileTypeScript()
    {
        if (scriptSourceRoot_.empty())
        {
            return false;
        }

        const auto now = std::chrono::steady_clock::now();
        if (lastTsCompileAt_.time_since_epoch().count() != 0)
        {
            const auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTsCompileAt_);
            if (delta.count() < 300)
            {
                return false;
            }
        }
        lastTsCompileAt_ = now;

        std::filesystem::path logDir;
        if (engineDirectoryService_)
        {
            logDir = engineDirectoryService_->GetDirectory("Logs");
        }
        if (logDir.empty())
        {
            logDir = scriptSourceRoot_;
        }
        std::error_code ec;
        std::filesystem::create_directories(logDir, ec);
        const std::filesystem::path compileLogPath = logDir / "script_tsc_hot_reload.log";

        const std::string command =
            "cmd /d /c \"cd /d \"\"" + scriptSourceRoot_.string() +
            "\"\" && npm run build > \"\"" + compileLogPath.string() + "\"\" 2>&1\"";
        SHINE_LOG_INFO(ScriptSystemLog, "ScriptRuntime", "开始编译TS: {}", scriptSourceRoot_.string());
        const int exitCode = std::system(command.c_str());

        const auto outputResult = util::read_file_text(shine::SString(compileLogPath.string()).view());
        if (exitCode != 0)
        {
            SHINE_LOG_ERROR(ScriptSystemLog, "ScriptError", "TS编译失败，exitCode={}", exitCode);
            if (outputResult.has_value())
            {
                std::string output = outputResult.value();
                size_t start = 0;
                while (start < output.size())
                {
                    const size_t end = output.find('\n', start);
                    std::string line = (end == std::string::npos) ? output.substr(start) : output.substr(start, end - start);
                    if (!line.empty() && line.back() == '\r')
                    {
                        line.pop_back();
                    }
                    if (!line.empty())
                    {
                        SHINE_LOG_ERROR(ScriptSystemLog, "ScriptError", "[tsc] {}", line);
                    }
                    if (end == std::string::npos)
                    {
                        break;
                    }
                    start = end + 1;
                }
            }
            return false;
        }

        SHINE_LOG_INFO(ScriptSystemLog, "ScriptRuntime", "TS编译成功");
        return true;
    }

    void ScriptSystem::ReloadAllLoadedScripts()
    {
        if (loadedScripts_.empty())
        {
            return;
        }
        std::vector<uint32_t> handles;
        handles.reserve(loadedScripts_.size());
        for (const auto& [handleId, entry] : loadedScripts_)
        {
            (void)entry;
            handles.push_back(handleId);
        }
        for (const uint32_t handleId : handles)
        {
            if (ReloadScript(ScriptHandle{handleId}))
            {
                SHINE_LOG_INFO(ScriptSystemLog, "ScriptRuntime", "脚本热重载成功: handle={}", handleId);
            }
            else
            {
                SHINE_LOG_ERROR(ScriptSystemLog, "ScriptError", "脚本热重载失败: handle={}", handleId);
            }
        }
    }

    bool ScriptSystem::SetScriptRuntimeEnabled(ScriptHandle handle, bool enabled, shine::gameplay::SObject* owner)
    {
        if (!handle.IsValid())
        {
            return false;
        }
        const auto it = loadedScripts_.find(handle.id);
        if (it == loadedScripts_.end())
        {
            return false;
        }

        auto& entry = it->second;
        invokeScope_.system = this;
        invokeScope_.handle = handle;
        invokeScope_.owner = owner;

        if (enabled)
        {
            entry.scriptEnabled = true;
            if (entry.enableNotified)
            {
                return true;
            }
            if (!InvokeNoArg(handle, entry, entry.onEnableFunc, STextView::from_literal("OnEnable")))
            {
                return false;
            }
            entry.enableNotified = true;
            return true;
        }

        entry.scriptEnabled = false;
        if (!entry.enableNotified)
        {
            return true;
        }
        if (entry.enableNotified)
        {
            if (!InvokeNoArg(handle, entry, entry.onDisableFunc, STextView::from_literal("OnDisable")))
            {
                return false;
            }
            entry.enableNotified = false;
        }
        return true;
    }

    shine::gameplay::SObject* ScriptSystem::FindActorById(int id) const
    {
        if (!shine::EngineContext::IsInitialized())
        {
            return nullptr;
        }
        auto* worldService = shine::EngineContext::Get().GetSystem<shine::gameplay::world::WorldService>();
        if (!worldService)
        {
            return nullptr;
        }
        return worldService->findActorById(static_cast<uint32_t>(std::max(id, 0)));
    }

    bool ScriptSystem::InvokeNoArg(ScriptHandle handle, ScriptEntry& entry, JSValue func, STextView stage)
    {
        if (!JS_IsFunction(context_, func))
        {
            return true;
        }

        invokeScope_.system = this;
        invokeScope_.handle = handle;
        JSValue ret = JS_Call(context_, func, JS_UNDEFINED, 0, nullptr);
        if (JS_IsException(ret))
        {
            ReportException(stage, entry.path.view());
            JS_FreeValue(context_, ret);
            return false;
        }
        JS_FreeValue(context_, ret);
        return true;
    }

    void ScriptSystem::TickTimers(ScriptHandle handle, ScriptEntry& entry, float deltaSeconds)
    {
        entry.tickingTimers = true;
        for (size_t i = 0; i < entry.timers.size();)
        {
            auto& timer = entry.timers[i];
            if (timer.cancelled)
            {
                JS_FreeValue(context_, timer.callback);
                entry.timers.erase(entry.timers.begin() + static_cast<long long>(i));
                continue;
            }
            timer.remainingSeconds -= deltaSeconds;
            if (timer.remainingSeconds > 0.0f)
            {
                ++i;
                continue;
            }

            JSValue ret = JS_Call(context_, timer.callback, JS_UNDEFINED, 0, nullptr);
            if (JS_IsException(ret))
            {
                ReportException(STextView::from_literal("Timer"), entry.path.view());
                JS_FreeValue(context_, ret);
            }
            else
            {
                JS_FreeValue(context_, ret);
            }

            if (timer.repeat && !timer.cancelled)
            {
                timer.remainingSeconds += timer.intervalSeconds;
                if (timer.remainingSeconds <= 0.0f)
                {
                    timer.remainingSeconds = timer.intervalSeconds;
                }
                ++i;
                continue;
            }

            JS_FreeValue(context_, timer.callback);
            entry.timers.erase(entry.timers.begin() + static_cast<long long>(i));
        }
        entry.tickingTimers = false;
    }

    JSValue ScriptSystem::JsLog(JSContext* ctx, JSValueConst thisVal, int argc, JSValueConst* argv)
    {
        (void)thisVal;
        auto* scope = static_cast<InvokeScope*>(JS_GetContextOpaque(ctx));
        if (!scope || !scope->system || argc <= 0)
        {
            return JS_UNDEFINED;
        }

        const char* message = JS_ToCString(ctx, argv[0]);
        if (!message)
        {
            return JS_UNDEFINED;
        }

        SHINE_LOG_INFO(ScriptSystemLog, "ScriptRuntime", "[JS] {}", message);
        JS_FreeCString(ctx, message);
        return JS_UNDEFINED;
    }

    JSValue ScriptSystem::JsSetTimeout(JSContext* ctx, JSValueConst thisVal, int argc, JSValueConst* argv)
    {
        (void)thisVal;
        auto* scope = static_cast<InvokeScope*>(JS_GetContextOpaque(ctx));
        if (!scope || !scope->system || argc < 2 || !JS_IsFunction(ctx, argv[0]))
        {
            return JS_NewInt32(ctx, 0);
        }

        double delayMs = 0.0;
        JS_ToFloat64(ctx, &delayMs, argv[1]);
        if (delayMs < 0.0)
        {
            delayMs = 0.0;
        }

        const auto it = scope->system->loadedScripts_.find(scope->handle.id);
        if (it == scope->system->loadedScripts_.end())
        {
            return JS_NewInt32(ctx, 0);
        }
        auto& entry = it->second;
        const uint32_t timerId = entry.nextTimerId++;
        ScriptEntry::TimerEntry timer;
        timer.id = timerId;
        timer.remainingSeconds = static_cast<float>(delayMs / 1000.0);
        timer.intervalSeconds = timer.remainingSeconds;
        timer.repeat = false;
        timer.callback = JS_DupValue(ctx, argv[0]);
        entry.timers.emplace_back(std::move(timer));
        return JS_NewInt32(ctx, static_cast<int32_t>(timerId));
    }

    JSValue ScriptSystem::JsSetInterval(JSContext* ctx, JSValueConst thisVal, int argc, JSValueConst* argv)
    {
        (void)thisVal;
        auto* scope = static_cast<InvokeScope*>(JS_GetContextOpaque(ctx));
        if (!scope || !scope->system || argc < 2 || !JS_IsFunction(ctx, argv[0]))
        {
            return JS_NewInt32(ctx, 0);
        }

        double intervalMs = 0.0;
        JS_ToFloat64(ctx, &intervalMs, argv[1]);
        if (intervalMs <= 0.0)
        {
            intervalMs = 1.0;
        }

        const auto it = scope->system->loadedScripts_.find(scope->handle.id);
        if (it == scope->system->loadedScripts_.end())
        {
            return JS_NewInt32(ctx, 0);
        }
        auto& entry = it->second;
        const uint32_t timerId = entry.nextTimerId++;
        ScriptEntry::TimerEntry timer;
        timer.id = timerId;
        timer.remainingSeconds = static_cast<float>(intervalMs / 1000.0);
        timer.intervalSeconds = timer.remainingSeconds;
        timer.repeat = true;
        timer.callback = JS_DupValue(ctx, argv[0]);
        entry.timers.emplace_back(std::move(timer));
        return JS_NewInt32(ctx, static_cast<int32_t>(timerId));
    }

    JSValue ScriptSystem::JsClearTimer(JSContext* ctx, JSValueConst thisVal, int argc, JSValueConst* argv)
    {
        (void)thisVal;
        auto* scope = static_cast<InvokeScope*>(JS_GetContextOpaque(ctx));
        if (!scope || !scope->system || argc < 1)
        {
            return JS_NewBool(ctx, false);
        }

        int timerId = 0;
        JS_ToInt32(ctx, &timerId, argv[0]);
        const auto it = scope->system->loadedScripts_.find(scope->handle.id);
        if (it == scope->system->loadedScripts_.end())
        {
            return JS_NewBool(ctx, false);
        }

        auto& timers = it->second.timers;
        for (size_t i = 0; i < timers.size(); ++i)
        {
            if (timers[i].id == static_cast<uint32_t>(timerId))
            {
                if (it->second.tickingTimers)
                {
                    timers[i].cancelled = true;
                }
                else
                {
                    JS_FreeValue(ctx, timers[i].callback);
                    timers.erase(timers.begin() + static_cast<long long>(i));
                }
                return JS_NewBool(ctx, true);
            }
        }
        return JS_NewBool(ctx, false);
    }

    JSValue ScriptSystem::JsSetScriptEnabled(JSContext* ctx, JSValueConst thisVal, int argc, JSValueConst* argv)
    {
        (void)thisVal;
        auto* scope = static_cast<InvokeScope*>(JS_GetContextOpaque(ctx));
        if (!scope || !scope->system || argc < 1)
        {
            return JS_NewBool(ctx, false);
        }

        const auto it = scope->system->loadedScripts_.find(scope->handle.id);
        if (it == scope->system->loadedScripts_.end())
        {
            return JS_NewBool(ctx, false);
        }
        const bool enabled = JS_ToBool(ctx, argv[0]) != 0;
        return JS_NewBool(ctx, scope->system->SetScriptRuntimeEnabled(scope->handle, enabled, scope->owner));
    }

    JSValue ScriptSystem::JsSetScriptTickInterval(JSContext* ctx, JSValueConst thisVal, int argc, JSValueConst* argv)
    {
        (void)thisVal;
        auto* scope = static_cast<InvokeScope*>(JS_GetContextOpaque(ctx));
        if (!scope || !scope->owner || argc < 1)
        {
            return JS_NewBool(ctx, false);
        }

        double intervalSeconds = 0.0;
        JS_ToFloat64(ctx, &intervalSeconds, argv[0]);
        if (intervalSeconds < 0.0)
        {
            intervalSeconds = 0.0;
        }

        auto* scriptComp = scope->owner->getComponent<shine::gameplay::component::ScriptComponent>();
        if (!scriptComp)
        {
            return JS_NewBool(ctx, false);
        }
        scriptComp->setTickInterval(static_cast<float>(intervalSeconds));
        return JS_NewBool(ctx, true);
    }

    JSValue ScriptSystem::JsReflectGetField(JSContext* ctx, JSValueConst thisVal, int argc, JSValueConst* argv)
    {
        (void)thisVal;
        auto* scope = static_cast<InvokeScope*>(JS_GetContextOpaque(ctx));
        if (!scope || !scope->system || argc < 3)
        {
            return JS_UNDEFINED;
        }

        int32_t actorId = 0;
        JS_ToInt32(ctx, &actorId, argv[0]);
        const char* typeName = JS_ToCString(ctx, argv[1]);
        const char* fieldName = JS_ToCString(ctx, argv[2]);
        if (!typeName || !fieldName)
        {
            if (typeName)
            {
                JS_FreeCString(ctx, typeName);
            }
            if (fieldName)
            {
                JS_FreeCString(ctx, fieldName);
            }
            return JS_UNDEFINED;
        }

        auto* actor = scope->system->FindActorById(actorId);
        const auto* typeInfo = reflection::TypeRegistry::Get().FindByNameFast(typeName);
        const auto* fieldInfo = typeInfo ? typeInfo->FindField(fieldName) : nullptr;
        if (!actor || !fieldInfo)
        {
            JS_FreeCString(ctx, fieldName);
            JS_FreeCString(ctx, typeName);
            return JS_UNDEFINED;
        }

        auto valueBuffer = std::make_unique<char[]>(fieldInfo->size);
        if (!fieldInfo->isPod)
        {
            reflection::Construct(valueBuffer.get(), fieldInfo->typeId);
        }
        fieldInfo->Get(actor, valueBuffer.get());
        QuickJSScriptBridge bridge(ctx);
        JSValue result = bridge.ToJSValue(bridge.ToScript(valueBuffer.get(), fieldInfo->typeId));
        if (!fieldInfo->isPod)
        {
            reflection::Destruct(valueBuffer.get(), fieldInfo->typeId);
        }

        JS_FreeCString(ctx, fieldName);
        JS_FreeCString(ctx, typeName);
        return result;
    }

    JSValue ScriptSystem::JsReflectSetField(JSContext* ctx, JSValueConst thisVal, int argc, JSValueConst* argv)
    {
        (void)thisVal;
        auto* scope = static_cast<InvokeScope*>(JS_GetContextOpaque(ctx));
        if (!scope || !scope->system || argc < 4)
        {
            return JS_NewBool(ctx, false);
        }

        int32_t actorId = 0;
        JS_ToInt32(ctx, &actorId, argv[0]);
        const char* typeName = JS_ToCString(ctx, argv[1]);
        const char* fieldName = JS_ToCString(ctx, argv[2]);
        if (!typeName || !fieldName)
        {
            if (typeName)
            {
                JS_FreeCString(ctx, typeName);
            }
            if (fieldName)
            {
                JS_FreeCString(ctx, fieldName);
            }
            return JS_NewBool(ctx, false);
        }

        auto* actor = scope->system->FindActorById(actorId);
        const auto* typeInfo = reflection::TypeRegistry::Get().FindByNameFast(typeName);
        const auto* fieldInfo = typeInfo ? typeInfo->FindField(fieldName) : nullptr;
        if (!actor || !fieldInfo || !fieldInfo->setterFn)
        {
            JS_FreeCString(ctx, fieldName);
            JS_FreeCString(ctx, typeName);
            return JS_NewBool(ctx, false);
        }

        QuickJSScriptBridge bridge(ctx);
        reflection::ScriptValue value = bridge.FromJSValue(argv[3]);
        auto valueBuffer = std::make_unique<char[]>(fieldInfo->size);
        if (!fieldInfo->isPod)
        {
            reflection::Construct(valueBuffer.get(), fieldInfo->typeId);
        }
        bridge.FromScript(value, valueBuffer.get(), fieldInfo->typeId);
        fieldInfo->Set(actor, valueBuffer.get());
        if (!fieldInfo->isPod)
        {
            reflection::Destruct(valueBuffer.get(), fieldInfo->typeId);
        }

        JS_FreeCString(ctx, fieldName);
        JS_FreeCString(ctx, typeName);
        return JS_NewBool(ctx, true);
    }

    JSValue ScriptSystem::JsReflectCallMethod(JSContext* ctx, JSValueConst thisVal, int argc, JSValueConst* argv)
    {
        (void)thisVal;
        auto* scope = static_cast<InvokeScope*>(JS_GetContextOpaque(ctx));
        if (!scope || !scope->system || argc < 3)
        {
            return JS_UNDEFINED;
        }

        int32_t actorId = 0;
        JS_ToInt32(ctx, &actorId, argv[0]);
        const char* typeName = JS_ToCString(ctx, argv[1]);
        const char* methodName = JS_ToCString(ctx, argv[2]);
        if (!typeName || !methodName)
        {
            if (typeName)
            {
                JS_FreeCString(ctx, typeName);
            }
            if (methodName)
            {
                JS_FreeCString(ctx, methodName);
            }
            return JS_UNDEFINED;
        }

        auto* actor = scope->system->FindActorById(actorId);
        const auto* typeInfo = reflection::TypeRegistry::Get().FindByNameFast(typeName);
        if (!actor || !typeInfo)
        {
            JS_FreeCString(ctx, methodName);
            JS_FreeCString(ctx, typeName);
            return JS_UNDEFINED;
        }

        reflection::ScriptView view;
        view.typeInfo = typeInfo;
        const auto* methodInfo = view.GetMethodInfo(methodName);
        if (!methodInfo)
        {
            JS_FreeCString(ctx, methodName);
            JS_FreeCString(ctx, typeName);
            return JS_UNDEFINED;
        }

        QuickJSScriptBridge bridge(ctx);
        std::vector<reflection::ScriptValue> args;
        args.reserve(static_cast<size_t>(argc - 3));
        for (int i = 3; i < argc; ++i)
        {
            args.emplace_back(bridge.FromJSValue(argv[i]));
        }

        reflection::ScriptValue result = view.CallMethod(actor, methodInfo, args, bridge);
        JS_FreeCString(ctx, methodName);
        JS_FreeCString(ctx, typeName);
        return bridge.ToJSValue(result);
    }

    bool ScriptSystem::InstallBindings()
    {
        if (!context_)
        {
            return false;
        }

        JS_SetContextOpaque(context_, &invokeScope_);
        JSValue globalObj = JS_GetGlobalObject(context_);
        struct BindingDef
        {
            const char* name;
            JSCFunction* function;
            int argc;
        };
        static constexpr std::array bindings{
            BindingDef{ "Log", &ScriptSystem::JsLog, 1 },
            BindingDef{ "SetTimeout", &ScriptSystem::JsSetTimeout, 2 },
            BindingDef{ "SetInterval", &ScriptSystem::JsSetInterval, 2 },
            BindingDef{ "ClearTimer", &ScriptSystem::JsClearTimer, 1 },
            BindingDef{ "SetScriptEnabled", &ScriptSystem::JsSetScriptEnabled, 1 },
            BindingDef{ "SetScriptTickInterval", &ScriptSystem::JsSetScriptTickInterval, 1 },
            BindingDef{ "ReflectGetField", &ScriptSystem::JsReflectGetField, 3 },
            BindingDef{ "ReflectSetField", &ScriptSystem::JsReflectSetField, 4 },
            BindingDef{ "ReflectCallMethod", &ScriptSystem::JsReflectCallMethod, 3 }
        };
        for (const auto& binding : bindings)
        {
            JSValue function = JS_NewCFunction(context_, binding.function, binding.name, binding.argc);
            JS_SetPropertyStr(context_, globalObj, binding.name, function);
        }

        JS_FreeValue(context_, globalObj);
        return true;
    }

    bool ScriptSystem::EvaluateScriptText(STextView scriptPath, std::string_view scriptText, ScriptEntry& outEntry)
    {
        outEntry.initFunc = JS_UNDEFINED;
        outEntry.startFunc = JS_UNDEFINED;
        outEntry.updateFunc = JS_UNDEFINED;
        outEntry.destroyFunc = JS_UNDEFINED;
        outEntry.onEnableFunc = JS_UNDEFINED;
        outEntry.onDisableFunc = JS_UNDEFINED;
        outEntry.startCalled = false;
        outEntry.enableNotified = false;
        outEntry.scriptEnabled = true;
        outEntry.nextTimerId = 1;
        outEntry.tickingTimers = false;
        outEntry.timers.clear();
        outEntry.className = {};
        outEntry.functions.clear();
        outEntry.properties.clear();

        const std::filesystem::path resolvedPath = ResolveScriptPath(scriptPath);
        const std::filesystem::path helperPath = resolvedPath.parent_path() / "shine_decorators.js";
        if (helperPath != resolvedPath && std::filesystem::exists(helperPath))
        {
            const auto helperTextResult = util::read_file_text(shine::SString(helperPath.string()).view());
            if (helperTextResult.has_value())
            {
                const std::string helperPathString = helperPath.string();
                JSValue helperEvalResult = JS_Eval(
                    context_,
                    helperTextResult.value().data(),
                    helperTextResult.value().size(),
                    helperPathString.c_str(),
                    JS_EVAL_TYPE_GLOBAL
                );
                if (JS_IsException(helperEvalResult))
                {
                    ReportException(STextView::from_literal("EvaluateDecoratorLib"), shine::SString(helperPathString).view());
                    JS_FreeValue(context_, helperEvalResult);
                    return false;
                }
                JS_FreeValue(context_, helperEvalResult);
            }
        }

        const std::string pathString = resolvedPath.string();
        JSValue evalResult = JS_Eval(context_, scriptText.data(), scriptText.size(), pathString.c_str(), JS_EVAL_TYPE_GLOBAL);
        if (JS_IsException(evalResult))
        {
            ReportException(STextView::from_literal("EvaluateScript"), scriptPath);
            JS_FreeValue(context_, evalResult);
            return false;
        }
        JS_FreeValue(context_, evalResult);

        JSValue globalObj = JS_GetGlobalObject(context_);
        auto readLifecycleFunc = [&](const char* lowerName, const char* upperName)
        {
            JSValue fn = JS_GetPropertyStr(context_, globalObj, lowerName);
            if (!JS_IsFunction(context_, fn))
            {
                JS_FreeValue(context_, fn);
                fn = JS_GetPropertyStr(context_, globalObj, upperName);
                if (!JS_IsFunction(context_, fn))
                {
                    JS_FreeValue(context_, fn);
                    return JS_UNDEFINED;
                }
            }
            return fn;
        };

        outEntry.initFunc = readLifecycleFunc("init", "Init");
        outEntry.startFunc = readLifecycleFunc("start", "Start");
        outEntry.updateFunc = readLifecycleFunc("update", "Update");
        outEntry.destroyFunc = readLifecycleFunc("destroy", "Destroy");
        outEntry.onEnableFunc = readLifecycleFunc("onEnable", "OnEnable");
        outEntry.onDisableFunc = readLifecycleFunc("onDisable", "OnDisable");
        ParseScriptMetadata(outEntry, globalObj);
        JS_FreeValue(context_, globalObj);
        return true;
    }

    void ScriptSystem::ParseScriptMetadata(ScriptEntry& entry, JSValueConst globalObj)
    {
        entry.className = {};
        entry.functions.clear();
        entry.properties.clear();

        auto readArrayLength = [&](JSValueConst value) -> uint32_t
        {
            uint32_t length = 0;
            JSValue lengthVal = JS_GetPropertyStr(context_, value, "length");
            JS_ToUint32(context_, &length, lengthVal);
            JS_FreeValue(context_, lengthVal);
            return length;
        };

        JSValue metaRoot = JS_GetPropertyStr(context_, globalObj, "__shine_meta");
        if (JS_IsObject(metaRoot))
        {
            JSValue classesArray = JS_GetPropertyStr(context_, metaRoot, "classes");
            if (JS_IsArray(classesArray))
            {
                const uint32_t classCount = readArrayLength(classesArray);
                if (classCount > 0)
                {
                    JSValue firstClass = JS_GetPropertyUint32(context_, classesArray, 0);
                    if (JS_IsObject(firstClass))
                    {
                        JSValue classNameVal = JS_GetPropertyStr(context_, firstClass, "name");
                        const char* classNameText = JS_IsString(classNameVal) ? JS_ToCString(context_, classNameVal) : nullptr;
                        if (classNameText)
                        {
                            entry.className = shine::SString(classNameText);
                            JS_FreeCString(context_, classNameText);
                        }
                        JS_FreeValue(context_, classNameVal);

                        JSValue propertiesArray = JS_GetPropertyStr(context_, firstClass, "properties");
                        if (JS_IsArray(propertiesArray))
                        {
                            const uint32_t propertyCount = readArrayLength(propertiesArray);
                            for (uint32_t i = 0; i < propertyCount; ++i)
                            {
                                JSValue item = JS_GetPropertyUint32(context_, propertiesArray, i);
                                if (!JS_IsObject(item))
                                {
                                    JS_FreeValue(context_, item);
                                    continue;
                                }
                                JSValue nameVal = JS_GetPropertyStr(context_, item, "name");
                                JSValue typeVal = JS_GetPropertyStr(context_, item, "type");
                                const char* nameText = JS_IsString(nameVal) ? JS_ToCString(context_, nameVal) : nullptr;
                                const char* typeText = JS_IsString(typeVal) ? JS_ToCString(context_, typeVal) : nullptr;
                                if (nameText && typeText)
                                {
                                    entry.properties.push_back(ScriptEntry::PropertyMeta{
                                        .name = shine::SString(nameText),
                                        .type = shine::SString(typeText)
                                    });
                                }
                                if (nameText)
                                {
                                    JS_FreeCString(context_, nameText);
                                }
                                if (typeText)
                                {
                                    JS_FreeCString(context_, typeText);
                                }
                                JS_FreeValue(context_, typeVal);
                                JS_FreeValue(context_, nameVal);
                                JS_FreeValue(context_, item);
                            }
                        }
                        JS_FreeValue(context_, propertiesArray);

                        JSValue functionsArray = JS_GetPropertyStr(context_, firstClass, "functions");
                        if (JS_IsArray(functionsArray))
                        {
                            const uint32_t functionCount = readArrayLength(functionsArray);
                            for (uint32_t i = 0; i < functionCount; ++i)
                            {
                                JSValue item = JS_GetPropertyUint32(context_, functionsArray, i);
                                if (!JS_IsObject(item))
                                {
                                    JS_FreeValue(context_, item);
                                    continue;
                                }
                                JSValue nameVal = JS_GetPropertyStr(context_, item, "name");
                                const char* nameText = JS_IsString(nameVal) ? JS_ToCString(context_, nameVal) : nullptr;
                                if (nameText)
                                {
                                    entry.functions.push_back(ScriptEntry::FunctionMeta{
                                        .name = shine::SString(nameText)
                                    });
                                    JS_FreeCString(context_, nameText);
                                }
                                JS_FreeValue(context_, nameVal);
                                JS_FreeValue(context_, item);
                            }
                        }
                        JS_FreeValue(context_, functionsArray);
                    }
                    JS_FreeValue(context_, firstClass);
                }
            }
            JS_FreeValue(context_, classesArray);
        }
        JS_FreeValue(context_, metaRoot);

        if (entry.className.empty())
        {
            JSValue propsArray = JS_GetPropertyStr(context_, globalObj, "__shine_props");
            if (JS_IsArray(propsArray))
            {
                const uint32_t count = readArrayLength(propsArray);
                for (uint32_t i = 0; i < count; ++i)
                {
                    JSValue item = JS_GetPropertyUint32(context_, propsArray, i);
                    if (!JS_IsObject(item))
                    {
                        JS_FreeValue(context_, item);
                        continue;
                    }
                    JSValue nameVal = JS_GetPropertyStr(context_, item, "name");
                    JSValue typeVal = JS_GetPropertyStr(context_, item, "type");
                    const char* nameText = JS_IsString(nameVal) ? JS_ToCString(context_, nameVal) : nullptr;
                    const char* typeText = JS_IsString(typeVal) ? JS_ToCString(context_, typeVal) : nullptr;
                    if (nameText && typeText)
                    {
                        entry.properties.push_back(ScriptEntry::PropertyMeta{
                            .name = shine::SString(nameText),
                            .type = shine::SString(typeText)
                        });
                    }
                    if (nameText)
                    {
                        JS_FreeCString(context_, nameText);
                    }
                    if (typeText)
                    {
                        JS_FreeCString(context_, typeText);
                    }
                    JS_FreeValue(context_, typeVal);
                    JS_FreeValue(context_, nameVal);
                    JS_FreeValue(context_, item);
                }
            }
            JS_FreeValue(context_, propsArray);
        }

        if (!entry.className.empty() || !entry.properties.empty() || !entry.functions.empty())
        {
            SHINE_LOG_INFO(
                ScriptSystemLog,
                "ScriptRuntime",
                "解析脚本元数据: class={} properties={} functions={}",
                entry.className.empty() ? shine::SString("None").to_string() : entry.className.to_string(),
                entry.properties.size(),
                entry.functions.size()
            );
        }
    }

    void ScriptSystem::ReleaseScriptEntry(ScriptEntry& entry)
    {
        if (context_ && !JS_IsUndefined(entry.initFunc))
        {
            JS_FreeValue(context_, entry.initFunc);
        }
        if (context_ && !JS_IsUndefined(entry.startFunc))
        {
            JS_FreeValue(context_, entry.startFunc);
        }
        if (context_ && !JS_IsUndefined(entry.updateFunc))
        {
            JS_FreeValue(context_, entry.updateFunc);
        }
        if (context_ && !JS_IsUndefined(entry.destroyFunc))
        {
            JS_FreeValue(context_, entry.destroyFunc);
        }
        if (context_ && !JS_IsUndefined(entry.onEnableFunc))
        {
            JS_FreeValue(context_, entry.onEnableFunc);
        }
        if (context_ && !JS_IsUndefined(entry.onDisableFunc))
        {
            JS_FreeValue(context_, entry.onDisableFunc);
        }
        if (context_)
        {
            for (auto& timer : entry.timers)
            {
                if (!JS_IsUndefined(timer.callback))
                {
                    JS_FreeValue(context_, timer.callback);
                }
            }
        }
        entry.timers.clear();
        entry.initFunc = JS_UNDEFINED;
        entry.startFunc = JS_UNDEFINED;
        entry.updateFunc = JS_UNDEFINED;
        entry.destroyFunc = JS_UNDEFINED;
        entry.onEnableFunc = JS_UNDEFINED;
        entry.onDisableFunc = JS_UNDEFINED;
        entry.startCalled = false;
        entry.enableNotified = false;
        entry.scriptEnabled = true;
        entry.nextTimerId = 1;
        entry.tickingTimers = false;
        entry.className = {};
        entry.functions.clear();
        entry.properties.clear();
    }

    void ScriptSystem::ReportException(STextView stage, STextView scriptPath) const
    {
        if (!context_)
        {
            return;
        }
        JSValue error = JS_GetException(context_);
        const char* errorText = JS_ToCString(context_, error);
        if (errorText)
        {
            SHINE_LOG_ERROR(
                ScriptSystemLog,
                "ScriptError",
                "QuickJS异常 stage={} path={} msg={}",
                shine::SString(stage).to_string(),
                shine::SString(scriptPath).to_string(),
                errorText
            );
            JS_FreeCString(context_, errorText);
        }

        JSValue stack = JS_GetPropertyStr(context_, error, "stack");
        if (!JS_IsUndefined(stack))
        {
            const char* stackText = JS_ToCString(context_, stack);
            if (stackText)
            {
                SHINE_LOG_ERROR(ScriptSystemLog, "ScriptError", "{}", stackText);
                JS_FreeCString(context_, stackText);
            }
        }
        JS_FreeValue(context_, stack);
        JS_FreeValue(context_, error);
    }

    std::filesystem::path ScriptSystem::ResolveScriptPath(STextView scriptPath) const
    {
        std::filesystem::path path(shine::SString(scriptPath).to_string());
        if (path.is_absolute())
        {
            return path;
        }
        return std::filesystem::absolute(path);
    }
}
