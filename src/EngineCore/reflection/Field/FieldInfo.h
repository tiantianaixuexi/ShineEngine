#pragma once

#include "EngineCore/reflection/ReflectionFlags.h"

#include <string_view>
#include <variant>
#include <vector>

#include "EngineCore/reflection/ReflectionHash.h"
#include "EngineCore/reflection/ReflectionUI.h"

namespace shine::reflection {

    using MetadataKey       = TypeId;
    using MetadataValue     = std::variant<int, float, bool, std::string_view>;
    using MetadataContainer = std::vector<std::pair<MetadataKey, MetadataValue>>;


    struct TypeOps {

        void (*onChange)(void *instance, const void *oldValue);
        bool (*equals)(const void *a, const void *b);
        void (*copy)(void *dst, const void *src);
    };

    using GetterFunc = void (*)(const void *instance, void *out_value, size_t offset, size_t size);
    using SetterFunc = void (*)(void *instance, const void *in_value, size_t offset, size_t size);

    

    struct FieldInfo {

        TypeId typeId;
        ContainerType containerType = ContainerType::None;
        size_t offset;
        size_t size;
        size_t alignment;

        GetterFunc getter;
        SetterFunc setter;


        bool isPod;

        const void   *containerTrait = nullptr; // Points to SequenceTrait or MapTrait

        PropertyFlags     flags = PropertyFlags::None;

        UI::Schema        uiSchema = UI::None{};
        std::string_view  name;
        
        MetadataContainer metadata;
        
        void (*onChange)(void *instance, const void *oldValue);
        bool (*equals)(const void *a, const void *b, size_t size);
        void (*copy)(void *dst, const void *src, size_t size);

        const MetadataValue *GetMeta(MetadataKey key) const {
            auto it = std::lower_bound(metadata.begin(), metadata.end(), key, [](const auto &pair, MetadataKey k) { return pair.first < k; });
            if (it != metadata.end() && it->first == key)
                return &it->second;
            return nullptr;
        }

        void Get(const void *instance, void *out_value) const { getter(instance, out_value, offset, size); }
        void Set(void *instance, const void *in_value) const { setter(instance, in_value, offset, size); }
    };

    struct MethodInfo {
        std::string_view name;
        using InvokeFunc = void (*)(void *instance, void **args, void *ret);
        InvokeFunc          invoke;
        TypeId              returnType;
        std::vector<TypeId> paramTypes;
        uint64_t            signatureHash = 0;
        FunctionFlags       flags         = FunctionFlags::None;
        MetadataContainer   metadata;
    };


} // namespace shine::reflection