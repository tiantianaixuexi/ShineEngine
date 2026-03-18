#pragma once
#include "EngineCore/reflection/Reflection.h"
#include "fmt/base.h"
#include "string/shine_string.h"

#include <iostream>
#include <map>

namespace shine::editor::settings {

enum class GameDifficulty {
    Easy,
    Normal,
    Hard,
    Nightmare
};

// Enum reflection (inside the same namespace)
REFLECT_ENUM(GameDifficulty) {
    builder.Enums({{GameDifficulty::Easy, "简单"},
                   {GameDifficulty::Normal, "普通"},
                   {GameDifficulty::Hard, "困难"},
                   {GameDifficulty::Nightmare, "噩梦"}});
}

struct EngineSettings
{
    EngineSettings() = default;

    float                      masterVolume     = 1.0f;
    int                        resolutionWidth  = 1920;
    int                        resolutionHeight = 1080;
    bool                       fullScreen       = false;
    bool                       vsync            = true;
    shine::SString             rendererType     = "OpenGL";
    float                      shadowDistance   = 50.0f;
    bool                       enableBloom      = true;
    std::map<shine::SString, int> testMapData;

    // Test Function Selection
    shine::SString onGameStart;

    // Enum Test
    GameDifficulty difficulty = GameDifficulty::Normal;

    void PlaySound() {
        std::cout << "Playing Sound!\n";
    }
    void SpawnPlayer() {
        std::cout << "Spawning Player!\n";
        for (auto &c : testMapData) {
            fmt::println("key :{}  , value:{}", c.first, c.second);
        }
    }
    void InternalReset() {}

    void OnVolumeChanged(float oldValue) {
        std::cout << "Master Volume Changed: " << oldValue << " -> " << masterVolume << '\n';
    }

    void OnDifficultyChanged(GameDifficulty oldValue) {
        std::cout << "Difficulty Changed: " << static_cast<int>(oldValue) << " -> " << static_cast<int>(difficulty) << '\n';
    }
};

// Struct reflection (inside the same namespace)
REFLECTION_STRUCT(EngineSettings) {
    using ES = EngineSettings;

    // UI control is auto-deduced from C++ type:
    //   float + Range → Slider,  float → DragFloat
    //   int + Range   → Slider,  int   → DragInt
    //   bool          → Checkbox
    //   SString       → TextInput
    //   enum          → Dropdown (Combo)

    REFLECT_FIELD(masterVolume)
        .Range(0.0f, 100.0f)
        .EditAnywhere()
        .template OnChange<&ES::OnVolumeChanged>()
        .DisplayName("主音量")
        .Meta("Category", shine::STextView::from_literal("Audio"));

    REFLECT_FIELD(resolutionWidth)
        .Range(640.0f, 3840.0f)
        .EditAnywhere()
        .DisplayName("分辨率宽度")
        .Meta("Category", shine::STextView::from_literal("Display"));

    REFLECT_FIELD(resolutionHeight)
        .Range(360.0f, 2160.0f)
        .EditAnywhere()
        .DisplayName("分辨率高度")
        .Meta("Category", shine::STextView::from_literal("Display"));

    REFLECT_FIELD(fullScreen)
        .EditAnywhere()
        .DisplayName("全屏模式")
        .Meta("Category", shine::STextView::from_literal("Display"));

    REFLECT_FIELD(vsync)
        .EditAnywhere()
        .DisplayName("垂直同步")
        .Meta("Category", shine::STextView::from_literal("Display"));

    REFLECT_FIELD(rendererType)
        .EditAnywhere()
        .DisplayName("渲染器类型")
        .Meta("Category", shine::STextView::from_literal("Display"));

    REFLECT_FIELD(shadowDistance)
        .Range(0.0f, 200.0f)
        .EditAnywhere()
        .DisplayName("阴影距离")
        .Meta("Category", shine::STextView::from_literal("Graphics"));

    REFLECT_FIELD(enableBloom)
        .EditAnywhere()
        .DisplayName("开启泛光")
        .Meta("Category", shine::STextView::from_literal("Graphics"));

    // Function Selector Test
    REFLECT_FIELD(onGameStart)
        .EditAnywhere()
        .ScriptReadWrite()
        .FunctionSelect()
        .DisplayName("游戏开始事件")
        .Meta("Category", shine::STextView::from_literal("Events"));

    REFLECT_FIELD(difficulty)
        .EditAnywhere()
        .template OnChange<&ES::OnDifficultyChanged>()
        .DisplayName("游戏难度")
        .Meta("Category", shine::STextView::from_literal("GamePlay"));

    // 方法注册 - 支持链式调用
    REFLECT_METHOD(PlaySound).ScriptCallable();
    REFLECT_METHOD(SpawnPlayer).Meta("BlueprintFunction", true);
    REFLECT_METHOD(InternalReset);
}

} // namespace shine::editor::settings
