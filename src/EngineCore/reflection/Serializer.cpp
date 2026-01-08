#include "Serializer.h"
#include "../../third/yyjson/yyjson.h"
#include <iostream>
#include <vector>
#include <charconv>

#ifdef SHINE_USE_MODULE
import shine.memory;
#else
#include "../../memory/memory.ixx"
#endif

namespace Shine::Reflection {

    // -----------------------------------------------------------------------------
    // Helper: Type Checks
    // -----------------------------------------------------------------------------
    static bool IsPrimitive(TypeId id) {
        return id == GetTypeId<bool>() || 
               id == GetTypeId<int>() || id == GetTypeId<int64_t>() || 
               id == GetTypeId<uint32_t>() || id == GetTypeId<uint64_t>() ||
               id == GetTypeId<float>() || id == GetTypeId<double>() ||
               id == GetTypeId<std::string>() || id == GetTypeId<std::string_view>() ||
               id == GetTypeId<shine::SString>();
    }

    static void ConstructValue(void* ptr, TypeId typeId) {
        shine::co::MemoryScope scope(shine::co::MemoryTag::Reflection);
        if (typeId == GetTypeId<std::string>()) {
            new (ptr) std::string();
            return;
        }
        if (typeId == GetTypeId<shine::SString>()) {
            new (ptr) shine::SString();
            return;
        }
        const TypeInfo* info = TypeRegistry::Get().Find(typeId);
        if (info && info->construct) {
            info->construct(ptr);
        }
    }

    static void DestructValue(void* ptr, TypeId typeId) {
        if (typeId == GetTypeId<std::string>()) {
            static_cast<std::string*>(ptr)->~basic_string();
            return;
        }
        if (typeId == GetTypeId<shine::SString>()) {
            static_cast<shine::SString*>(ptr)->~SString();
            return;
        }
        const TypeInfo* info = TypeRegistry::Get().Find(typeId);
        if (info && info->destruct) {
            info->destruct(ptr);
        }
    }

    // -----------------------------------------------------------------------------
    // Writer Implementation
    // -----------------------------------------------------------------------------
    struct JsonWriterContext {
        yyjson_mut_doc* doc;
    };

    static yyjson_mut_val* SerializeValue(JsonWriterContext& ctx, const void* instance, TypeId typeId);
    static yyjson_mut_val* SerializeObject(JsonWriterContext& ctx, const void* instance, const TypeInfo* info);

    static yyjson_mut_val* SerializePrimitive(JsonWriterContext& ctx, const void* instance, TypeId typeId) {
        if (typeId == GetTypeId<bool>()) return yyjson_mut_bool(ctx.doc, *static_cast<const bool*>(instance));
        if (typeId == GetTypeId<int>()) return yyjson_mut_int(ctx.doc, *static_cast<const int*>(instance));
        if (typeId == GetTypeId<int64_t>()) return yyjson_mut_sint(ctx.doc, *static_cast<const int64_t*>(instance));
        if (typeId == GetTypeId<uint32_t>()) return yyjson_mut_uint(ctx.doc, *static_cast<const uint32_t*>(instance));
        if (typeId == GetTypeId<uint64_t>()) return yyjson_mut_uint(ctx.doc, *static_cast<const uint64_t*>(instance));
        if (typeId == GetTypeId<float>()) return yyjson_mut_real(ctx.doc, *static_cast<const float*>(instance));
        if (typeId == GetTypeId<double>()) return yyjson_mut_real(ctx.doc, *static_cast<const double*>(instance));
        if (typeId == GetTypeId<std::string>()) {
            const auto& str = *static_cast<const std::string*>(instance);
            return yyjson_mut_strn(ctx.doc, str.c_str(), str.size());
        }
        if (typeId == GetTypeId<std::string_view>()) {
            const auto& str = *static_cast<const std::string_view*>(instance);
            return yyjson_mut_strn(ctx.doc, str.data(), str.size());
        }
        return yyjson_mut_null(ctx.doc);
    }

    static yyjson_mut_val* SerializeSequence(JsonWriterContext& ctx, const void* instance, const SequenceTrait* trait) {
        yyjson_mut_val* arr = yyjson_mut_arr(ctx.doc);
        size_t size = trait->getSize(instance);
        for (size_t i = 0; i < size; ++i) {
            const void* elemPtr = trait->getElementConst(instance, i);
            yyjson_mut_arr_append(arr, SerializeValue(ctx, elemPtr, trait->elementTypeId));
        }
        return arr;
    }

