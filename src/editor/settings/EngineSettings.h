#pragma once
#include "EngineCore/reflection/Reflection.h"
#include <string>

namespace shine::editor::settings {

    struct EngineSettings {
        float masterVolume = 1.0f;
        int resolutionWidth = 1920;
        int resolutionHeight = 1080;
        bool fullScreen = false;
        bool vsync = true;
        std::string rendererType = "OpenGL";
        float shadowDistance = 50.0f;
        bool enableBloom = true;

        // Modern Header-Only Reflection
        REFLECT_STRUCT(EngineSettings) {
            REFLECT_FIELD(masterVolume)
                .Range(0.0f, 100.0f)
                .UI(Shine::Reflection::UI::Slider{}) 
                .EditAnywhere()
                .Meta("Category", "Audio");

            REFLECT_FIELD(resolutionWidth)
                .Range(640.0f, 3840.0f)
                .UI(Shine::Reflection::UI::Slider{})
                .EditAnywhere()
                .Meta("Category", "Display");

            REFLECT_FIELD(resolutionHeight)
                .Range(360.0f, 2160.0f)
                .UI(Shine::Reflection::UI::Slider{})
                .EditAnywhere()
                .Meta("Category", "Display");

            REFLECT_FIELD(fullScreen)
                .UI(Shine::Reflection::UI::Checkbox{})
                .EditAnywhere()
                .Meta("Category", "Display");

            REFLECT_FIELD(vsync)
                .UI(Shine::Reflection::UI::Checkbox{})
                .EditAnywhere()
                .Meta("Category", "Display");

            REFLECT_FIELD(rendererType)
                .UI(Shine::Reflection::UI::InputText{})
                .EditAnywhere()
                .Meta("Category", "Display");

            REFLECT_FIELD(shadowDistance)
                .Range(0.0f, 200.0f)
                .UI(Shine::Reflection::UI::Slider{})
                .EditAnywhere()
                .Meta("Category", "Graphics");

            REFLECT_FIELD(enableBloom)
                .UI(Shine::Reflection::UI::Checkbox{})
                .EditAnywhere()
                .Meta("Category", "Graphics");
        }
    };

    // Auto-register at startup
    REFLECTION_REGISTER(EngineSettings)
}
