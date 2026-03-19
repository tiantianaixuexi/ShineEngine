// =============================================================================
// ReflectionPerfTest.cpp — 反射性能专项测试
// =============================================================================

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "../common/reflection_test_fixture.h"
#include "EngineCore/reflection/Reflection.h"
#include "../common/test_benchmark_framework.h"

#include <fmt/format.h>

struct PerfReport {
    double fieldLookup_ns = 0.0;
    double fieldLookup_ops = 0.0;
    double fieldGetLookup_ns = 0.0;
    double fieldGetCached_ns = 0.0;
    double fieldGetOffset_ns = 0.0;
    double fieldGetBound_ns = 0.0;
    double fieldSetLookup_ns = 0.0;
    double fieldSetCached_ns = 0.0;
    double fieldSetOffset_ns = 0.0;
    double fieldSetBound_ns = 0.0;
    double nestedGetBound_ns = 0.0;
    double nestedGetOffset_ns = 0.0;
    double nestedGetCt_ns = 0.0;
    double nestedSetBound_ns = 0.0;
    double nestedSetOffset_ns = 0.0;
    double nestedSetCt_ns = 0.0;
    double nestedMethodNative_ns = 0.0;
    double nestedMethodBound_ns = 0.0;
    double nestedMethodReflect_ns = 0.0;
    double nestedMethodLookupReflect_ns = 0.0;
    double nativeRead_ns = 0.0;
    double nativeWrite_ns = 0.0;
    double methodLookupInvoke_ns = 0.0;
    double methodCachedInvoke_ns = 0.0;
    double methodNative_ns = 0.0;
    double typeFindSlow_ns = 0.0;
    double typeFindFast_ns = 0.0;
    double findSpeedup_x = 0.0;

    void Print() const {
        fmt::print("\n============================================================\n");
        fmt::print("                    反射性能专项报告\n");
        fmt::print("============================================================\n");
        fmt::print("  字段名查找: {:.2f} ns ({:.2f} M ops/sec)\n", fieldLookup_ns, fieldLookup_ops / 1000000.0);
        fmt::print("  字段读取: native {:.2f} / lookup+get {:.2f} / cached {:.2f} / offset {:.2f} / bound {:.2f} ns\n",
            nativeRead_ns, fieldGetLookup_ns, fieldGetCached_ns, fieldGetOffset_ns, fieldGetBound_ns);
        fmt::print("  字段写入: native {:.2f} / lookup+set {:.2f} / cached {:.2f} / offset {:.2f} / bound {:.2f} ns\n",
            nativeWrite_ns, fieldSetLookup_ns, fieldSetCached_ns, fieldSetOffset_ns, fieldSetBound_ns);
        fmt::print("  嵌套路径: get bound {:.2f} / get offset {:.2f} / get CT {:.2f} / set bound {:.2f} / set offset {:.2f} / set CT {:.2f} ns\n",
            nestedGetBound_ns, nestedGetOffset_ns, nestedGetCt_ns, nestedSetBound_ns, nestedSetOffset_ns, nestedSetCt_ns);
        fmt::print("  路径方法: native {:.2f} / bound {:.2f} / cached reflect {:.2f} / lookup reflect {:.2f} ns\n",
            nestedMethodNative_ns, nestedMethodBound_ns, nestedMethodReflect_ns, nestedMethodLookupReflect_ns);
        fmt::print("  方法调用: native {:.2f} / cached {:.2f} / lookup+invoke {:.2f} ns\n",
            methodNative_ns, methodCachedInvoke_ns, methodLookupInvoke_ns);
        fmt::print("  类型查找: {:.1f}x 加速 (FindFast)\n", findSpeedup_x);
        fmt::print("============================================================\n\n");
    }
};

PerfReport g_perfReport;

