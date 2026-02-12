#include <chrono>
#include <iostream>
#include <string>
#include <vector>
#include <fmt/format.h>

// Include reflection system
#include "../../src/EngineCore/reflection/Reflection.h"

using namespace shine::reflection;

// ============================================================================
// Performance Test Structures
// ============================================================================

struct SmallStruct {
    int id = 1;
    float value = 1.0f;
};

struct MediumStruct {
    int id = 1;
    std::string name = "medium";
    float x = 1.0f, y = 2.0f, z = 3.0f;
    bool active = true;
};

struct LargeStruct {
    int id = 1;
    std::string name = "large";
    float position[3] = {1.0f, 2.0f, 3.0f};
    float rotation[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    bool flags[8] = {true, false, true, false, true, false, true, false};
    double data[16] = {};
    std::string description = "This is a large structure for performance testing";
};

// Register structures with all available flags
REFLECTION_STRUCT(SmallStruct) {
    REFLECT_FIELD(id).EditAnywhere().ReadOnly().ScriptReadWrite().Transient().SaveGame()
                      .DisplayName("Identifier")
                      .Range(0, 1000)
                      .Meta("Category", "Basic");
    REFLECT_FIELD(value).EditAnywhere().ScriptReadWrite().SaveGame()
                        .DisplayName("Value")
                        .Range(-100.0f, 100.0f);
};

REFLECTION_STRUCT(MediumStruct) {
    REFLECT_FIELD(id).EditAnywhere().ScriptRead().Transient()
                      .DisplayName("ID")
                      .Meta(MetaKeys::Category, "Identity");
    REFLECT_FIELD(name).EditAnywhere().ScriptReadWrite().SaveGame()
                       .DisplayName("Name")
                       .Meta("MaxLength", 64);
    REFLECT_FIELD(x).EditAnywhere().ScriptWrite().SaveGame()
                    .DisplayName("Position X")
                    .Range(-1000.0f, 1000.0f);
    REFLECT_FIELD(y).EditAnywhere().ReadOnly().ScriptRead().Transient()
                    .DisplayName("Position Y")
                    .Range(-1000.0f, 1000.0f);
    REFLECT_FIELD(z).EditAnywhere().ScriptReadWrite().SaveGame()
                    .DisplayName("Position Z")
                    .Range(-1000.0f, 1000.0f);
    REFLECT_FIELD(active).EditAnywhere().ScriptReadWrite().SaveGame()
                         .DisplayName("Is Active")
                         .UI(UI::Checkbox{});
};

REFLECTION_STRUCT(LargeStruct) {
    REFLECT_FIELD(id).EditAnywhere().ReadOnly().ScriptRead().Transient().SaveGame()
                      .DisplayName("Object ID")
                      .Meta("Important", true);
    REFLECT_FIELD(name).EditAnywhere().ScriptReadWrite().SaveGame()
                       .DisplayName("Object Name")
                       .Meta(MetaKeys::DisplayName, "Name Override");
    REFLECT_FIELD(position).EditAnywhere().ScriptReadWrite().SaveGame()
                           .DisplayName("Position")
                           .UI(UI::VectorEditor{3, -10000.0, 10000.0});
    REFLECT_FIELD(rotation).EditAnywhere().ScriptReadWrite().Transient()
                           .DisplayName("Rotation")
                           .UI(UI::VectorEditor{4, -1.0, 1.0});
    REFLECT_FIELD(flags).EditAnywhere().ScriptRead().SaveGame()
                        .DisplayName("Feature Flags");
    REFLECT_FIELD(data).EditAnywhere().ReadOnly().Transient()
                       .DisplayName("Internal Data");
    REFLECT_FIELD(description).EditAnywhere().ScriptReadWrite().SaveGame()
                              .DisplayName("Description")
                              .UI(UI::TextInput{256, true});
};

// ============================================================================
// Benchmark Utilities
// ============================================================================

template<typename Func>
auto benchmark(const std::string& name, Func&& func, int iterations = 100000) {
    // Warmup
    for (int i = 0; i < 1000; ++i) {
        func();
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        func();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    
    double avg_ns = static_cast<double>(duration.count()) / iterations;
    double ops_per_sec = 1e9 / avg_ns;
    
    fmt::print("{:<40} {:>12.2f} ns/op {:>12.0f} ops/sec\n", 
               name, avg_ns, ops_per_sec);
    
    return avg_ns;
}

// ============================================================================
// Performance Tests
// ============================================================================

// Direct access baseline
void test_direct_small() {
    SmallStruct obj;
    volatile auto temp = obj.id;
    (void)temp;
}

void test_direct_medium() {
    MediumStruct obj;
    volatile auto temp = obj.x;
    (void)temp;
}

void test_direct_large() {
    LargeStruct obj;
    volatile auto temp = obj.position[0];
    (void)temp;
}

// Type registry lookup
void test_type_lookup_small() {
    static auto result = TypeRegistry::Get().Find<SmallStruct>();
    const TypeInfo* info = result.has_value() ? result.value() : nullptr;
    volatile auto temp = info;
    (void)temp;
}

void test_type_lookup_medium() {
    static auto result = TypeRegistry::Get().Find<MediumStruct>();
    const TypeInfo* info = result.has_value() ? result.value() : nullptr;
    volatile auto temp = info;
    (void)temp;
}

void test_type_lookup_large() {
    static auto result = TypeRegistry::Get().Find<LargeStruct>();
    const TypeInfo* info = result.has_value() ? result.value() : nullptr;
    volatile auto temp = info;
    (void)temp;
}

// Field access
void test_field_access_small() {
    static auto result = TypeRegistry::Get().Find<SmallStruct>();
    static const TypeInfo* info = result.has_value() ? result.value() : nullptr;
    static const FieldInfo* field = info ? info->FindField("id") : nullptr;
    
    SmallStruct obj;
    int value = 0;
    if (field) {
        field->Get(&obj, &value);
    }
    volatile auto temp = value;
    (void)temp;
}

void test_field_access_medium() {
    static auto result = TypeRegistry::Get().Find<MediumStruct>();
    static const TypeInfo* info = result.has_value() ? result.value() : nullptr;
    static const FieldInfo* field = info ? info->FindField("x") : nullptr;
    
    MediumStruct obj;
    float value = 0.0f;
    if (field) {
        field->Get(&obj, &value);
    }
    volatile auto temp = value;
    (void)temp;
}

void test_field_access_large() {
    static auto result = TypeRegistry::Get().Find<LargeStruct>();
    static const TypeInfo* info = result.has_value() ? result.value() : nullptr;
    static const FieldInfo* field = info ? info->FindField("position") : nullptr;
    
    LargeStruct obj;
    float value[3] = {};
    if (field) {
        field->Get(&obj, &value);
    }
    volatile auto temp = value[0];
    (void)temp;
}

// Multiple field accesses
void test_multi_field_access() {
    static auto result = TypeRegistry::Get().Find<MediumStruct>();
    static const TypeInfo* info = result.has_value() ? result.value() : nullptr;
    
    MediumStruct obj;
    float values[3] = {};
    
    const FieldInfo* x_field = info ? info->FindField("x") : nullptr;
    const FieldInfo* y_field = info ? info->FindField("y") : nullptr;
    const FieldInfo* z_field = info ? info->FindField("z") : nullptr;
    
    if (x_field) x_field->Get(&obj, &values[0]);
    if (y_field) y_field->Get(&obj, &values[1]);
    if (z_field) z_field->Get(&obj, &values[2]);
    
    volatile auto temp = values[0] + values[1] + values[2];
    (void)temp;
}

// Hash computation
void test_compile_time_hash() {
    constexpr auto hash_val = Hash("PerformanceTestString");
    volatile auto temp = hash_val;
    (void)temp;
}

void test_runtime_hash() {
    auto hash_val = Hash(std::string_view("RuntimePerformanceTestString"));
    volatile auto temp = hash_val;
    (void)temp;
}

// ============================================================================
// Main
// ============================================================================

int main() {
    fmt::print("=== ShineEngine Reflection Performance Benchmark ===\n");
    fmt::print("Platform: 64-bit Windows\n");
    fmt::print("Compiler: MSVC C++23\n");
    fmt::print("Iterations: 100,000 per test (with warmup)\n\n");
    
    // Verify registrations
    fmt::print("Verifying type registrations...\n");
    auto small_result = TypeRegistry::Get().Find<SmallStruct>();
    auto medium_result = TypeRegistry::Get().Find<MediumStruct>();
    auto large_result = TypeRegistry::Get().Find<LargeStruct>();
    
    if (!small_result.has_value() || !medium_result.has_value() || !large_result.has_value()) {
        fmt::print("ERROR: Type registration failed!\n");
        return 1;
    }
    
    fmt::print("✓ SmallStruct: {} fields\n", small_result.value()->GetFieldCount());
    fmt::print("✓ MediumStruct: {} fields\n", medium_result.value()->GetFieldCount());
    fmt::print("✓ LargeStruct: {} fields\n\n", large_result.value()->GetFieldCount());
    
    fmt::print("{:<40} {:>15} {:>15}\n", "Test", "Time per op", "Operations/sec");
    fmt::print("{}\n", std::string(70, '-'));
    
    // Run benchmarks
    try {
        // Baseline direct access
        fmt::print("\n--- Direct Access Baseline ---\n");
        double direct_small = benchmark("Direct SmallStruct access", test_direct_small);
        double direct_medium = benchmark("Direct MediumStruct access", test_direct_medium);
        double direct_large = benchmark("Direct LargeStruct access", test_direct_large);
        
        // Type lookup
        fmt::print("\n--- Type Registry Lookup ---\n");
        double lookup_small = benchmark("SmallStruct type lookup", test_type_lookup_small);
        double lookup_medium = benchmark("MediumStruct type lookup", test_type_lookup_medium);
        double lookup_large = benchmark("LargeStruct type lookup", test_type_lookup_large);
        
        // Field access
        fmt::print("\n--- Single Field Access ---\n");
        double field_small = benchmark("SmallStruct field access", test_field_access_small);
        double field_medium = benchmark("MediumStruct field access", test_field_access_medium);
        double field_large = benchmark("LargeStruct field access", test_field_access_large);
        
        // Multi-field access
        fmt::print("\n--- Multiple Field Access ---\n");
        double multi_field = benchmark("Multi-field access (3 fields)", test_multi_field_access);
        
        // Hash computation
        fmt::print("\n--- Hash Computation ---\n");
        double compile_hash = benchmark("Compile-time hash", test_compile_time_hash);
        double runtime_hash = benchmark("Runtime hash", test_runtime_hash);
        
        // Summary
        fmt::print("\n{}\n", std::string(70, '='));
        fmt::print("PERFORMANCE SUMMARY:\n");
        fmt::print("====================\n\n");
        
        fmt::print("Overhead Ratios (compared to direct access):\n");
        fmt::print("  SmallStruct type lookup: {:.2f}x\n", lookup_small / (direct_small > 0 ? direct_small : 1));
        fmt::print("  MediumStruct type lookup: {:.2f}x\n", lookup_medium / (direct_medium > 0 ? direct_medium : 1));
        fmt::print("  LargeStruct type lookup: {:.2f}x\n", lookup_large / (direct_large > 0 ? direct_large : 1));
        fmt::print("\n");
        fmt::print("  SmallStruct field access: {:.2f}x\n", field_small / (direct_small > 0 ? direct_small : 1));
        fmt::print("  MediumStruct field access: {:.2f}x\n", field_medium / (direct_medium > 0 ? direct_medium : 1));
        fmt::print("  LargeStruct field access: {:.2f}x\n", field_large / (direct_large > 0 ? direct_large : 1));
        fmt::print("\n");
        fmt::print("  Multi-field access: {:.2f}x\n", multi_field / (direct_medium > 0 ? direct_medium : 1));
        fmt::print("\n");
        fmt::print("  Hash computation ratio (runtime/compile): {:.2f}x\n", 
                   runtime_hash / (compile_hash > 0 ? compile_hash : 1));
        
        fmt::print("\nPerformance Analysis:\n");
        if (field_small / direct_small < 5.0) {
            fmt::print("✅ Small struct reflection performance is excellent (<5x overhead)\n");
        } else if (field_small / direct_small < 10.0) {
            fmt::print("⚠️  Small struct reflection performance is acceptable (5-10x overhead)\n");
        } else {
            fmt::print("❌ Small struct reflection performance needs optimization (>10x overhead)\n");
        }
        
        fmt::print("\nBenchmark completed successfully!\n");
        return 0;
        
    } catch (const std::exception& e) {
        fmt::print("Exception occurred: {}\n", e.what());
        return 1;
    } catch (...) {
        fmt::print("Unknown exception occurred\n");
        return 1;
    }
}