    static std::string ToString(const void* instance, TypeId typeId) {
        if (typeId == GetTypeId<std::string>()) return *static_cast<const std::string*>(instance);
        if (typeId == GetTypeId<std::string_view>()) return std::string(*static_cast<const std::string_view*>(instance));
        if (typeId == GetTypeId<int>()) return std::to_string(*static_cast<const int*>(instance));
        if (typeId == GetTypeId<int64_t>()) return std::to_string(*static_cast<const int64_t*>(instance));
        // Fallback
        return "Key";
    }

    static yyjson_mut_val* SerializeMap(JsonWriterContext& ctx, const void* instance, const MapTrait* trait) {
        yyjson_mut_val* obj = yyjson_mut_obj(ctx.doc);
        
        // Iterator
        void* iter = trait->begin(const_cast<void*>(instance)); 
        while (trait->valid(iter, instance)) {
            const void* keyPtr = trait->key(iter);
            void* valPtr = trait->value(iter);
            
            // Key must be string-able for JSON Object
            std::string keyStr = ToString(keyPtr, trait->keyType);
            
            yyjson_mut_obj_add_val(ctx.doc, obj, keyStr.c_str(), SerializeValue(ctx, valPtr, trait->valueType));
            
            trait->next(iter);
        }
        trait->destroyIterator(iter);
        return obj;
    }

    static yyjson_mut_val* SerializeObject(JsonWriterContext& ctx, const void* instance, const TypeInfo* info) {
        yyjson_mut_val* obj = yyjson_mut_obj(ctx.doc);
        
        // Helper to traverse hierarchy
        auto processFields = [&](const TypeInfo* currentInfo, auto& self) -> void {
            if (currentInfo->baseType) self(currentInfo->baseType, self);
            
            for (const auto& field : currentInfo->fields) {
                // Check SaveGame flag
                if (!HasFlag(field.flags, PropertyFlags::SaveGame)) continue;

                // Get Field Value Ptr
                size_t align = field.alignment > 0 ? field.alignment : 8;
                alignas(16) char buffer[64];
                
                bool isHeap = (field.size > 64 || align > 16);
                void* storage = nullptr;

                if (isHeap) {
                    shine::co::MemoryScope scope(shine::co::MemoryTag::Reflection);
                    storage = shine::co::Memory::Alloc(field.size, align);
                } else {
                    storage = buffer;
                }
                
                if (!field.isPod) {
                    ConstructValue(storage, field.typeId);
                }

                field.Get(instance, storage);
                
                yyjson_mut_val* fieldVal = nullptr;
                
                // Check Container
                if (field.containerType == ContainerType::Sequence) {
                     const SequenceTrait* seqTrait = static_cast<const SequenceTrait*>(field.containerTrait);
                     fieldVal = SerializeSequence(ctx, storage, seqTrait);
                } 
                else if (field.containerType == ContainerType::Associative) {
                     const MapTrait* mapTrait = static_cast<const MapTrait*>(field.containerTrait);
                     fieldVal = SerializeMap(ctx, storage, mapTrait);
                }
                else {
                    fieldVal = SerializeValue(ctx, storage, field.typeId);
                }
                
                if (fieldVal) {
                    yyjson_mut_obj_add_val(ctx.doc, obj, std::string(field.name).c_str(), fieldVal);
                }

                if (!field.isPod) {
                    DestructValue(storage, field.typeId);
                }

                if (isHeap) {
                    shine::co::MemoryScope scope(shine::co::MemoryTag::Reflection);
                    shine::co::Memory::Free(storage);
                }
            }
        };

        processFields(info, processFields);
        return obj;
    }

    static yyjson_mut_val* SerializeValue(JsonWriterContext& ctx, const void* instance, TypeId typeId) {
        if (IsPrimitive(typeId)) {
            return SerializePrimitive(ctx, instance, typeId);
        }

        const TypeInfo* info = TypeRegistry::Get().Find(typeId);
        if (!info) {
             // Try pointer?
             // For now, assume it's a value type structure.
             return yyjson_mut_null(ctx.doc);
        }

        if (info->isEnum) {
            // Serialize enum as int (simplest)
            // Or string if we look up enumEntries
            // Let's do int for now
             if (info->size == 4) return yyjson_mut_int(ctx.doc, *static_cast<const int*>(instance));
             if (info->size == 8) return yyjson_mut_sint(ctx.doc, *static_cast<const int64_t*>(instance));
             return yyjson_mut_int(ctx.doc, (int)*static_cast<const int*>(instance));
        }

        return SerializeObject(ctx, instance, info);
    }

