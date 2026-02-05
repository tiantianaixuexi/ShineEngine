#pragma once

#include <string>
#include <string_view>
#include <system_error>
#include <optional>
#include <variant>
#include <source_location>

#ifdef SHINE_USE_MODULE
import shine.util.shineLog;
#else
#include "../../util/shineLog/shineLog.h"
#endif

namespace shine::reflection {

    /**
     * @brief 反射系统错误码枚举
     */
    enum class ReflectionErrorCode {
        Success = 0,
        
        // 类型相关错误
        TypeNotFound = 1001,
        TypeAlreadyRegistered = 1002,
        InvalidTypeCast = 1003,
        TypeMismatch = 1004,
        
        // 字段相关错误
        FieldNotFound = 2001,
        FieldNotAccessible = 2002,
        FieldReadOnly = 2003,
        InvalidFieldOffset = 2004,
        
        // 方法相关错误
        MethodNotFound = 3001,
        MethodNotCallable = 3002,
        InvalidMethodSignature = 3003,
        ParameterCountMismatch = 3004,
        
        // 内存相关错误
        OutOfMemory = 4001,
        InvalidMemoryAccess = 4002,
        BufferTooSmall = 4003,
        
        // 序列化相关错误
        SerializationFailed = 5001,
        DeserializationFailed = 5002,
        InvalidFormat = 5003,
        
        // 脚本集成错误
        ScriptBindingFailed = 6001,
        ScriptExecutionError = 6002,
        InvalidScriptValue = 6003,
        
        // 容器操作错误
        ContainerOperationFailed = 7001,
        InvalidContainerIndex = 7002,
        ContainerFull = 7003,
        
        // 内部错误
        InternalError = 9001,
        NotImplemented = 9002,
        InvalidState = 9003
    };

    /**
     * @brief 反射错误信息结构
     */
    struct ReflectionError {
        ReflectionErrorCode code = ReflectionErrorCode::Success;
        std::string message;
        std::string context;
        std::source_location location;
        
        ReflectionError() = default;
        
        ReflectionError(ReflectionErrorCode err_code, 
                       std::string_view msg = "",
                       std::string_view ctx = "",
                       std::source_location loc = std::source_location::current())
            : code(err_code)
            , message(msg)
            , context(ctx)
            , location(loc) {}
            
        bool IsError() const { return code != ReflectionErrorCode::Success; }
        bool IsSuccess() const { return code == ReflectionErrorCode::Success; }
        
        std::string ToString() const {
            if (IsSuccess()) {
                return "Success";
            }
            
            std::string result = "[" + std::to_string(static_cast<int>(code)) + "] ";
            result += GetErrorCodeString(code);
            
            if (!message.empty()) {
                result += ": " + message;
            }
            
            if (!context.empty()) {
                result += " (Context: " + context + ")";
            }
            
            result += " at " + std::string(location.file_name()) + 
                     ":" + std::to_string(location.line());
                     
            return result;
        }
        