template<typename Func>
shine::test::BenchmarkStats MeasureBenchmark(Func&& func, int iterations = 1000, int rounds = 32) {
    return shine::test::measure_benchmark([&]() {
        if constexpr (std::is_void_v<std::invoke_result_t<Func&>>) {
            func();
        } else {
            decltype(auto) value = func();
            shine::test::DoNotOptimize(value);
        }
        shine::test::ClobberMemory();
    }, rounds, iterations);
}

template<typename Func>
double BenchmarkNs(Func&& func, int iterations = 1000, int rounds = 32) {
    return MeasureBenchmark(std::forward<Func>(func), iterations, rounds).mean_ns;
}

double SafeRatio(double lhs, double rhs) {
    return rhs > 0.0 ? lhs / rhs : 0.0;
}

void PrintBenchmarkLine(const char* label, double ns, double baseline_ns = 0.0) {
    fmt::print("  - {:<24} {:>10.2f} ns", label, ns);
    if (baseline_ns > 0.0) {
        fmt::print(" ({:.2f}x)", ns / baseline_ns);
    }
    fmt::print("\n");
}

void PrintSeparator(const char* title) {
    fmt::print("\n============================================================\n");
    fmt::print("  {}\n", title);
    fmt::print("============================================================\n");
}

void TestBoundPathConvenience() {
    PrintSeparator("性能专项: 链式字段路径包装");

    using PositionXPath = shine::reflection::BoundPath<&Transform::position, &Vec3::x>;
    using PositionLengthSq = shine::reflection::BoundMethodPath<&Vec3::LengthSquared, &Transform::position>;
    using PositionSetX = shine::reflection::BoundMethodPath<&Vec3::SetX, &Transform::position>;

    Transform t;
    PositionXPath::Set(t, 12.5f);
    PositionSetX::Invoke(t, 15.0f);

    fmt::print("  - path: {}\n", PositionXPath::Path());
    fmt::print("  - leaf: {}\n", PositionXPath::LeafName());
    fmt::print("  - depth: {}\n", PositionXPath::Depth);
    fmt::print("  - offset: {}\n", PositionXPath::Offset());
    fmt::print("  - value after Set/Get: {}\n", PositionXPath::Get(t));
    fmt::print("  - method path: {}\n", PositionLengthSq::Path());
    fmt::print("  - method invoke: {}\n", PositionLengthSq::Invoke(t));
}

void TestBoundMethodPathPerformance() {
    PrintSeparator("性能专项: 路径方法调用");

    using PositionLengthSq = shine::reflection::BoundMethodPath<&Vec3::LengthSquared, &Transform::position>;

    Transform t;
    t.position = Vec3(3.0f, 4.0f, 5.0f);

    auto* vecType = shine::reflection::TypeRegistry::Get().FindFast(shine::reflection::GetTypeId<Vec3>());
    if (vecType == nullptr) {
        fmt::print("错误: 无法获取 Vec3 类型信息\n");
        return;
    }

    auto* lengthMethod = vecType->FindMethodFast("LengthSquared");
    if (lengthMethod == nullptr) {
        fmt::print("错误: 无法获取 Vec3::LengthSquared 方法信息\n");
        return;
    }

    const auto nativeStats = MeasureBenchmark([&]() {
        return t.position.LengthSquared();
    }, 20000);
    const auto boundStats = MeasureBenchmark([&]() {
        return PositionLengthSq::Invoke(t);
    }, 20000);
    const auto reflectCachedStats = MeasureBenchmark([&]() {
        float ret = 0.0f;
        lengthMethod->Invoke(&t.position, nullptr, &ret);
        return ret;
    }, 12000);
    const auto reflectLookupStats = MeasureBenchmark([&]() {
        float ret = 0.0f;
        auto* method = vecType->FindMethodFast("LengthSquared");
        method->Invoke(&t.position, nullptr, &ret);
        return ret;
    }, 12000);

    PrintBenchmarkLine("native nested call", nativeStats.mean_ns);
    PrintBenchmarkLine("BoundMethodPath", boundStats.mean_ns, nativeStats.mean_ns);
    PrintBenchmarkLine("cached reflect", reflectCachedStats.mean_ns, nativeStats.mean_ns);
    PrintBenchmarkLine("lookup reflect", reflectLookupStats.mean_ns, nativeStats.mean_ns);

    g_perfReport.nestedMethodNative_ns = nativeStats.mean_ns;
    g_perfReport.nestedMethodBound_ns = boundStats.mean_ns;
    g_perfReport.nestedMethodReflect_ns = reflectCachedStats.mean_ns;
    g_perfReport.nestedMethodLookupReflect_ns = reflectLookupStats.mean_ns;
}

