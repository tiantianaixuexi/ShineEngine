---
name: shine-reflection
description: "Shine Engine runtime reflection usage guide. Invoke when adding or debugging REFLECTION_STRUCT, REFLECT_ENUM, REFLECT_FIELD, REFLECT_METHOD, TypeRegistry registration, inspector metadata, ScriptView exposure, editor property panels, enum dropdowns, function selectors, or reflected callbacks such as OnChange."
argument-hint: "Describe the reflected type, fields, methods, metadata, or runtime issue you want to implement"
---

# Shine Engine Reflection

Use this skill when working with the runtime reflection system under src/EngineCore/reflection.

This skill covers:
- Declaring reflected enums and structs
- Registering fields and methods
- Adding editor-facing metadata such as category, display name, range, and edit conditions
- Exposing fields and methods to ScriptView
- Understanding what InspectorView and TypeRegistry actually consume
- Avoiding the common mistakes that make a reflected type appear registered but unusable

## Primary Entry Points

- Reflection.h: aggregated public include for reflection users
- ReflectionMacros.h: REFLECT_ENUM, REFLECTION_STRUCT, REFLECT_FIELD, REFLECT_METHOD
- TypeBuilder.h: fluent builder methods that field and method chains resolve to
- TypeRegistry.h: runtime registration and lookup
- Views/InspectorView.h: editor inspection rules
- Views/ScriptView.h: script field access and method invocation rules

## When To Use

Use this skill when you need to:
- add a new reflected config/settings/gameplay struct
- expose properties to an inspector panel
- register an enum for dropdown display
- mark methods as script callable or editor callable
- attach metadata like Category, DisplayName, Min, Max, EditCondition, BlueprintFunction
- diagnose why a reflected field is visible but not editable
- diagnose why ScriptView cannot read or write a field

## Core Rules

1. Include Reflection.h, not individual low-level headers, in normal usage code.
2. Keep the type declaration and its reflection registration in the same namespace.
3. Register enums before registering structs that use them.
4. Use exactly one REFLECTION_STRUCT(Type) block per reflected type.
5. Use REFLECT_FIELD and REFLECT_METHOD only inside the REFLECTION_STRUCT(Type) block.
6. Inspector editing requires EditAnywhere and must not be ReadOnly.
7. ScriptView field access requires ScriptRead and or ScriptWrite flags on the field.
8. ScriptView method calls require ScriptCallable on the method.
9. OnChange callbacks must be either void() or void(OldValueType oldValue).
10. If you add string fields in new project-internal code, use shine::SString for owning storage and shine::STextView for metadata and view-style parameters.

## Minimal Pattern

```cpp
#include "EngineCore/reflection/Reflection.h"
#include "string/shine_string.h"

namespace shine::example {

enum class Mode {
    A,
    B
};

REFLECT_ENUM(Mode) {
    builder.Enums({
        {Mode::A, "Mode A"},
        {Mode::B, "Mode B"},
    });
}

struct ExampleSettings {
    float value = 1.0f;
    Mode mode = Mode::A;
    shine::SString scriptName;

    void OnValueChanged(float oldValue) {}
    void Reset() {}
};

REFLECTION_STRUCT(ExampleSettings) {
    using ES = ExampleSettings;

    REFLECT_FIELD(value)
        .Range(0.0f, 10.0f)
        .EditAnywhere()
        .template OnChange<&ES::OnValueChanged>()
        .DisplayName("Value")
        .Meta("Category", shine::STextView::from_literal("Gameplay"));

    REFLECT_FIELD(mode)
        .EditAnywhere()
        .DisplayName("Mode")
        .Meta("Category", shine::STextView::from_literal("Gameplay"));

    REFLECT_FIELD(scriptName)
        .EditAnywhere()
        .ScriptReadWrite()
        .DisplayName("Script Name")
        .Meta("Category", shine::STextView::from_literal("Gameplay"));

    REFLECT_METHOD(Reset)
        .ScriptCallable();
}

} // namespace shine::example
```

## Recommended Authoring Order

1. Define the enum and or struct.
2. Add helper methods and OnChange callbacks to the type itself.
3. Register enums with REFLECT_ENUM.
4. Register fields with REFLECT_FIELD chains.
5. Register methods with REFLECT_METHOD chains.
6. Verify the intended consumer:
   - InspectorView: EditAnywhere, ReadOnly, Range, Category, DisplayName, EditCondition
   - ScriptView: ScriptRead, ScriptWrite, ScriptReadWrite, ScriptCallable

## Field Registration

REFLECT_FIELD(name) returns a builder-backed field registration chain. The important supported calls are:

- .EditAnywhere(): editable in InspectorView unless also marked ReadOnly
- .ReadOnly(): visible but not editable in InspectorView
- .ScriptRead(): readable from ScriptView
- .ScriptWrite(): writable from ScriptView
- .ScriptReadWrite(): readable and writable from ScriptView
- .Transient(): marks transient data
- .SaveGame(): marks save-game data
- .Range(min, max): stores Min and Max metadata
- .UI(schema): explicit UI schema override
- .FunctionSelect(): marks the field as a function selector UI
- .DisplayName(text): stores display name metadata
- .Meta(key, value): arbitrary metadata, commonly Category, EditCondition, BlueprintFunction
- .template OnChange<&Type::Method>(): callback when the consumer invokes change handling

### Inspector-Focused Metadata

InspectorView currently relies on these conventions:

- Category: grouping label, typically passed as shine::STextView::from_literal(...)
- DisplayName: user-facing field name
- Min and Max: numeric limits used by UI code
- EditCondition: name of another reflected bool field; when false, the field is hidden

Example:

