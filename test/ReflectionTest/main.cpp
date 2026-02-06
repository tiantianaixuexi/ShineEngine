#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <chrono>
#include <fmt/format.h>

// Single include for the whole reflection system
#include "../../src/EngineCore/reflection/Reflection.h"

// ============================================================================
// Test types
// ============================================================================

enum class Colour { Red, Green, Blue };

REFLECT_ENUM(Colour) {
    builder.Enums({{Colour::Red, "Red"}, {Colour::Green, "Green"}, {Colour::Blue, "Blue"}});
}

struct TestStruct {
    int         id          = 42;
    std::string name        = "test";
    float       health      = 100.0f;
    bool        active      = true;
    Colour      colour      = Colour::Green;

    void Greet()       { std::cout << "Hello from " << name << "!\n"; }
    void ResetHealth() { health = 100.0f; }

    void OnHealthChanged(float oldVal) {
        fmt::println("Health changed: {} -> {}", oldVal, health);
    }
};

REFLECTION_STRUCT(TestStruct) {
    using TS = TestStruct;

    REFLECT_FIELD(id)
        .EditAnywhere()
        .Range(0.0f, 9999.0f)
        .DisplayName("ID");

    REFLECT_FIELD(name)
        .EditAnywhere()
        .UI(shine::reflection::UI::TextInput{})
        .DisplayName("Name");

    REFLECT_FIELD(health)
        .EditAnywhere()
        .Range(0.0f, 100.0f)
        .UI(shine::reflection::UI::Slider{})
        .OnChange<&TS::OnHealthChanged>()
        .DisplayName("Health")
        .Meta("Category", std::string_view{"Stats"});

    REFLECT_FIELD(active)
        .EditAnywhere()
        .UI(shine::reflection::UI::Checkbox{})
        .DisplayName("Active")
        .Meta("Category", std::string_view{"Stats"});

    REFLECT_FIELD(colour)
        .EditAnywhere()
        .DisplayName("Colour")
        .Meta("Category", std::string_view{"Appearance"});

    REFLECT_METHOD(Greet).ScriptCallable();
    REFLECT_METHOD(ResetHealth).EditorCallable();
}

REFLECTION_REGISTER(TestStruct)

// ============================================================================
// Main
// ============================================================================

int main() {
    using namespace shine::reflection;

    fmt::println("========================================");
    fmt::println("  ShineEngine Reflection Test Suite");
    fmt::println("========================================\n");

    // --- Type lookup ---------------------------------------------------------
    const TypeInfo* info = TypeRegistry::Get().Find<TestStruct>();
    if (!info) {
        fmt::println("ERROR: TestStruct not registered!");
        return 1;
    }

    fmt::println("[TypeInfo]");
    fmt::println("  Name      : {}", info->name);
    fmt::println("  Size      : {} bytes", info->size);
    fmt::println("  Alignment : {}", info->alignment);
    fmt::println("  isPod     : {}", info->isPod);
    fmt::println("  Fields    : {}", info->GetFieldCount());
    fmt::println("  Methods   : {}\n", info->GetMethodCount());

    // --- Field access --------------------------------------------------------
    TestStruct obj;

    fmt::println("[Field Access]");
    for (const auto& f : info->fields) {
        fmt::print("  {} (typeId={:#010x})", f.name, f.typeId);

        if (f.typeId == GetTypeId<int>()) {
            int v; f.Get(&obj, &v);
            fmt::print(" = {}", v);
        } else if (f.typeId == GetTypeId<float>()) {
            float v; f.Get(&obj, &v);
            fmt::print(" = {:.1f}", v);
        } else if (f.typeId == GetTypeId<bool>()) {
            bool v; f.Get(&obj, &v);
            fmt::print(" = {}", v);
        } else if (f.typeId == GetTypeId<std::string>()) {
            std::string v; f.Get(&obj, &v);
            fmt::print(" = \"{}\"", v);
        }
        fmt::println("");
    }
    fmt::println("");

    // --- OnChange callback ---------------------------------------------------
    fmt::println("[OnChange]");
    const FieldInfo* hpField = info->FindField("health");
    if (hpField) {
        float newHp = 42.0f;
        float oldHp; hpField->Get(&obj, &oldHp);
        hpField->Set(&obj, &newHp);
        hpField->OnChange(&obj, &oldHp);
    }
    fmt::println("");

    // --- Enum reflection -----------------------------------------------------
    fmt::println("[Enum]");
    const TypeInfo* colourInfo = TypeRegistry::Get().Find<Colour>();
    if (colourInfo && colourInfo->isEnum) {
        for (const auto& e : colourInfo->enumEntries)
            fmt::println("  {} = {}", e.name, e.value);
    }
    fmt::println("");

    // --- Method invocation ---------------------------------------------------
    fmt::println("[Methods]");
    for (const auto& m : info->methods) {
        fmt::print("  {} ", m.name);
        if (HasFlag(m.flags, FunctionFlags::ScriptCallable)) fmt::print("[ScriptCallable] ");
        if (HasFlag(m.flags, FunctionFlags::EditorCallable)) fmt::print("[EditorCallable] ");
        fmt::println("");
    }

    const MethodInfo* greet = info->FindMethod("Greet");
    if (greet) {
        fmt::print("  Invoking Greet(): ");
        greet->Invoke(&obj, nullptr, nullptr);
    }
    fmt::println("");

    // --- InspectorView -------------------------------------------------------
    fmt::println("[InspectorView]");
    InspectorView view;
    view.typeInfo = info;
    for (const auto& f : view) {
        auto cat = view.GetCategory(f);
        fmt::println("  {} | editable={} | category={}",
                     f.name, view.IsEditable(f), cat.empty() ? "(none)" : cat);
    }
    fmt::println("");

    // --- Performance ---------------------------------------------------------
    fmt::println("[Performance]");
    constexpr int N = 1'000'000;

    auto t0 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < N; ++i) { volatile auto t = obj.id; (void)t; }
    auto t1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < N; ++i) { int v; hpField->Get(&obj, &v); (void)v; }
    auto t2 = std::chrono::high_resolution_clock::now();

    auto direct  = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
    auto reflect = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
    fmt::println("  Direct  : {} ns ({:.2f} ns/op)", direct,  (double)direct  / N);
    fmt::println("  Reflect : {} ns ({:.2f} ns/op)", reflect, (double)reflect / N);
    fmt::println("  Ratio   : {:.2f}x\n", (double)reflect / (direct ? direct : 1));

    fmt::println("========================================");
    fmt::println("  All tests passed!");
    fmt::println("========================================");

    return 0;
}