void TestFieldAccessPerformance() {
    PrintSeparator("性能专项: 字段访问");

    using PositionBinding = shine::reflection::BoundMember<&Transform::position>;
    using PositionXPath = shine::reflection::BoundPath<&Transform::position, &Vec3::x>;

    Transform t;
    t.position = Vec3(1.0f, 2.0f, 3.0f);

    auto* typeInfo = shine::reflection::TypeRegistry::Get().FindFast(shine::reflection::GetTypeId<Transform>());
    if (!typeInfo) {
        fmt::print("错误: 无法获取 Transform 类型信息\n");
        return;
    }

    const auto* positionField = typeInfo->FindFieldFast(PositionBinding::Name());
    if (positionField == nullptr) {
        fmt::print("错误: 无法获取 position 字段信息\n");
        return;
    }

    auto* offsetPtr = reinterpret_cast<Vec3*>(reinterpret_cast<char*>(&t) + positionField->offset);
    auto* nestedOffsetPtr = reinterpret_cast<float*>(reinterpret_cast<char*>(&t) + PositionXPath::Offset());
    Vec3 readBuffer{};
    Vec3 writeBuffer{4.0f, 5.0f, 6.0f};
    float scalarWrite = 1.0f;

    for (int i = 0; i < 1024; ++i) {
        (void)typeInfo->FindFieldFast(PositionBinding::Name());
        positionField->Get(&t, &readBuffer);
        shine::test::DoNotOptimize(PositionXPath::Get(t));
    }

    const auto lookupStats = MeasureBenchmark([&]() {
        return typeInfo->FindFieldFast(PositionBinding::Name());
    }, 20000);
    const auto nativeReadStats = MeasureBenchmark([&]() {
        return t.position;
    }, 20000);
    const auto reflectLookupReadStats = MeasureBenchmark([&]() {
        auto* field = typeInfo->FindFieldFast(PositionBinding::Name());
        field->Get(&t, &readBuffer);
    }, 12000);
    const auto reflectCachedReadStats = MeasureBenchmark([&]() {
        positionField->Get(&t, &readBuffer);
    }, 20000);
    const auto offsetReadStats = MeasureBenchmark([&]() {
        return *offsetPtr;
    }, 20000);
    const auto boundReadStats = MeasureBenchmark([&]() {
        return PositionBinding::Get(t);
    }, 20000);

    const auto nativeWriteStats = MeasureBenchmark([&]() {
        scalarWrite += 0.125f;
        t.position = Vec3(scalarWrite, scalarWrite + 1.0f, scalarWrite + 2.0f);
    }, 12000);
    const auto reflectLookupWriteStats = MeasureBenchmark([&]() {
        writeBuffer.x += 0.25f;
        auto* field = typeInfo->FindFieldFast(PositionBinding::Name());
        field->Set(&t, &writeBuffer);
    }, 12000);
    const auto reflectCachedWriteStats = MeasureBenchmark([&]() {
        writeBuffer.y += 0.125f;
        positionField->Set(&t, &writeBuffer);
    }, 12000);
    const auto offsetWriteStats = MeasureBenchmark([&]() {
        writeBuffer.z += 0.0625f;
        *offsetPtr = writeBuffer;
    }, 12000);
    const auto boundWriteStats = MeasureBenchmark([&]() {
        writeBuffer.x += 0.03125f;
        PositionBinding::Set(t, writeBuffer);
    }, 12000);

    const auto nestedBoundGetStats = MeasureBenchmark([&]() {
        return PositionXPath::Get(t);
    }, 20000);
    const auto nestedOffsetGetStats = MeasureBenchmark([&]() {
        return *nestedOffsetPtr;
    }, 20000);
    const auto nestedCtGetStats = MeasureBenchmark([&]() {
        return shine::reflection::CT_GET<&Vec3::x>(t.position);
    }, 20000);
    const auto nestedBoundSetStats = MeasureBenchmark([&]() {
        scalarWrite += 0.015625f;
        PositionXPath::Set(t, scalarWrite);
    }, 12000);
    const auto nestedOffsetSetStats = MeasureBenchmark([&]() {
        scalarWrite += 0.015625f;
        *nestedOffsetPtr = scalarWrite;
    }, 12000);
    const auto nestedCtSetStats = MeasureBenchmark([&]() {
        scalarWrite += 0.015625f;
        shine::reflection::CT_SET<&Vec3::x>(t.position, scalarWrite);
    }, 12000);

    fmt::print("专项基准: 字段读取路径 (position)\n");
    PrintBenchmarkLine("name lookup", lookupStats.mean_ns);
    PrintBenchmarkLine("native read", nativeReadStats.mean_ns);
    PrintBenchmarkLine("lookup + Get", reflectLookupReadStats.mean_ns, nativeReadStats.mean_ns);
    PrintBenchmarkLine("cached Get", reflectCachedReadStats.mean_ns, nativeReadStats.mean_ns);
    PrintBenchmarkLine("offset read", offsetReadStats.mean_ns, nativeReadStats.mean_ns);
    PrintBenchmarkLine("BoundMember::Get", boundReadStats.mean_ns, nativeReadStats.mean_ns);

    fmt::print("专项基准: 字段写入路径 (position)\n");
    PrintBenchmarkLine("native write", nativeWriteStats.mean_ns);
    PrintBenchmarkLine("lookup + Set", reflectLookupWriteStats.mean_ns, nativeWriteStats.mean_ns);
    PrintBenchmarkLine("cached Set", reflectCachedWriteStats.mean_ns, nativeWriteStats.mean_ns);
    PrintBenchmarkLine("offset write", offsetWriteStats.mean_ns, nativeWriteStats.mean_ns);
    PrintBenchmarkLine("BoundMember::Set", boundWriteStats.mean_ns, nativeWriteStats.mean_ns);

    fmt::print("专项基准: 链式路径 (position.x) path={}\n", PositionXPath::Path());
    PrintBenchmarkLine("BoundPath::Get", nestedBoundGetStats.mean_ns);
    PrintBenchmarkLine("offset leaf read", nestedOffsetGetStats.mean_ns);
    PrintBenchmarkLine("CT_GET<&Vec3::x>", nestedCtGetStats.mean_ns);
    PrintBenchmarkLine("BoundPath::Set", nestedBoundSetStats.mean_ns);
    PrintBenchmarkLine("offset leaf write", nestedOffsetSetStats.mean_ns);
    PrintBenchmarkLine("CT_SET<&Vec3::x>", nestedCtSetStats.mean_ns);

    g_perfReport.fieldLookup_ns = lookupStats.mean_ns;
    g_perfReport.fieldLookup_ops = lookupStats.mean_ns > 0.0 ? 1000000000.0 / lookupStats.mean_ns : 0.0;
    g_perfReport.fieldGetLookup_ns = reflectLookupReadStats.mean_ns;
    g_perfReport.fieldGetCached_ns = reflectCachedReadStats.mean_ns;
    g_perfReport.fieldGetOffset_ns = offsetReadStats.mean_ns;
    g_perfReport.fieldGetBound_ns = boundReadStats.mean_ns;
    g_perfReport.fieldSetLookup_ns = reflectLookupWriteStats.mean_ns;
    g_perfReport.fieldSetCached_ns = reflectCachedWriteStats.mean_ns;
    g_perfReport.fieldSetOffset_ns = offsetWriteStats.mean_ns;
    g_perfReport.fieldSetBound_ns = boundWriteStats.mean_ns;
    g_perfReport.nestedGetBound_ns = nestedBoundGetStats.mean_ns;
    g_perfReport.nestedGetOffset_ns = nestedOffsetGetStats.mean_ns;
    g_perfReport.nestedGetCt_ns = nestedCtGetStats.mean_ns;
    g_perfReport.nestedSetBound_ns = nestedBoundSetStats.mean_ns;
    g_perfReport.nestedSetOffset_ns = nestedOffsetSetStats.mean_ns;
    g_perfReport.nestedSetCt_ns = nestedCtSetStats.mean_ns;
    g_perfReport.nativeRead_ns = nativeReadStats.mean_ns;
    g_perfReport.nativeWrite_ns = nativeWriteStats.mean_ns;
}

