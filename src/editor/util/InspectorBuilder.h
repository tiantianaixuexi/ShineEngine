#pragma once
#include "EngineCore/reflection/Reflection.h"

namespace shine::editor::util {

    class InspectorBuilder {
    public:
        // Draws the entire inspector for an object, handling categories and layout
        static void DrawInspector(void* instance, const reflection::TypeInfo* typeInfo);

        // Helper template for ease of use
        template<typename T>
        static void DrawInspector(T* instance) {
            DrawInspector(instance, reflection::TypeRegistry::Get().Find<T>());
        }
    };

}