    private:
        static std::string GetErrorCodeString(ReflectionErrorCode code) {
            switch (code) {
                case ReflectionErrorCode::Success: return "Success";
                case ReflectionErrorCode::TypeNotFound: return "Type not found";
                case ReflectionErrorCode::TypeAlreadyRegistered: return "Type already registered";
                case ReflectionErrorCode::InvalidTypeCast: return "Invalid type cast";
                case ReflectionErrorCode::TypeMismatch: return "Type mismatch";
                case ReflectionErrorCode::FieldNotFound: return "Field not found";
                case ReflectionErrorCode::FieldNotAccessible: return "Field not accessible";
                case ReflectionErrorCode::FieldReadOnly: return "Field is read-only";
                case ReflectionErrorCode::InvalidFieldOffset: return "Invalid field offset";
                case ReflectionErrorCode::MethodNotFound: return "Method not found";
                case ReflectionErrorCode::MethodNotCallable: return "Method not callable";
                case ReflectionErrorCode::InvalidMethodSignature: return "Invalid method signature";
                case ReflectionErrorCode::ParameterCountMismatch: return "Parameter count mismatch";
                case ReflectionErrorCode::OutOfMemory: return "Out of memory";
                case ReflectionErrorCode::InvalidMemoryAccess: return "Invalid memory access";
                case ReflectionErrorCode::BufferTooSmall: return "Buffer too small";
                case ReflectionErrorCode::SerializationFailed: return "Serialization failed";
                case ReflectionErrorCode::DeserializationFailed: return "Deserialization failed";
                case ReflectionErrorCode::InvalidFormat: return "Invalid format";
                case ReflectionErrorCode::ScriptBindingFailed: return "Script binding failed";
                case ReflectionErrorCode::ScriptExecutionError: return "Script execution error";
                case ReflectionErrorCode::InvalidScriptValue: return "Invalid script value";
                case ReflectionErrorCode::ContainerOperationFailed: return "Container operation failed";
                case ReflectionErrorCode::InvalidContainerIndex: return "Invalid container index";
                case ReflectionErrorCode::ContainerFull: return "Container full";
                case ReflectionErrorCode::InternalError: return "Internal error";
                case ReflectionErrorCode::NotImplemented: return "Not implemented";
                case ReflectionErrorCode::InvalidState: return "Invalid state";
                default: return "Unknown error";
            }
        }
    };

    /**
     * @brief 结果类型包装器
     */
    template<typename T>
    class Result {
    private:
        std::variant<T, ReflectionError> value_;
        
    public:
        Result() : value_(ReflectionError(ReflectionErrorCode::InternalError, "Uninitialized result")) {}
        
        Result(const T& value) : value_(value) {}
        Result(T&& value) : value_(std::move(value)) {}
        Result(const ReflectionError& error) : value_(error) {}
        Result(ReflectionError&& error) : value_(std::move(error)) {}
        
        bool IsSuccess() const { 
            return std::holds_alternative<T>(value_); 
        }
        
        bool IsError() const { 
            return std::holds_alternative<ReflectionError>(value_); 
        }
        
        const T& GetValue() const {
            if (IsError()) {
                throw std::runtime_error("Attempt to get value from error result");
            }
            return std::get<T>(value_);
        }
        
        T& GetValue() {
            if (IsError()) {
                throw std::runtime_error("Attempt to get value from error result");
            }
            return std::get<T>(value_);
        }
        
        const ReflectionError& GetError() const {
            if (IsSuccess()) {
                throw std::runtime_error("Attempt to get error from success result");
            }
            return std::get<ReflectionError>(value_);
        }
        
        T GetValueOr(const T& default_value) const {
            return IsSuccess() ? GetValue() : default_value;
        }
        
        // 隐式转换操作符（谨慎使用）
        explicit operator bool() const { return IsSuccess(); }
        
        // 便捷方法
        static Result<T> Success(T&& value) {
            return Result<T>(std::move(value));
        }
        
        static Result<T> Success(const T& value) {
            return Result<T>(value);
        }
        
        static Result<T> Error(ReflectionErrorCode code, 
                              std::string_view message = "",
                              std::string_view context = "") {
            return Result<T>(ReflectionError(code, message, context));
        }
    };

    // 特化void类型的结果
    template<>
    class Result<void> {
    private:
        std::optional<ReflectionError> error_;
        
    public:
        Result() : error_(std::nullopt) {}
        Result(const ReflectionError& error) : error_(error) {}
        Result(ReflectionError&& error) : error_(std::move(error)) {}
        
        bool IsSuccess() const { return !error_.has_value(); }
        bool IsError() const { return error_.has_value(); }
        
        const ReflectionError& GetError() const {
            if (IsSuccess()) {
                throw std::runtime_error("Attempt to get error from success result");
            }
            return error_.value();
        }
        