    std::string Serializer::ToJson(const void* instance, TypeId typeId) {
        yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
        JsonWriterContext ctx{ doc };
        
        yyjson_mut_val* root = SerializeValue(ctx, instance, typeId);
        yyjson_mut_doc_set_root(doc, root);
        
        char* json = yyjson_mut_write(doc, 0, NULL);
        std::string result = json ? json : "{}";
        
        if (json) free(json);
        yyjson_mut_doc_free(doc);
        
        return result;
    }

    // -----------------------------------------------------------------------------
    // Reader Implementation
    // -----------------------------------------------------------------------------

    static void DeserializeValue(yyjson_val* val, void* instance, TypeId typeId);

    static void DeserializePrimitive(yyjson_val* val, void* instance, TypeId typeId) {
        if (typeId == GetTypeId<bool>()) *static_cast<bool*>(instance) = yyjson_get_bool(val);
        else if (typeId == GetTypeId<int>()) *static_cast<int*>(instance) = yyjson_get_int(val);
        else if (typeId == GetTypeId<int64_t>()) *static_cast<int64_t*>(instance) = yyjson_get_sint(val);
        else if (typeId == GetTypeId<uint32_t>()) *static_cast<uint32_t*>(instance) = yyjson_get_uint(val);
        else if (typeId == GetTypeId<uint64_t>()) *static_cast<uint64_t*>(instance) = yyjson_get_uint(val);
        else if (typeId == GetTypeId<float>()) *static_cast<float*>(instance) = (float)yyjson_get_real(val);
        else if (typeId == GetTypeId<double>()) *static_cast<double*>(instance) = yyjson_get_real(val);
        else if (typeId == GetTypeId<std::string>()) {
            const char* str = yyjson_get_str(val);
            if (str) *static_cast<std::string*>(instance) = str;
        }
        // string_view cannot be deserialized into (it's a view), unless we point to some persistent store. 
        // Skip for now.
    }

    static void DeserializeSequence(yyjson_val* arr, void* instance, const SequenceTrait* trait) {
        if (!yyjson_is_arr(arr)) return;
        size_t count = yyjson_arr_size(arr);
        trait->resize(instance, count);
        
        size_t idx, max;
        yyjson_val* item;
        yyjson_arr_foreach(arr, idx, max, item) {
            void* elemPtr = trait->getElement(instance, idx);
            DeserializeValue(item, elemPtr, trait->elementTypeId);
        }
    }

    static void FromString(const char* str, void* outVal, TypeId typeId) {
        if (typeId == GetTypeId<std::string>()) *static_cast<std::string*>(outVal) = str;
        else if (typeId == GetTypeId<int>()) *static_cast<int*>(outVal) = std::atoi(str);
        // ... more conversions
    }

