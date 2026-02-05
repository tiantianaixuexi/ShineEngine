#pragma once

#include "FieldMeta.h"
#include "../ReflectionFlags.h"
#include "../ReflectionHash.h"
#include "../ReflectionUI.h"

#include <algorithm>
#include <string_view>
#include <vector>

namespace shine::reflection {

    using MetadataContainer = std::vector<std::pair<MetadataKey, MetadataValue>>;
    

    struct TypeInfo;
    
    // 直接使用函数指针替代FunctionTag
    using GetterFn = void(*)(const void* instance, void* out_value, size_t offset, size_t size);
    using SetterFn = void(*)(void* instance, const void* in_value, size_t offset, size_t size);
    using OnChangeFn = void(*)(void* instance, const void* oldValue);
    using EqualsFn = bool(*)(const void* a, const void* b, size_t size);
    using CopyFn = void(*)(void* dst, const void* src, size_t size);
    using InvokeFn = void(*)(void* instance, void** args, void* ret);

    struct TypeOps {
        OnChangeFn onChangeFn = nullptr;
        EqualsFn equalsFn = nullptr;
        CopyFn copyFn = nullptr;
    };

    struct FieldInfo {

        TypeId typeId;
        ContainerType containerType = ContainerType::None;
        size_t offset;
        size_t size;
        size_t alignment;

        // 现代化的函数指针
        GetterFn getterFn = nullptr;
        SetterFn setterFn = nullptr;
        OnChangeFn onChangeFn = nullptr;
        EqualsFn equalsFn = nullptr;
        CopyFn copyFn = nullptr;
        InvokeFn invokeFn = nullptr;

        bool isPod;

        const void   *containerTrait = nullptr; // Points to SequenceTrait or MapTrait

        PropertyFlags     flags = PropertyFlags::None;

        UI::Schema        uiSchema = UI::None{};
        std::string_view  name;
        
        MetadataContainer metadata;
        
        // 现代化的调用方法
        inline void Get(const void* instance, void* out_value) const {
            if (getterFn) {
                getterFn(instance, out_value, offset, size);
            }
        }
        
        inline void Set(void* instance, const void* in_value) const {
            if (setterFn) {
                setterFn(instance, in_value, offset, size);
            }
        }
        
        inline void OnChange(void* instance, const void* oldValue) const {
            if (onChangeFn) {
                onChangeFn(instance, oldValue);
            }
        }
        
        inline bool Equals(const void* a, const void* b) const {
            if (equalsFn) {
                return equalsFn(a, b, size);
            }
            return false;
        }
        
        inline void Copy(void* dst, const void* src) const {
            if (copyFn) {
                copyFn(dst, src, size);
            }
        }
        

        bool Equals(const void *a, const void *b, size_t size) const;
        void Copy(void *dst, const void *src, size_t size) const;
    };

    struct MethodInfo {
        std::string_view name;
        InvokeFn          invokeFn = nullptr;
        TypeId              returnType;
        std::vector<TypeId> paramTypes;
        uint64_t            signatureHash = 0;
        FunctionFlags       flags         = FunctionFlags::None;
        MetadataContainer   metadata;
        const TypeInfo     *owner = nullptr;

        void Invoke(void *instance, void **args, void *ret) const {
            if (invokeFn) {
                invokeFn(instance, args, ret);
            }
        }
    };


} // namespace shine::reflection