        static Result<void> Success() {
            return Result<void>();
        }
        
        static Result<void> Error(ReflectionErrorCode code,
                                 std::string_view message = "",
                                 std::string_view context = "") {
            return Result<void>(ReflectionError(code, message, context));
        }
    };

    /**
     * @brief 错误处理助手类
     */
    class ErrorHandler {
    public:
        template<typename T>
        static void LogError(const Result<T>& result) {
            if (result.IsError()) {
                LogError(result.GetError());
            }
        }
        
        static void LogError(const ReflectionError& error) {
            shine::Log::Error("[Reflection] {}", error.ToString());
        }
        
        static void LogWarning(const ReflectionError& error) {
            shine::Log::Warn("[Reflection] {}", error.ToString());
        }
        
        static void LogInfo(const ReflectionError& error) {
            shine::Log::Info("[Reflection] {}", error.ToString());
        }
        
        template<typename T>
        static T GetOrDefault(Result<T>&& result, T&& default_value) {
            if (result.IsSuccess()) {
                return std::move(result.GetValue());
            } else {
                LogError(result);
                return std::move(default_value);
            }
        }
        
        static bool CheckAndLog(const ReflectionError& error) {
            if (error.IsError()) {
                LogError(error);
                return false;
            }
            return true;
        }
        
        // 异常安全的执行包装器
        template<typename Func, typename... Args>
        static auto SafeExecute(Func&& func, Args&&... args) 
            -> Result<std::invoke_result_t<Func, Args...>> {
            
            try {
                if constexpr (std::is_same_v<std::invoke_result_t<Func, Args...>, void>) {
                    func(std::forward<Args>(args)...);
                    return Result<void>::Success();
                } else {
                    return Result<std::invoke_result_t<Func, Args...>>::Success(
                        func(std::forward<Args>(args)...));
                }
            } catch (const std::exception& ex) {
                return Result<std::invoke_result_t<Func, Args...>>::Error(
                    ReflectionErrorCode::InternalError, 
                    ex.what(), 
                    "Exception in SafeExecute");
            } catch (...) {
                return Result<std::invoke_result_t<Func, Args...>>::Error(
                    ReflectionErrorCode::InternalError, 
                    "Unknown exception occurred", 
                    "Exception in SafeExecute");
            }
        }
    };

    /**
     * @brief 断言宏定义
     */
    #define REFLECTION_ASSERT(condition, error_code, message) \
        do { \
            if (!(condition)) { \
                return ::shine::reflection::Result<T>::Error( \
                    error_code, message, #condition); \
            } \
        } while(0)

    #define REFLECTION_ASSERT_VOID(condition, error_code, message) \
        do { \
            if (!(condition)) { \
                return ::shine::reflection::Result<void>::Error( \
                    error_code, message, #condition); \
            } \
        } while(0)

    /**
     * @brief 便捷的错误创建函数
     */
    inline ReflectionError MakeError(ReflectionErrorCode code, 
                                   std::string_view message = "",
                                   std::string_view context = "") {
        return ReflectionError(code, message, context);
    }

    // 类型相关的错误创建函数
    inline ReflectionError TypeError(std::string_view type_name, std::string_view context = "") {
        return MakeError(ReflectionErrorCode::TypeNotFound, 
                        std::string("Type not found: ") + std::string(type_name), 
                        context);
    }

    inline ReflectionError FieldError(std::string_view field_name, std::string_view context = "") {
        return MakeError(ReflectionErrorCode::FieldNotFound, 
                        std::string("Field not found: ") + std::string(field_name), 
                        context);
    }

    inline ReflectionError MethodError(std::string_view method_name, std::string_view context = "") {
        return MakeError(ReflectionErrorCode::MethodNotFound, 
                        std::string("Method not found: ") + std::string(method_name), 
                        context);
    }

} // namespace shine::reflection