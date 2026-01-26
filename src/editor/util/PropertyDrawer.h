#pragma once
#include "EngineCore/reflection/Reflection.h"

namespace shine::editor::util {

    class PropertyDrawer {
    public:
        // Main entry point to draw a field based on its UI schema and metadata
        static void DrawField(void* instance, const reflection::FieldInfo& field, const reflection::TypeInfo* ownerType = nullptr);

        // Immediate mode helpers (Static access without FieldInfo)
        static bool DrawFloat(const char* label, float& value, float min = 0.0f, float max = 0.0f);
        static bool DrawInt(const char* label, int& value, int min = 0, int max = 0);
        static bool DrawBool(const char* label, bool& value);
        static bool DrawString(const char* label, std::string& value);
    };

}