void TestContainerPerformance() {
    PrintSeparator("性能专项: 容器反射");

    Transform t;
    t.tags = {1, 2, 3, 4, 5};
    t.properties["health"] = 100;
    t.properties["mana"] = 50;
    t.flags = {1, 2, 3};

    auto* typeInfo = shine::reflection::TypeRegistry::Get().FindFast(shine::reflection::GetTypeId<Transform>());
    if (!typeInfo) {
        return;
    }

    auto* tagsField = typeInfo->FindField("tags");
    auto* propsField = typeInfo->FindField("properties");
    auto* flagsField = typeInfo->FindField("flags");

    auto* tagsContainer = reinterpret_cast<std::vector<int>*>(reinterpret_cast<char*>(&t) + tagsField->offset);
    auto* propsContainer = reinterpret_cast<std::map<std::string, int>*>(reinterpret_cast<char*>(&t) + propsField->offset);
    auto* flagsContainer = reinterpret_cast<std::set<int>*>(reinterpret_cast<char*>(&t) + flagsField->offset);

    const double nativeVecGet = BenchmarkNs([&]() { return t.tags.size(); }, 100000);
    const double nativeVecAccess = BenchmarkNs([&]() { return t.tags[0]; }, 100000);
    const double nativeMapFind = BenchmarkNs([&]() {
        const auto it = t.properties.find("health");
        return it != t.properties.end();
    }, 100000);
    const double nativeSetCount = BenchmarkNs([&]() { return t.flags.count(2); }, 100000);

    const double reflectVecGet = BenchmarkNs([&]() { return tagsContainer->size(); }, 100000);
    const double reflectVecAccess = BenchmarkNs([&]() { return (*tagsContainer)[0]; }, 100000);
    const double reflectMapFind = BenchmarkNs([&]() {
        const auto it = propsContainer->find("health");
        return it != propsContainer->end();
    }, 100000);
    const double reflectSetCount = BenchmarkNs([&]() { return flagsContainer->count(2); }, 100000);

    fmt::print("容器反射访问对比:\n");
    PrintBenchmarkLine("vector::size native", nativeVecGet);
    PrintBenchmarkLine("vector::size offset", reflectVecGet, nativeVecGet);
    PrintBenchmarkLine("vector::[] native", nativeVecAccess);
    PrintBenchmarkLine("vector::[] offset", reflectVecAccess, nativeVecAccess);
    PrintBenchmarkLine("map::find native", nativeMapFind);
    PrintBenchmarkLine("map::find offset", reflectMapFind, nativeMapFind);
    PrintBenchmarkLine("set::count native", nativeSetCount);
    PrintBenchmarkLine("set::count offset", reflectSetCount, nativeSetCount);
}

