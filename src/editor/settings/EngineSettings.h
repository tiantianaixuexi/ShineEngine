#pragma once
#include "EngineCore/reflection/Reflection.h"
#include <string>
#include <string>
#include <iostream>

namespace shine::editor::settings {

    enum class GameDifficulty {
        Easy,
        Normal,
        Hard,
        Nightmare
    };

    // Auto-register Enum (Moved up for visibility)
    REFLECT_ENUM(GameDifficulty) {
        builder.Enums({
            {GameDifficulty::Easy, "简单"},
            {GameDifficulty::Normal, "普通"},
            {GameDifficulty::Hard, "困难"},
            {GameDifficulty::Nightmare, "噩梦"}
        });
    }

    struct EngineSettings {
        // Constructor to force linking of reflection data
        EngineSettings() {
            // Force reference to ensure static initializers run
            (void)GameDifficulty_Reg;
        }

        float masterVolume = 1.0f;
        int resolutionWidth = 1920;
        int resolutionHeight = 1080;
        bool fullScreen = false;
        bool vsync = true;
        std::string rendererType = "OpenGL";
        float shadowDistance = 50.0f;
        bool enableBloom = true;
        
        // Test Function Selection
        std::string onGameStart = "";

        // Enum Test
        GameDifficulty difficulty = GameDifficulty::Normal;

        void PlaySound() {
            std::cout << "Playing Sound!" << std::endl;
        }
        void SpawnPlayer() {
            std::cout << "Spawning Player!" << std::endl;
        }
        void InternalReset() {}

        void OnVolumeChanged(float oldValue) {
            std::cout << "Master Volume Changed: " << oldValue << " -> " << masterVolume << std::endl;
        }

        void OnDifficultyChanged(GameDifficulty oldValue) {
            std::cout << "Difficulty Changed: " << (int)oldValue << " -> " << (int)difficulty << std::endl;
        }

        // Modern Header-Only Reflection
        REFLECT_STRUCT(EngineSettings) {
            REFLECT_FIELD(masterVolume)
                .Range(0.0f, 100.0f)
                .UI(Shine::Reflection::UI::Slider{}) 
                .EditAnywhere()
                .OnChange<&EngineSettings::OnVolumeChanged>()
                .DisplayName("主音量")
                .Meta("Category", "Audio");

            REFLECT_FIELD(resolutionWidth)
                .Range(640.0f, 3840.0f)
                .UI(Shine::Reflection::UI::Slider{})
                .EditAnywhere()
                .DisplayName("分辨率宽度")
                .Meta("Category", "Display");

            REFLECT_FIELD(resolutionHeight)
                .Range(360.0f, 2160.0f)
                .UI(Shine::Reflection::UI::Slider{})
                .EditAnywhere()
                .DisplayName("分辨率高度")
                .Meta("Category", "Display");

            REFLECT_FIELD(fullScreen)
                .UI(Shine::Reflection::UI::Checkbox{})
                .EditAnywhere()
                .DisplayName("全屏模式")
                .Meta("Category", "Display");

            REFLECT_FIELD(vsync)
                .UI(Shine::Reflection::UI::Checkbox{})
                .EditAnywhere()
                .DisplayName("垂直同步")
                .Meta("Category", "Display");

            REFLECT_FIELD(rendererType)
                .UI(Shine::Reflection::UI::InputText{})
                .EditAnywhere()
                .DisplayName("渲染器类型")
                .Meta("Category", "Display");

            REFLECT_FIELD(shadowDistance)
                .Range(0.0f, 200.0f)
                .UI(Shine::Reflection::UI::Slider{})
                .EditAnywhere()
                .DisplayName("阴影距离")
                .Meta("Category", "Graphics");

            REFLECT_FIELD(enableBloom)
                .UI(Shine::Reflection::UI::Checkbox{})
                .EditAnywhere()
                .DisplayName("开启泛光")
                .Meta("Category", "Graphics");

            // Function Selector Test
            REFLECT_FIELD(onGameStart)
                .EditAnywhere()
                .FunctionSelect()
                .DisplayName("游戏开始事件")
                .Meta("Category", "Events");

            REFLECT_FIELD(difficulty)
                .EditAnywhere()
                .OnChange<&EngineSettings::OnDifficultyChanged>()
                .DisplayName("游戏难度")
                .Meta("Category", "GamePlay");

            REFLECT_METHOD(PlaySound).ScriptCallable();
            REFLECT_METHOD(SpawnPlayer).Meta("BlueprintFunction", true);
            REFLECT_METHOD(InternalReset); // Should not appear
        }
    };

    REFLECTION_REGISTER(EngineSettings)
}