```cpp
REFLECT_FIELD(enableBloom)
    .EditAnywhere()
    .DisplayName("Enable Bloom")
    .Meta("Category", shine::STextView::from_literal("Graphics"));

REFLECT_FIELD(shadowDistance)
    .Range(0.0f, 200.0f)
    .EditAnywhere()
    .DisplayName("Shadow Distance")
    .Meta("Category", shine::STextView::from_literal("Graphics"))
    .Meta("EditCondition", shine::STextView::from_literal("enableBloom"));
```

### Script-Focused Fields

ScriptView::GetField only reads fields flagged with ScriptRead.
ScriptView::SetField only writes fields flagged with ScriptWrite.

If a field needs full script access, prefer:

```cpp
REFLECT_FIELD(onGameStart)
    .EditAnywhere()
    .ScriptReadWrite()
    .FunctionSelect()
    .DisplayName("On Game Start")
    .Meta("Category", shine::STextView::from_literal("Events"));
```

## Method Registration

REFLECT_METHOD(name) returns a method registration chain. Supported calls are:

- .ScriptCallable(): callable through ScriptView
- .EditorCallable(): callable by editor-side tools
- .Meta(key, value): arbitrary metadata such as BlueprintFunction

Example:

```cpp
REFLECT_METHOD(PlaySound)
    .ScriptCallable();

REFLECT_METHOD(SpawnPlayer)
    .EditorCallable()
    .Meta("BlueprintFunction", true);
```

## Enum Registration

Enums are registered through builder.Enums with display labels.

```cpp
enum class GameDifficulty {
    Easy,
    Normal,
    Hard,
    Nightmare
};

REFLECT_ENUM(GameDifficulty) {
    builder.Enums({
        {GameDifficulty::Easy, "简单"},
        {GameDifficulty::Normal, "普通"},
        {GameDifficulty::Hard, "困难"},
        {GameDifficulty::Nightmare, "噩梦"},
    });
}
```

Register the enum before any reflected struct that stores it as a field.

## Complete Example Pattern

This pattern matches the style of src/editor/settings/EngineSettings.h.

```cpp
namespace shine::editor::settings {

enum class GameDifficulty {
    Easy,
    Normal,
    Hard,
    Nightmare
};

REFLECT_ENUM(GameDifficulty) {
    builder.Enums({
        {GameDifficulty::Easy, "简单"},
        {GameDifficulty::Normal, "普通"},
        {GameDifficulty::Hard, "困难"},
        {GameDifficulty::Nightmare, "噩梦"},
    });
}

struct EngineSettings {
    float masterVolume = 1.0f;
    int resolutionWidth = 1920;
    int resolutionHeight = 1080;
    bool fullScreen = false;
    bool vsync = true;
    shine::SString rendererType = "OpenGL";
    float shadowDistance = 50.0f;
    bool enableBloom = true;
    shine::SString onGameStart;
    GameDifficulty difficulty = GameDifficulty::Normal;

    void PlaySound() {}
    void SpawnPlayer() {}
    void InternalReset() {}
    void OnVolumeChanged(float oldValue) {}
    void OnDifficultyChanged(GameDifficulty oldValue) {}
};

REFLECTION_STRUCT(EngineSettings) {
    using ES = EngineSettings;

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

    REFLECT_METHOD(PlaySound)
        .ScriptCallable();

    REFLECT_METHOD(SpawnPlayer)
        .EditorCallable()
        .Meta("BlueprintFunction", true);

    REFLECT_METHOD(InternalReset);
}

} // namespace shine::editor::settings
```

## Runtime Behavior To Remember

- REFLECTION_STRUCT and REFLECT_ENUM both create inline static registration initializers.
- Registration inserts TypeInfo into TypeRegistry at startup.
- Duplicate type IDs cause TypeRegistry::Register to fail.
- InspectorView uses field flags plus metadata to decide visibility and editability.
- ScriptView uses field and method flags, not just registration presence.
- Sequence and associative container traits are auto-detected for std::vector, std::array, std::map, std::unordered_map, std::set, and std::unordered_set.

## Common Mistakes

1. Registering the reflection block outside the type namespace.
2. Forgetting to register an enum that is used by a reflected field.
3. Assuming EditAnywhere also enables script access. It does not.
4. Assuming a registered method is script callable without ScriptCallable. It is not.
5. Writing an OnChange callback with the wrong signature.
6. Using EditCondition with a field name that is not a reflected bool field.
7. Adding string fields in new engine code with std::string or std::string_view instead of shine::SString and shine::STextView.
8. Expecting metadata alone to change behavior when the actual consumer checks flags.

## Implementation Checklist

1. Include Reflection.h.
2. Put enum and struct reflection blocks in the same namespace as the type.
3. Register enums first.
4. Add field flags for the actual consumer you need: inspector, script, or both.
5. Add DisplayName and Category for editor-facing types.
6. Add Range for numeric sliders.
7. Add ScriptCallable for methods invoked through ScriptView.
8. Verify the type can be found through TypeRegistry when debugging runtime issues.

## Debugging Checklist

If reflection looks broken, verify in this order:

1. The header containing the reflected type is compiled into the target.
2. The reflection macro block is in the correct namespace.
3. The type name is unique enough to avoid hash collisions or duplicate registration.
4. The field has the correct flags for the consuming system.
5. The method has ScriptCallable if scripts are expected to invoke it.
6. The enum was registered before a UI or script layer tries to resolve it.
7. The metadata key matches the exact expected string, such as Category or EditCondition.

## Good Prompts For This Skill

- Add reflection for a new editor settings struct with enum dropdowns and grouped categories.
- Expose this component to ScriptView and make two methods script callable.
- Review this REFLECTION_STRUCT block and tell me why the inspector is not editing the field.
- Convert this plain config struct into a reflected type using the EngineSettings pattern.