void TestAllFieldsPerformance() {
    PrintSeparator("性能专项: 所有字段反射访问");

    Transform t;
    t.position = Vec3(1, 2, 3);
    t.rotation = Vec3(0, 0, 0);
    t.scale = Vec3(1, 1, 1);
    t.name = "TestObject";
    t.id = 42;
    t.enabled = true;
    t.tags = {1, 2, 3};
    t.properties = {{"hp", 100}};
    t.flags = {1};

    auto* typeInfo = shine::reflection::TypeRegistry::Get().FindFast(shine::reflection::GetTypeId<Transform>());
    if (!typeInfo) {
        return;
    }

    fmt::print("Transform 字段数量: {}\n\n", typeInfo->GetFieldCount());
    fmt::print("{:<15} {:>10} {:>10} {:>10} {:>10}\n", "字段", "原生", "反射", "offset", "CT_GET");
    fmt::print("--------------------------------------------------------\n");

    for (const auto& field : typeInfo->GetFields()) {
        const char* fieldName = field.GetNameView().data();

        double nativeTime = 0.0;
        if (strcmp(fieldName, "position") == 0) nativeTime = BenchmarkNs([&]() { Vec3 v = t.position; (void)v; }, 100000);
        else if (strcmp(fieldName, "rotation") == 0) nativeTime = BenchmarkNs([&]() { Vec3 v = t.rotation; (void)v; }, 100000);
        else if (strcmp(fieldName, "scale") == 0) nativeTime = BenchmarkNs([&]() { Vec3 v = t.scale; (void)v; }, 100000);
        else if (strcmp(fieldName, "name") == 0) nativeTime = BenchmarkNs([&]() { std::string_view v = t.name; (void)v; }, 100000);
        else if (strcmp(fieldName, "id") == 0) nativeTime = BenchmarkNs([&]() { int v = t.id; (void)v; }, 100000);
        else if (strcmp(fieldName, "enabled") == 0) nativeTime = BenchmarkNs([&]() { bool v = t.enabled; (void)v; }, 100000);
        else if (strcmp(fieldName, "tags") == 0) nativeTime = BenchmarkNs([&]() { auto v = t.tags.size(); (void)v; }, 100000);
        else if (strcmp(fieldName, "properties") == 0) nativeTime = BenchmarkNs([&]() { auto v = t.properties.size(); (void)v; }, 100000);
        else if (strcmp(fieldName, "flags") == 0) nativeTime = BenchmarkNs([&]() { auto v = t.flags.size(); (void)v; }, 100000);

        double reflectTime = 0.0;
        if (strcmp(fieldName, "position") == 0 || strcmp(fieldName, "rotation") == 0 || strcmp(fieldName, "scale") == 0)
            reflectTime = BenchmarkNs([&]() { auto* f = typeInfo->FindField(fieldName); if (f) { Vec3 v; f->Get(&t, &v); } }, 100000);
        else if (strcmp(fieldName, "name") == 0)
            reflectTime = BenchmarkNs([&]() { auto* f = typeInfo->FindField(fieldName); if (f) { std::string v; f->Get(&t, &v); } }, 100000);
        else if (strcmp(fieldName, "id") == 0)
            reflectTime = BenchmarkNs([&]() { auto* f = typeInfo->FindField(fieldName); if (f) { int v; f->Get(&t, &v); } }, 100000);
        else if (strcmp(fieldName, "enabled") == 0)
            reflectTime = BenchmarkNs([&]() { auto* f = typeInfo->FindField(fieldName); if (f) { bool v; f->Get(&t, &v); } }, 100000);
        else if (strcmp(fieldName, "tags") == 0)
            reflectTime = BenchmarkNs([&]() { auto* f = typeInfo->FindField(fieldName); if (f) { std::vector<int> v; f->Get(&t, &v); } }, 100000);
        else if (strcmp(fieldName, "properties") == 0)
            reflectTime = BenchmarkNs([&]() { auto* f = typeInfo->FindField(fieldName); if (f) { std::map<std::string, int> v; f->Get(&t, &v); } }, 100000);
        else if (strcmp(fieldName, "flags") == 0)
            reflectTime = BenchmarkNs([&]() { auto* f = typeInfo->FindField(fieldName); if (f) { std::set<int> v; f->Get(&t, &v); } }, 100000);

        double offsetTime = 0.0;
        if (field.isPod) {
            if (strcmp(fieldName, "position") == 0 || strcmp(fieldName, "rotation") == 0 || strcmp(fieldName, "scale") == 0) {
                offsetTime = BenchmarkNs([&]() { auto* ptr = reinterpret_cast<Vec3*>(reinterpret_cast<char*>(&t) + field.offset); Vec3 v = *ptr; }, 100000);
            } else if (strcmp(fieldName, "id") == 0) {
                offsetTime = BenchmarkNs([&]() { auto* ptr = reinterpret_cast<int*>(reinterpret_cast<char*>(&t) + field.offset); int v = *ptr; }, 100000);
            } else if (strcmp(fieldName, "enabled") == 0) {
                offsetTime = BenchmarkNs([&]() { auto* ptr = reinterpret_cast<bool*>(reinterpret_cast<char*>(&t) + field.offset); bool v = *ptr; }, 100000);
            }
        } else {
            if (strcmp(fieldName, "name") == 0) {
                offsetTime = BenchmarkNs([&]() { auto* ptr = reinterpret_cast<std::string*>(reinterpret_cast<char*>(&t) + field.offset); (void)ptr->data(); }, 100000);
            } else if (strcmp(fieldName, "tags") == 0) {
                offsetTime = BenchmarkNs([&]() { auto* ptr = reinterpret_cast<std::vector<int>*>(reinterpret_cast<char*>(&t) + field.offset); (void)ptr->data(); }, 100000);
            } else if (strcmp(fieldName, "properties") == 0) {
                offsetTime = BenchmarkNs([&]() { auto* ptr = reinterpret_cast<std::map<std::string, int>*>(reinterpret_cast<char*>(&t) + field.offset); (void)ptr->begin(); }, 100000);
            } else if (strcmp(fieldName, "flags") == 0) {
                offsetTime = BenchmarkNs([&]() { auto* ptr = reinterpret_cast<std::set<int>*>(reinterpret_cast<char*>(&t) + field.offset); (void)ptr->begin(); }, 100000);
            }
        }

        double ctGetTime = 0.0;
        if (field.isPod) {
            if (strcmp(fieldName, "position") == 0) ctGetTime = BenchmarkNs([&]() { Vec3 v = shine::reflection::CT_GET<&Transform::position>(t); (void)v; }, 100000);
            else if (strcmp(fieldName, "rotation") == 0) ctGetTime = BenchmarkNs([&]() { Vec3 v = shine::reflection::CT_GET<&Transform::rotation>(t); (void)v; }, 100000);
            else if (strcmp(fieldName, "scale") == 0) ctGetTime = BenchmarkNs([&]() { Vec3 v = shine::reflection::CT_GET<&Transform::scale>(t); (void)v; }, 100000);
            else if (strcmp(fieldName, "id") == 0) ctGetTime = BenchmarkNs([&]() { int v = shine::reflection::CT_GET<&Transform::id>(t); (void)v; }, 100000);
            else if (strcmp(fieldName, "enabled") == 0) ctGetTime = BenchmarkNs([&]() { bool v = shine::reflection::CT_GET<&Transform::enabled>(t); (void)v; }, 100000);
        }

        fmt::print("{:<15} {:>10.4f} {:>10.4f} {:>10.4f} {:>10.4f}\n", fieldName, nativeTime, reflectTime, offsetTime, ctGetTime);
    }
}

