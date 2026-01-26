#pragma once

#include "Reflection.h"
#include <string>

namespace shine::reflection {

    class Serializer {
    public:
        // -----------------------------------------------------------------------------
        // JSON Serialization
        // -----------------------------------------------------------------------------
        
        // Serialize instance to JSON string
        static std::string ToJson(const void* instance, TypeId typeId);

        // Deserialize JSON string to instance
        // Returns true on success
        static bool FromJson(const std::string& json, void* instance, TypeId typeId);

        // -----------------------------------------------------------------------------
        // Template Helpers
        // -----------------------------------------------------------------------------
        
        template<typename T>
        static std::string ToJson(const T& obj) { 
            return ToJson(&obj, GetTypeId<T>()); 
        }
        
        template<typename T>
        static bool FromJson(const std::string& json, T& obj) { 
            return FromJson(json, &obj, GetTypeId<T>()); 
        }
    };

}
