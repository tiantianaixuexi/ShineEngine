#pragma once
#include "ReflectionHash.h"


namespace shine::reflection {

    struct TypeInfo;

    struct ScriptValue {
        enum class Type { Null,
                          Bool,
                          Int64,
                          Double,
                          Pointer };
        Type   type      = Type::Null;
        TypeId ptrTypeId = 0; // For Pointer type
        union {
            bool    bValue;
            int64_t iValue;
            double  dValue;
            void   *pValue;
        };
        ScriptValue() : type(Type::Null), pValue(nullptr) {}

        ScriptValue(bool v) : type(Type::Bool), bValue(v) {}
        ScriptValue(int64_t v) : type(Type::Int64), iValue(v) {}
        ScriptValue(double v) : type(Type::Double), dValue(v) {}
        ScriptValue(void *v, TypeId tid = 0) : type(Type::Pointer), pValue(v), ptrTypeId(tid) {}
        template <typename T>
        T As() const;
    };

    template <>
    inline bool ScriptValue::As<bool>() const { return type == Type::Bool ? bValue : false; }
    template <>
    inline int64_t ScriptValue::As<int64_t>() const { return type == Type::Int64 ? iValue : (type == Type::Double ? static_cast<int64_t>(dValue) : 0); }
    template <>
    inline double ScriptValue::As<double>() const { return type == Type::Double ? dValue : (type == Type::Int64 ? static_cast<double>(iValue) : 0.0); }
    template <>
    inline void *ScriptValue::As<void *>() const { return type == Type::Pointer ? pValue : nullptr; }
    template <>
    inline int ScriptValue::As<int>() const { return static_cast<int>(As<int64_t>()); }
    template <>
    inline float ScriptValue::As<float>() const { return static_cast<float>(As<double>()); }

    struct ScriptBridge {
        using ToScriptFunc        = ScriptValue (*)(void *context, const void *src, TypeId typeId);
        using FromScriptFunc      = void (*)(void *context, const ScriptValue &val, void *dst, TypeId typeId);
        void          *context    = nullptr;
        ToScriptFunc   toScript   = nullptr;
        FromScriptFunc fromScript = nullptr;
        ScriptValue    ToScript(const void *src, TypeId typeId) const {
            return toScript ? toScript(context, src, typeId) : ScriptValue();
        }
        void FromScript(const ScriptValue &val, void *dst, TypeId typeId) const {
            if (fromScript)
                fromScript(context, val, dst, typeId);
        }
    };



    inline void MemcpyGetter(const void *instance, void *out_value, size_t offset, size_t size) {
        std::memcpy(out_value, static_cast<const char *>(instance) + offset, size);
    }
    inline void MemcpySetter(void *instance, const void *in_value, size_t offset, size_t size) {
        std::memcpy(static_cast<char *>(instance) + offset, in_value, size);
    }
    template <typename Class, typename MemberType, MemberType Class::*Member>
    void GenericGetter(const void *instance, void *out_value, size_t /*offset*/, size_t /*size*/) {
        const auto *obj                       = static_cast<const Class *>(instance);
        *static_cast<MemberType *>(out_value) = obj->*Member;
    }
    template <typename Class, typename MemberType, MemberType Class::*Member>
    void GenericSetter(void *instance, const void *in_value, size_t /*offset*/, size_t /*size*/) {
        auto *obj    = static_cast<Class *>(instance);
        obj->*Member = *static_cast<const MemberType *>(in_value);
    }

} // namespace Shine::Reflection