void TestTypeLookupPerformance() {
    PrintSeparator("性能专项: 类型查找");

    constexpr int iterations = 100000;
    const auto findSlowStats = MeasureBenchmark([&]() {
        return shine::reflection::TypeRegistry::Get().Find(shine::reflection::GetTypeId<Transform>());
    }, iterations);
    const auto findFastStats = MeasureBenchmark([&]() {
        return shine::reflection::TypeRegistry::Get().FindFast(shine::reflection::GetTypeId<Transform>());
    }, iterations);

    PrintBenchmarkLine("TypeRegistry::Find", findSlowStats.mean_ns);
    PrintBenchmarkLine("TypeRegistry::FindFast", findFastStats.mean_ns, findSlowStats.mean_ns);

    g_perfReport.typeFindSlow_ns = findSlowStats.mean_ns;
    g_perfReport.typeFindFast_ns = findFastStats.mean_ns;
    g_perfReport.findSpeedup_x = SafeRatio(findSlowStats.mean_ns, findFastStats.mean_ns);
}

void TestMethodCallPerformance() {
    PrintSeparator("性能专项: 方法调用");

    auto* typeInfo = shine::reflection::TypeRegistry::Get().FindFast(shine::reflection::GetTypeId<Transform>());
    if (!typeInfo || typeInfo->GetMethodCount() == 0) {
        fmt::print("Transform 没有注册方法\n");
        return;
    }

    Transform t;
    auto* method = typeInfo->FindMethodFast("SetPosition");
    if (!method) {
        fmt::print("无法找到 SetPosition 方法\n");
        return;
    }

    float args[3] = {10.0f, 20.0f, 30.0f};
    void* argsPtr[] = {args, args + 1, args + 2};

    const auto lookupInvokeStats = MeasureBenchmark([&]() {
        auto* found = typeInfo->FindMethodFast("SetPosition");
        found->Invoke(&t, argsPtr, nullptr);
    }, 12000);

    const auto cachedInvokeStats = MeasureBenchmark([&]() {
        method->Invoke(&t, argsPtr, nullptr);
    }, 12000);

    const auto nativeStats = MeasureBenchmark([&]() {
        t.SetPosition(10.0f, 20.0f, 30.0f);
    }, 12000);

    PrintBenchmarkLine("native call", nativeStats.mean_ns);
    PrintBenchmarkLine("cached Invoke", cachedInvokeStats.mean_ns, nativeStats.mean_ns);
    PrintBenchmarkLine("lookup + Invoke", lookupInvokeStats.mean_ns, nativeStats.mean_ns);

    g_perfReport.methodNative_ns = nativeStats.mean_ns;
    g_perfReport.methodCachedInvoke_ns = cachedInvokeStats.mean_ns;
    g_perfReport.methodLookupInvoke_ns = lookupInvokeStats.mean_ns;
}

void TestMemoryUsage() {
    PrintSeparator("性能专项: 内存使用快照");

    const auto count = shine::reflection::TypeRegistry::Get().GetRegisteredTypeCount();
    const std::size_t estimated = sizeof(shine::reflection::TypeInfo) * count;
    fmt::print("已注册类型数量: {}\n", count);
    fmt::print("估算 TypeInfo 内存: ~{} bytes\n", estimated);
}

int main() {
    fmt::print("============================================================\n");
    fmt::print("         ShineEngine 反射性能专项测试\n");
    fmt::print("============================================================\n");
    fmt::print("建议使用 Release 构建运行此测试\n");

    TestBoundPathConvenience();
    TestFieldAccessPerformance();
    TestBoundMethodPathPerformance();
    TestContainerPerformance();
    TestAllFieldsPerformance();
    TestTypeLookupPerformance();
    TestMethodCallPerformance();
    TestMemoryUsage();
    g_perfReport.Print();

    fmt::print("============================================================\n");
    fmt::print("                    测试完成\n");
    fmt::print("============================================================\n");
    return 0;
}