    static void DeserializeMap(yyjson_val* obj, void* instance, const MapTrait* trait) {
        if (!yyjson_is_obj(obj)) return;
        trait->clear(instance);
        
        size_t idx, max;
        yyjson_val* key;
        yyjson_val* val;
        yyjson_obj_foreach(obj, idx, max, key, val) {
            const char* keyStr = yyjson_get_str(key);
            if (!keyStr) continue;

            // Convert key string to KeyType
            // For now, assume Key is std::string or int
            // We need a temp storage for key
            const TypeInfo* keyInfo = TypeRegistry::Get().Find(trait->keyType);
            size_t keySize = keyInfo ? keyInfo->size : sizeof(std::string); 
            // Warning: if keyInfo is null (primitive), we need size.
            // Hack: use a buffer
            char keyBuf[64];
            void* keyPtr = keyBuf;
            
            // Construct and Parse Key
            ConstructValue(keyPtr, trait->keyType);
            FromString(keyStr, keyPtr, trait->keyType);
            
            // Value
             const TypeInfo* valInfo = TypeRegistry::Get().Find(trait->valueType);
             size_t valSize = valInfo ? valInfo->size : 8; // primitive size?
             if (IsPrimitive(trait->valueType)) {
                 if (trait->valueType == GetTypeId<int>()) valSize = 4;
                 if (trait->valueType == GetTypeId<bool>()) valSize = 1;
                 // etc.
             }

             alignas(16) char valBuf[64];
             bool isHeap = (valSize > 64);
             void* valStorage = nullptr;
             
             if (isHeap) {
                 shine::co::MemoryScope scope(shine::co::MemoryTag::Reflection);
                 valStorage = shine::co::Memory::Alloc(valSize);
             } else {
                 valStorage = valBuf;
             }

             ConstructValue(valStorage, trait->valueType);
             
             DeserializeValue(val, valStorage, trait->valueType);
             
             trait->insert(instance, keyPtr, valStorage);
             
             // Cleanup
             DestructValue(keyPtr, trait->keyType);
             DestructValue(valStorage, trait->valueType);
             
             if (isHeap) {
                 shine::co::MemoryScope scope(shine::co::MemoryTag::Reflection);
                 shine::co::Memory::Free(valStorage);
             }
        }
    }

    static void DeserializeObject(yyjson_val* obj, void* instance, const TypeInfo* info) {
        if (!yyjson_is_obj(obj)) return;

        auto processFields = [&](const TypeInfo* currentInfo, auto& self) -> void {
            if (currentInfo->baseType) self(currentInfo->baseType, self);

            for (const auto& field : currentInfo->fields) {
                if (!HasFlag(field.flags, PropertyFlags::SaveGame)) continue;

                yyjson_val* fieldVal = yyjson_obj_get(obj, std::string(field.name).c_str());
                if (!fieldVal) continue;

                // Prepare storage
                size_t align = field.alignment > 0 ? field.alignment : 8;
                alignas(16) char buffer[64];
                
                bool isHeap = (field.size > 64 || align > 16);
                void* storage = nullptr;

                if (isHeap) {
                    shine::co::MemoryScope scope(shine::co::MemoryTag::Reflection);
                    storage = shine::co::Memory::Alloc(field.size, align);
                } else {
                    storage = buffer;
                }

                if (!field.isPod) {
                    ConstructValue(storage, field.typeId);
                }

                // Read current value (important for containers to append, or objects to partial update)
                // Actually, we usually want to overwrite.
                // For containers, we might want to clear and fill?
                // DeserializeSequence calls resize, which might keep old elements if we don't be careful.
                // But usually we resize(N) and overwrite.
                
                // We must Get() the current value to 'storage' so we can modify it.
                field.Get(instance, storage);

                if (field.containerType == ContainerType::Sequence) {
                    DeserializeSequence(fieldVal, storage, static_cast<const SequenceTrait*>(field.containerTrait));
                } else if (field.containerType == ContainerType::Associative) {
                    DeserializeMap(fieldVal, storage, static_cast<const MapTrait*>(field.containerTrait));
                } else {
                    DeserializeValue(fieldVal, storage, field.typeId);
                }

                // Write back
                field.Set(instance, storage);

                if (!field.isPod) {
                    DestructValue(storage, field.typeId);
                }

                if (isHeap) {
                    shine::co::MemoryScope scope(shine::co::MemoryTag::Reflection);
                    shine::co::Memory::Free(storage);
                }
            }
        };
        processFields(info, processFields);
    }

    static void DeserializeValue(yyjson_val* val, void* instance, TypeId typeId) {
        if (IsPrimitive(typeId)) {
            DeserializePrimitive(val, instance, typeId);
            return;
        }
        
        const TypeInfo* info = TypeRegistry::Get().Find(typeId);
        if (!info) return;

        if (info->isEnum) {
            // Enum as int
             if (info->size == 4) *static_cast<int*>(instance) = yyjson_get_int(val);
             return;
        }

        DeserializeObject(val, instance, info);
    }

    bool Serializer::FromJson(const std::string& json, void* instance, TypeId typeId) {
        yyjson_doc* doc = yyjson_read(json.c_str(), json.size(), 0);
        if (!doc) return false;

        yyjson_val* root = yyjson_doc_get_root(doc);
        DeserializeValue(root, instance, typeId);

        yyjson_doc_free(doc);
        return true;
    }

}
