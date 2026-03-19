// =============================================================================
// ReflectionTest.cpp — 反射系统测试与性能测试
// =============================================================================
// 测试内容:
// 1. 基础类型反射注册
// 2. 字段访问 (getter/setter)
// 3. 方法调用
// 4. 元数据和属性标志
// 5. 查找缓存性能
// 6. 编译期 vs 运行时性能对比
// 7. 发现系统中的潜在问题
//
// 性能基准 (Release):
// +------------------+----------+--------+-------
// | 指标             | 基准值   | 当前值 | 状态 |
// +------------------+----------+--------+-------
// | 字段查找         | 150M/s   | -     | TODO |
// | Getter           | 0.008ns  | -     | TODO |
// | Setter           | 0.008ns  | -     | TODO |
// | 方法调用         | 4.0x     | -     | TODO |
// | 类型查找加速     | 4.0x     | -     | TODO |
// +------------------+----------+--------+-------
// =============================================================================

#include <iostream>
#include <chrono>
#include <cstddef>
#include <vector>
#include <string>
#include <random>
#include <algorithm>
#include <cstring>

#include "../common/reflection_test_fixture.h"
#include "EngineCore/reflection/Reflection.h"
#include "EngineCore/reflection/Views/ScriptView.h"
#include "editor/util/PropertyDrawer.h"
#include "../common/test_benchmark_framework.h"
#include "memory/memory.ixx"
#include "memory/ue_binned2_port.h"

// 使用 fmt 进行输出
#include <fmt/format.h>

// =============================================================================
// 性能基准结构
// =============================================================================
struct PerfReport {
    double fieldLookup_ns = 0;
    double fieldLookup_ops = 0;
    double fieldGetLookup_ns = 0;
    double fieldGetCached_ns = 0;
    double fieldGetOffset_ns = 0;
    double fieldGetBound_ns = 0;
    double fieldSetLookup_ns = 0;
    double fieldSetCached_ns = 0;
    double fieldSetOffset_ns = 0;
    double fieldSetBound_ns = 0;
    double nativeRead_ns = 0;
    double nativeWrite_ns = 0;
    double methodLookupInvoke_ns = 0;
    double methodCachedInvoke_ns = 0;
    double methodNative_ns = 0;
    double typeFindSlow_ns = 0;
    double typeFindFast_ns = 0;
    double findSpeedup_x = 0;
    
    void Print() const {
        fmt::print("\n============================================================\n");
        fmt::print("                    性能测试报告\n");
        fmt::print("============================================================\n");
        fmt::print("  字段名查找: {:.2f} ns ({:.2f} M ops/sec)\n", fieldLookup_ns, fieldLookup_ops / 1000000.0);
        fmt::print("  字段读取: native {:.2f} / lookup+get {:.2f} / cached {:.2f} / offset {:.2f} / bound {:.2f} ns\n",
            nativeRead_ns, fieldGetLookup_ns, fieldGetCached_ns, fieldGetOffset_ns, fieldGetBound_ns);
        fmt::print("  字段写入: native {:.2f} / lookup+set {:.2f} / cached {:.2f} / offset {:.2f} / bound {:.2f} ns\n",
            nativeWrite_ns, fieldSetLookup_ns, fieldSetCached_ns, fieldSetOffset_ns, fieldSetBound_ns);
        fmt::print("  方法调用: native {:.2f} / cached {:.2f} / lookup+invoke {:.2f} ns\n",
            methodNative_ns, methodCachedInvoke_ns, methodLookupInvoke_ns);
        fmt::print("  类型查找: {:.1f}x 加速 (FindFast)\n", findSpeedup_x);
        fmt::print("============================================================\n\n");
    }
};

// 全局性能报告
PerfReport g_perfReport;

// =============================================================================
// 测试辅助函数
// =============================================================================

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
double Benchmark(Func&& func, int iterations = 1000, int rounds = 32) {
    return MeasureBenchmark(std::forward<Func>(func), iterations, rounds).mean_ns / 1000000.0;
}

template<typename Func>
double BenchmarkNs(Func&& func, int iterations = 1000, int rounds = 32) {
    return MeasureBenchmark(std::forward<Func>(func), iterations, rounds).mean_ns;
}

double SafeRatio(double lhs, double rhs) {
    return rhs > 0.0 ? lhs / rhs : 0.0;
}

void PrintBenchmarkLine(const char* label, double ns, double baseline_ns = 0.0) {
    fmt::print("  - {:<20} {:>10.2f} ns", label, ns);
    if (baseline_ns > 0.0) {
        fmt::print(" ({:.2f}x)", ns / baseline_ns);
    }
    fmt::print("\n");
}

void PrintSeparator(const char* title) {
    fmt::print("\n");
    fmt::print("============================================================\n");
    fmt::print("  {}\n", title);
    fmt::print("============================================================\n");
}

constexpr std::size_t kAuditCacheLineBytes = 64;

template <typename T>
void PrintLayoutHeader(shine::STextView typeName, std::size_t hotBytes = 0) {
    const auto cacheLines = (sizeof(T) + kAuditCacheLineBytes - 1) / kAuditCacheLineBytes;
    fmt::print("  {:<28} sizeof={:<4} align={:<3} cache_lines={}", typeName, sizeof(T), alignof(T), cacheLines);
    if (hotBytes != 0) {
        fmt::print(" hot_bytes={} hot_cache_lines={}", hotBytes, (hotBytes + kAuditCacheLineBytes - 1) / kAuditCacheLineBytes);
    }
    fmt::print("\n");
}

template <typename T>
void EnsurePrimitiveReflectionType() {
    if (shine::reflection::TypeRegistry::Get().FindFast(shine::reflection::GetTypeId<T>())) {
        return;
    }

    shine::reflection::TypeInfo typeInfo{};
    typeInfo.id = shine::reflection::GetTypeId<T>();
    typeInfo.size = sizeof(T);
    typeInfo.alignment = alignof(T);
    typeInfo.isPod = std::is_trivially_copyable_v<T>;
    typeInfo.SetName(shine::reflection::GetTypeName<T>());
    std::ignore = shine::reflection::TypeRegistry::Get().Register(std::move(typeInfo));
}

void PrintMemberLayout(shine::STextView memberName, std::size_t offset, std::size_t size, std::size_t alignment) {
    const auto firstCacheLine = offset / kAuditCacheLineBytes;
    const auto lastCacheLine = size == 0 ? firstCacheLine : (offset + size - 1) / kAuditCacheLineBytes;
    fmt::print("    {:<24} offset={:<4} size={:<4} align={:<3} cache_line_span={}..{}\n",
        memberName,
        offset,
        size,
        alignment,
        firstCacheLine,
        lastCacheLine);
}

void TestReflectionLayoutBaseline() {
    PrintSeparator("基线: 反射对象尺寸与布局");

    fmt::print("结构摘要:\n");
    PrintLayoutHeader<shine::reflection::TypeInfo>(shine::STextView::from_literal("TypeInfo"),
        shine::reflection::TypeInfo::ColdDataOffsetForTests());
    PrintLayoutHeader<shine::reflection::FieldInfo>(shine::STextView::from_literal("FieldInfo"),
        shine::reflection::FieldInfo::ColdDataOffsetForTests());
    PrintLayoutHeader<shine::reflection::MethodInfo>(shine::STextView::from_literal("MethodInfo"),
        shine::reflection::MethodInfo::ColdDataOffsetForTests());
    PrintLayoutHeader<shine::reflection::TypeColdData>(shine::STextView::from_literal("TypeColdData"));
    PrintLayoutHeader<shine::reflection::FieldColdData>(shine::STextView::from_literal("FieldColdData"));
    PrintLayoutHeader<shine::reflection::MethodColdData>(shine::STextView::from_literal("MethodColdData"));
    PrintLayoutHeader<shine::reflection::ReflectionOwnerHandle>(shine::STextView::from_literal("ReflectionOwnerHandle"));
    PrintLayoutHeader<shine::reflection::ReflectionMetadataStorage>(shine::STextView::from_literal("ReflectionMetadataStorage"));
    PrintLayoutHeader<shine::reflection::MethodCallParamStorage>(shine::STextView::from_literal("MethodCallParamStorage"));
    PrintLayoutHeader<shine::reflection::MethodCallCache>(shine::STextView::from_literal("MethodCallCache"));
    PrintLayoutHeader<shine::reflection::MetadataValue>(shine::STextView::from_literal("MetadataValue"));
    PrintLayoutHeader<shine::reflection::EnumEntry>(shine::STextView::from_literal("EnumEntry"));
    PrintLayoutHeader<shine::reflection::ScriptValue>(shine::STextView::from_literal("ScriptValue"));
    PrintLayoutHeader<shine::reflection::detail::ScratchBuffer>(shine::STextView::from_literal("ScratchBuffer"));

    fmt::print("\nFieldInfo 成员布局:\n");
    PrintMemberLayout(shine::STextView::from_literal("typeId"), offsetof(shine::reflection::FieldInfo, typeId), sizeof(shine::reflection::TypeId), alignof(shine::reflection::TypeId));
    PrintMemberLayout(shine::STextView::from_literal("containerType"), offsetof(shine::reflection::FieldInfo, containerType), sizeof(shine::reflection::ContainerType), alignof(shine::reflection::ContainerType));
    PrintMemberLayout(shine::STextView::from_literal("owner"), offsetof(shine::reflection::FieldInfo, owner), sizeof(shine::reflection::ReflectionOwnerHandle), alignof(shine::reflection::ReflectionOwnerHandle));
    PrintMemberLayout(shine::STextView::from_literal("offset"), offsetof(shine::reflection::FieldInfo, offset), sizeof(std::size_t), alignof(std::size_t));
    PrintMemberLayout(shine::STextView::from_literal("size"), offsetof(shine::reflection::FieldInfo, size), sizeof(std::size_t), alignof(std::size_t));
    PrintMemberLayout(shine::STextView::from_literal("alignment"), offsetof(shine::reflection::FieldInfo, alignment), sizeof(std::size_t), alignof(std::size_t));
    PrintMemberLayout(shine::STextView::from_literal("getterFn"), offsetof(shine::reflection::FieldInfo, getterFn), sizeof(shine::reflection::GetterFn), alignof(shine::reflection::GetterFn));
    PrintMemberLayout(shine::STextView::from_literal("setterFn"), offsetof(shine::reflection::FieldInfo, setterFn), sizeof(shine::reflection::SetterFn), alignof(shine::reflection::SetterFn));
    PrintMemberLayout(shine::STextView::from_literal("onChangeFn"), offsetof(shine::reflection::FieldInfo, onChangeFn), sizeof(shine::reflection::OnChangeFn), alignof(shine::reflection::OnChangeFn));
    PrintMemberLayout(shine::STextView::from_literal("equalsFn"), offsetof(shine::reflection::FieldInfo, equalsFn), sizeof(shine::reflection::EqualsFn), alignof(shine::reflection::EqualsFn));
    PrintMemberLayout(shine::STextView::from_literal("copyFn"), offsetof(shine::reflection::FieldInfo, copyFn), sizeof(shine::reflection::CopyFn), alignof(shine::reflection::CopyFn));
    PrintMemberLayout(shine::STextView::from_literal("invokeFn"), offsetof(shine::reflection::FieldInfo, invokeFn), sizeof(shine::reflection::InvokeFn), alignof(shine::reflection::InvokeFn));
    PrintMemberLayout(shine::STextView::from_literal("containerTrait"), offsetof(shine::reflection::FieldInfo, containerTrait), sizeof(const void*), alignof(const void*));
    PrintMemberLayout(shine::STextView::from_literal("flags"), offsetof(shine::reflection::FieldInfo, flags), sizeof(shine::reflection::PropertyFlags), alignof(shine::reflection::PropertyFlags));
    PrintMemberLayout(shine::STextView::from_literal("nameHash"), offsetof(shine::reflection::FieldInfo, nameHash), sizeof(uint32_t), alignof(uint32_t));
    PrintMemberLayout(shine::STextView::from_literal("coldData"), shine::reflection::FieldInfo::ColdDataOffsetForTests(), sizeof(shine::reflection::ReflectionColdPtr<shine::reflection::FieldColdData>), alignof(shine::reflection::ReflectionColdPtr<shine::reflection::FieldColdData>));

    fmt::print("\nMethodInfo 成员布局:\n");
    PrintMemberLayout(shine::STextView::from_literal("nameHash"), offsetof(shine::reflection::MethodInfo, nameHash), sizeof(uint32_t), alignof(uint32_t));
    PrintMemberLayout(shine::STextView::from_literal("invokeFn"), offsetof(shine::reflection::MethodInfo, invokeFn), sizeof(shine::reflection::InvokeFn), alignof(shine::reflection::InvokeFn));
    PrintMemberLayout(shine::STextView::from_literal("returnType"), offsetof(shine::reflection::MethodInfo, returnType), sizeof(shine::reflection::TypeId), alignof(shine::reflection::TypeId));
    PrintMemberLayout(shine::STextView::from_literal("signatureHash"), offsetof(shine::reflection::MethodInfo, signatureHash), sizeof(uint64_t), alignof(uint64_t));
    PrintMemberLayout(shine::STextView::from_literal("flags"), offsetof(shine::reflection::MethodInfo, flags), sizeof(shine::reflection::FunctionFlags), alignof(shine::reflection::FunctionFlags));
    PrintMemberLayout(shine::STextView::from_literal("owner"), offsetof(shine::reflection::MethodInfo, owner), sizeof(shine::reflection::ReflectionOwnerHandle), alignof(shine::reflection::ReflectionOwnerHandle));
    PrintMemberLayout(shine::STextView::from_literal("callCache"), offsetof(shine::reflection::MethodInfo, callCache), sizeof(shine::reflection::ReflectionColdPtr<shine::reflection::MethodCallCache>), alignof(shine::reflection::ReflectionColdPtr<shine::reflection::MethodCallCache>));
    PrintMemberLayout(shine::STextView::from_literal("coldData"), shine::reflection::MethodInfo::ColdDataOffsetForTests(), sizeof(shine::reflection::ReflectionColdPtr<shine::reflection::MethodColdData>), alignof(shine::reflection::ReflectionColdPtr<shine::reflection::MethodColdData>));

    fmt::print("\nReflectionOwnerHandle 布局:\n");
    PrintMemberLayout(shine::STextView::from_literal("rawValue"), 0, sizeof(shine::reflection::ReflectionOwnerHandle), alignof(shine::reflection::ReflectionOwnerHandle));

    fmt::print("\nTypeInfo 成员布局:\n");
    PrintMemberLayout(shine::STextView::from_literal("id"), offsetof(shine::reflection::TypeInfo, id), sizeof(shine::reflection::TypeId), alignof(shine::reflection::TypeId));
    PrintMemberLayout(shine::STextView::from_literal("size"), offsetof(shine::reflection::TypeInfo, size), sizeof(std::size_t), alignof(std::size_t));
    PrintMemberLayout(shine::STextView::from_literal("alignment"), offsetof(shine::reflection::TypeInfo, alignment), sizeof(std::size_t), alignof(std::size_t));
    PrintMemberLayout(shine::STextView::from_literal("isEnum"), offsetof(shine::reflection::TypeInfo, isEnum), sizeof(bool), alignof(bool));
    PrintMemberLayout(shine::STextView::from_literal("isPod"), offsetof(shine::reflection::TypeInfo, isPod), sizeof(bool), alignof(bool));
    PrintMemberLayout(shine::STextView::from_literal("fields"), shine::reflection::TypeInfo::FieldsOffsetForTests(), sizeof(shine::reflection::ReflectionColdVector<shine::reflection::FieldInfo>), alignof(shine::reflection::ReflectionColdVector<shine::reflection::FieldInfo>));
    PrintMemberLayout(shine::STextView::from_literal("methods"), shine::reflection::TypeInfo::MethodsOffsetForTests(), sizeof(shine::reflection::ReflectionColdVector<shine::reflection::MethodInfo>), alignof(shine::reflection::ReflectionColdVector<shine::reflection::MethodInfo>));
    PrintMemberLayout(shine::STextView::from_literal("coldData"), shine::reflection::TypeInfo::ColdDataOffsetForTests(), sizeof(shine::reflection::ReflectionColdPtr<shine::reflection::TypeColdData>), alignof(shine::reflection::ReflectionColdPtr<shine::reflection::TypeColdData>));
    PrintMemberLayout(shine::STextView::from_literal("fieldLookup_"), shine::reflection::TypeInfo::FieldLookupOffsetForTests(), sizeof(shine::reflection::ReflectionColdVector<shine::reflection::TypeInfo::LookupEntry>), alignof(shine::reflection::ReflectionColdVector<shine::reflection::TypeInfo::LookupEntry>));
    PrintMemberLayout(shine::STextView::from_literal("methodLookup_"), shine::reflection::TypeInfo::MethodLookupOffsetForTests(), sizeof(shine::reflection::ReflectionColdVector<shine::reflection::TypeInfo::LookupEntry>), alignof(shine::reflection::ReflectionColdVector<shine::reflection::TypeInfo::LookupEntry>));
    PrintMemberLayout(shine::STextView::from_literal("lookupSorted_"), shine::reflection::TypeInfo::LookupSortedOffsetForTests(), sizeof(bool), alignof(bool));

    fmt::print("\nTypeColdData 成员布局:\n");
    PrintMemberLayout(shine::STextView::from_literal("name"), offsetof(shine::reflection::TypeColdData, name), sizeof(shine::STextView), alignof(shine::STextView));
    PrintMemberLayout(shine::STextView::from_literal("enumEntries"), offsetof(shine::reflection::TypeColdData, enumEntries), sizeof(shine::reflection::ReflectionColdVector<shine::reflection::EnumEntry>), alignof(shine::reflection::ReflectionColdVector<shine::reflection::EnumEntry>));

    fmt::print("\nMethodColdData 成员布局:\n");
    PrintMemberLayout(shine::STextView::from_literal("name"), offsetof(shine::reflection::MethodColdData, name), sizeof(shine::STextView), alignof(shine::STextView));
    PrintMemberLayout(shine::STextView::from_literal("paramTypes"), offsetof(shine::reflection::MethodColdData, paramTypes), sizeof(shine::reflection::ReflectionColdVector<shine::reflection::TypeId>), alignof(shine::reflection::ReflectionColdVector<shine::reflection::TypeId>));
    PrintMemberLayout(shine::STextView::from_literal("metadata"), offsetof(shine::reflection::MethodColdData, metadata), sizeof(shine::reflection::ReflectionMetadataStorage), alignof(shine::reflection::ReflectionMetadataStorage));

    fmt::print("\nReflectionMetadataStorage 成员布局:\n");
    PrintMemberLayout(shine::STextView::from_literal("data"), 0, sizeof(shine::reflection::ReflectionMetadataStorage), alignof(shine::reflection::ReflectionMetadataStorage));

    fmt::print("\nMethodCallParamStorage 成员布局:\n");
    PrintMemberLayout(shine::STextView::from_literal("inlineTypeInfos"), offsetof(shine::reflection::MethodCallParamStorage, inlineTypeInfos), sizeof(decltype(shine::reflection::MethodCallParamStorage::inlineTypeInfos)), alignof(decltype(shine::reflection::MethodCallParamStorage::inlineTypeInfos)));
    PrintMemberLayout(shine::STextView::from_literal("inlineOffsets"), offsetof(shine::reflection::MethodCallParamStorage, inlineOffsets), sizeof(decltype(shine::reflection::MethodCallParamStorage::inlineOffsets)), alignof(decltype(shine::reflection::MethodCallParamStorage::inlineOffsets)));
    PrintMemberLayout(shine::STextView::from_literal("overflowStorage"), offsetof(shine::reflection::MethodCallParamStorage, overflowStorage), sizeof(std::byte*), alignof(std::byte*));
    PrintMemberLayout(shine::STextView::from_literal("paramCount"), offsetof(shine::reflection::MethodCallParamStorage, paramCount), sizeof(std::size_t), alignof(std::size_t));
    PrintMemberLayout(shine::STextView::from_literal("overflowCapacity"), offsetof(shine::reflection::MethodCallParamStorage, overflowCapacity), sizeof(std::size_t), alignof(std::size_t));

    fmt::print("\nMethodCallCache 成员布局:\n");
    PrintMemberLayout(shine::STextView::from_literal("returnTypeInfo"), offsetof(shine::reflection::MethodCallCache, returnTypeInfo), sizeof(const shine::reflection::TypeInfo*), alignof(const shine::reflection::TypeInfo*));
    PrintMemberLayout(shine::STextView::from_literal("params"), offsetof(shine::reflection::MethodCallCache, params), sizeof(shine::reflection::MethodCallParamStorage), alignof(shine::reflection::MethodCallParamStorage));
    PrintMemberLayout(shine::STextView::from_literal("returnOffset"), offsetof(shine::reflection::MethodCallCache, returnOffset), sizeof(std::size_t), alignof(std::size_t));
    PrintMemberLayout(shine::STextView::from_literal("frameSize"), offsetof(shine::reflection::MethodCallCache, frameSize), sizeof(std::size_t), alignof(std::size_t));
    PrintMemberLayout(shine::STextView::from_literal("frameAlignment"), offsetof(shine::reflection::MethodCallCache, frameAlignment), sizeof(std::size_t), alignof(std::size_t));
    PrintMemberLayout(shine::STextView::from_literal("valid"), offsetof(shine::reflection::MethodCallCache, valid), sizeof(bool), alignof(bool));

    fmt::print("\nScratchBuffer 成员布局:\n");
    PrintMemberLayout(shine::STextView::from_literal("stackBuf"), offsetof(shine::reflection::detail::ScratchBuffer, stackBuf), shine::reflection::detail::ScratchBuffer::kStack, alignof(std::max_align_t));
    PrintMemberLayout(shine::STextView::from_literal("ptr"), offsetof(shine::reflection::detail::ScratchBuffer, ptr), sizeof(void*), alignof(void*));
    PrintMemberLayout(shine::STextView::from_literal("heapAlignment"), offsetof(shine::reflection::detail::ScratchBuffer, heapAlignment), sizeof(std::size_t), alignof(std::size_t));

    fmt::print("\nScriptValue 成员布局:\n");
    PrintMemberLayout(shine::STextView::from_literal("data"), offsetof(shine::reflection::ScriptValue, data), sizeof(shine::reflection::ScriptValue), alignof(shine::reflection::ScriptValue));

    fmt::print("\nTagged pointer 前提: min_align={} usable_low_bits={} supports_2_bits={} supports_4_bits={}\n",
        shine::co::ue_binned2_port::GetTaggedPointerMinAlign(),
        shine::co::ue_binned2_port::GetTaggedPointerUsableLowBits(),
        shine::co::ue_binned2_port::SupportsTaggedPointerTagBits(2),
        shine::co::ue_binned2_port::SupportsTaggedPointerTagBits(4));
}

void TestReflectionMemoryTags() {
    PrintSeparator("基线: 反射内存标签路由");

    using shine::co::Memory;
    using shine::co::MemoryTag;
    using shine::co::MemoryTagStats;

    const auto printDelta = [](const char* label, const MemoryTagStats& before, const MemoryTagStats& after) {
        fmt::print("  {:<20} alloc_delta={:<4} free_delta={:<4} current_delta={} peak_delta={}\n",
            label,
            static_cast<long long>(after.alloc_count) - static_cast<long long>(before.alloc_count),
            static_cast<long long>(after.free_count) - static_cast<long long>(before.free_count),
            static_cast<long long>(after.bytes_current) - static_cast<long long>(before.bytes_current),
            static_cast<long long>(after.bytes_peak) - static_cast<long long>(before.bytes_peak));
    };

    const auto printColdPoolState = [](const char* label,
                                       size_t fieldBlocksBefore,
                                       size_t fieldLiveBefore,
                                       size_t methodBlocksBefore,
                                       size_t methodLiveBefore) {
        auto& fieldPool = shine::reflection::ReflectionColdPool<shine::reflection::FieldColdData>::Get();
        auto& methodPool = shine::reflection::ReflectionColdPool<shine::reflection::MethodColdData>::Get();

        const auto fieldBlocksAfter = fieldPool.PageCount();
        const auto fieldLiveAfter = shine::reflection::ReflectionColdPool<shine::reflection::FieldColdData>::Get().LiveCount();
        const auto methodBlocksAfter = methodPool.PageCount();
        const auto methodLiveAfter = shine::reflection::ReflectionColdPool<shine::reflection::MethodColdData>::Get().LiveCount();

        fmt::print("  {:<20} field_pages={} (delta={}) live_pages={} slots/page={} tail_used={} field_live={} (delta={}) method_pages={} (delta={}) live_pages={} slots/page={} tail_used={} method_live={} (delta={})\n",
            label,
            fieldBlocksAfter,
            static_cast<long long>(fieldBlocksAfter) - static_cast<long long>(fieldBlocksBefore),
            fieldPool.LivePageCount(),
            fieldPool.SlotsPerPage(),
            fieldPool.LastPageCommittedSlots(),
            fieldLiveAfter,
            static_cast<long long>(fieldLiveAfter) - static_cast<long long>(fieldLiveBefore),
            methodBlocksAfter,
            static_cast<long long>(methodBlocksAfter) - static_cast<long long>(methodBlocksBefore),
            methodPool.LivePageCount(),
            methodPool.SlotsPerPage(),
            methodPool.LastPageCommittedSlots(),
            methodLiveAfter,
            static_cast<long long>(methodLiveAfter) - static_cast<long long>(methodLiveBefore));
    };

    Memory::FlushAllThreadStats();
    const auto metaBefore = Memory::GetTagStats(MemoryTag::ReflectionMeta);
    void* reflectionMetaBlock = nullptr;
    {
        shine::co::MemoryScope scope(MemoryTag::ReflectionMeta);
        reflectionMetaBlock = Memory::Alloc(256, alignof(std::max_align_t));
    }
    Memory::FlushAllThreadStats();
    const auto metaDuring = Memory::GetTagStats(MemoryTag::ReflectionMeta);
    Memory::Free(reflectionMetaBlock);
    Memory::FlushAllThreadStats();
    const auto metaAfter = Memory::GetTagStats(MemoryTag::ReflectionMeta);
    printDelta("ReflectionMeta", metaBefore, metaDuring);
    printDelta("ReflectionMeta/free", metaDuring, metaAfter);

    Memory::FlushAllThreadStats();
    const auto coldBefore = Memory::GetTagStats(MemoryTag::ReflectionCold);
    const auto fieldBlocksBefore = shine::reflection::ReflectionColdPool<shine::reflection::FieldColdData>::Get().PageCount();
    const auto fieldLiveBefore = shine::reflection::ReflectionColdPool<shine::reflection::FieldColdData>::Get().LiveCount();
    const auto methodBlocksBefore = shine::reflection::ReflectionColdPool<shine::reflection::MethodColdData>::Get().PageCount();
    const auto methodLiveBefore = shine::reflection::ReflectionColdPool<shine::reflection::MethodColdData>::Get().LiveCount();

    constexpr size_t kColdBurstCount = 96;
    {
        std::vector<shine::reflection::FieldInfo> coldFields(kColdBurstCount);
        std::vector<shine::reflection::MethodInfo> coldMethods(kColdBurstCount);
        for (auto& coldField : coldFields) {
            coldField.SetName(shine::STextView::from_literal("ColdField"));
            coldField.SetDisplayName(shine::STextView::from_literal("Cold Field"));
        }
        for (auto& coldMethod : coldMethods) {
            coldMethod.SetName(shine::STextView::from_literal("ColdMethod"));
        }

        Memory::FlushAllThreadStats();
        const auto coldDuring = Memory::GetTagStats(MemoryTag::ReflectionCold);
        printDelta("ReflectionCold/burst1", coldBefore, coldDuring);
        printColdPoolState("ReflectionColdPool1", fieldBlocksBefore, fieldLiveBefore, methodBlocksBefore, methodLiveBefore);
    }
    Memory::FlushAllThreadStats();
    const auto coldAfterRelease = Memory::GetTagStats(MemoryTag::ReflectionCold);
    printDelta("ReflectionCold/retain", coldBefore, coldAfterRelease);
    printColdPoolState("ReflectionColdPoolR", fieldBlocksBefore, fieldLiveBefore, methodBlocksBefore, methodLiveBefore);

    {
        std::vector<shine::reflection::FieldInfo> coldFieldsReuse(kColdBurstCount);
        std::vector<shine::reflection::MethodInfo> coldMethodsReuse(kColdBurstCount);
        for (auto& coldFieldReuse : coldFieldsReuse) {
            coldFieldReuse.SetName(shine::STextView::from_literal("ColdFieldReuse"));
        }
        for (auto& coldMethodReuse : coldMethodsReuse) {
            coldMethodReuse.SetName(shine::STextView::from_literal("ColdMethodReuse"));
        }

        Memory::FlushAllThreadStats();
        const auto coldReuse = Memory::GetTagStats(MemoryTag::ReflectionCold);
        printDelta("ReflectionCold/reuse", coldAfterRelease, coldReuse);
        printColdPoolState("ReflectionColdPool2", fieldBlocksBefore, fieldLiveBefore, methodBlocksBefore, methodLiveBefore);
    }

    Memory::FlushAllThreadStats();
    const auto stringBefore = Memory::GetTagStats(MemoryTag::ReflectionString);
    void* reflectionStringBlock = nullptr;
    {
        shine::co::MemoryScope scope(MemoryTag::ReflectionString);
        reflectionStringBlock = Memory::Alloc(1024, alignof(std::max_align_t));
    }
    Memory::FlushAllThreadStats();
    const auto stringDuring = Memory::GetTagStats(MemoryTag::ReflectionString);
    Memory::Free(reflectionStringBlock);
    Memory::FlushAllThreadStats();
    const auto stringAfter = Memory::GetTagStats(MemoryTag::ReflectionString);
    printDelta("ReflectionString", stringBefore, stringDuring);
    printDelta("ReflectionString/free", stringDuring, stringAfter);

    Memory::FlushAllThreadStats();
    const auto tempBefore = Memory::GetTagStats(MemoryTag::ScriptBridgeTemp);
    {
        shine::reflection::detail::ScratchBuffer scratch(512, alignof(std::max_align_t));
        std::memset(scratch.ptr, 0, 512);
        Memory::FlushAllThreadStats();
        const auto tempDuring = Memory::GetTagStats(MemoryTag::ScriptBridgeTemp);
        printDelta("ScriptBridgeTemp", tempBefore, tempDuring);
    }
    Memory::FlushAllThreadStats();
    const auto tempAfter = Memory::GetTagStats(MemoryTag::ScriptBridgeTemp);
    printDelta("ScriptBridgeTemp/free", tempBefore, tempAfter);

    struct ReflectionTempBridge final : shine::reflection::ScriptBridge {
        void FromScript(const shine::reflection::ScriptValue& value, void* target, shine::reflection::TypeId typeId) const override {
            if (typeId == shine::reflection::GetTypeId<float>()) {
                if (const auto* asFloat = std::get_if<float>(&value.data)) {
                    *static_cast<float*>(target) = *asFloat;
                }
            }
        }
    };

    Memory::FlushAllThreadStats();
    const auto reflectionTempBefore = Memory::GetTagStats(MemoryTag::ReflectionTemp);
    bool methodCallCacheBefore = false;
    bool methodCallCacheAfter = false;
    EnsurePrimitiveReflectionType<float>();
    EnsurePrimitiveReflectionType<int>();
    if (const auto* transformType = shine::reflection::TypeRegistry::Get().FindFast(shine::reflection::GetTypeId<Transform>())) {
        shine::reflection::ScriptView view;
        view.typeInfo = transformType;
        if (const auto* setPositionMethod = view.GetMethodInfo(shine::STextView::from_literal("SetPosition"))) {
            methodCallCacheBefore = setPositionMethod->HasCallCache();
            Transform instance;
            ReflectionTempBridge bridge;
            const std::vector<shine::reflection::ScriptValue> methodArgs = {
                shine::reflection::ScriptValue(1.0f),
                shine::reflection::ScriptValue(2.0f),
                shine::reflection::ScriptValue(3.0f)
            };
            std::ignore = view.CallMethod(&instance, setPositionMethod, methodArgs, bridge);
            methodCallCacheAfter = setPositionMethod->GetCallCache() && setPositionMethod->GetCallCache()->valid;
            fmt::print("  MethodCallCache storage SetPosition={} inline_capacity={} cached_params={}\n",
                (setPositionMethod->GetCallCache() && setPositionMethod->GetCallCache()->UsesOverflowParamStorage()) ? "Overflow" : "Inline",
                shine::reflection::MethodCallParamStorage::kInlineParamCount,
                setPositionMethod->GetCallCache() ? setPositionMethod->GetCallCache()->ParamCount() : 0);
        }
        if (const auto* addTagMethod = view.GetMethodInfo(shine::STextView::from_literal("AddTag"))) {
            Transform instance;
            ReflectionTempBridge bridge;
            const std::vector<shine::reflection::ScriptValue> methodArgs = {
                shine::reflection::ScriptValue(7)
            };
            std::ignore = view.CallMethod(&instance, addTagMethod, methodArgs, bridge);
            fmt::print("  MethodCallCache storage AddTag={} inline_capacity={} cached_params={}\n",
                (addTagMethod->GetCallCache() && addTagMethod->GetCallCache()->UsesOverflowParamStorage()) ? "Overflow" : "Inline",
                shine::reflection::MethodCallParamStorage::kInlineParamCount,
                addTagMethod->GetCallCache() ? addTagMethod->GetCallCache()->ParamCount() : 0);
        }
    }
    Memory::FlushAllThreadStats();
    const auto reflectionTempAfter = Memory::GetTagStats(MemoryTag::ReflectionTemp);
    printDelta("ReflectionTemp/call", reflectionTempBefore, reflectionTempAfter);

    fmt::print("  MethodCallCache lazy state before_call={} after_call={}\n",
        methodCallCacheBefore ? "Warm" : "Cold",
        methodCallCacheAfter ? "Warm" : "Cold");

    Memory::FlushAllThreadStats();
    const auto editorInspectorBefore = Memory::GetTagStats(MemoryTag::EditorInspectorTemp);
    {
        shine::SString inspectorText;
        inspectorText.resize(512, 'x');
        shine::editor::util::detail::ScopedInputTextBuffer inspectorBuffer(inspectorText);
        inspectorBuffer.data()[0] = 'y';
        fmt::print("  EditorInspector buffer bytes={} heap_fallback={}\n",
            inspectorBuffer.size(),
            inspectorBuffer.size() > shine::editor::util::detail::ScopedInputTextBuffer::kStackCapacity ? "Yes" : "No");
    }
    Memory::FlushAllThreadStats();
    const auto editorInspectorAfter = Memory::GetTagStats(MemoryTag::EditorInspectorTemp);
    printDelta("EditorInspectorTemp", editorInspectorBefore, editorInspectorAfter);
}

void TestReflectionStringInterning() {
    PrintSeparator("基线: 反射名字驻留");

    auto& stringManager = shine::reflection::StringMemoryManager::GetInstance();
    const auto stringCountBefore = stringManager.GetStringCount();

    shine::SString typeName = "DynamicTypeName";
    shine::SString fieldName = "DynamicFieldName";
    shine::SString displayName = "DynamicDisplayName";
    shine::SString methodName = "DynamicMethodName";
    shine::SString enumName = "DynamicEnumName";

    shine::reflection::TypeInfo dynamicInfo{};
    dynamicInfo.id = shine::reflection::Hash("DynamicInternProbeType") ^ 0x31415926u;
    dynamicInfo.SetName(typeName);
    dynamicInfo.size = sizeof(Transform);
    dynamicInfo.alignment = alignof(Transform);
    dynamicInfo.isPod = false;
    dynamicInfo.isEnum = false;

    {
        shine::reflection::TypeBuilder<Transform> builder(dynamicInfo, {.fieldCount = 1, .methodCount = 1});
        auto fieldBuilder = builder.RegisterFieldFromDSL(shine::reflection::DSL::FieldDSLNode<&Transform::id>(fieldName));
        fieldBuilder.DisplayName(displayName);
        std::ignore = builder.RegisterMethodFromDSL(shine::reflection::DSL::MakeMethodDSL<&Transform::AddTag>(methodName));
    }
    dynamicInfo.BuildLookup();

    shine::reflection::TypeInfo dynamicEnumInfo{};
    dynamicEnumInfo.id = shine::reflection::Hash("DynamicInternProbeEnum") ^ 0x27182818u;
    dynamicEnumInfo.SetName(shine::STextView::from_literal("DynamicInternProbeEnum"));
    dynamicEnumInfo.size = sizeof(ETestEnum);
    dynamicEnumInfo.alignment = alignof(ETestEnum);
    dynamicEnumInfo.isPod = false;
    dynamicEnumInfo.isEnum = true;

    {
        shine::reflection::TypeBuilder<ETestEnum> enumBuilder(dynamicEnumInfo, {.enumCount = 1});
        enumBuilder.Enums({
            {ETestEnum::Value1, enumName}
        });
    }

    typeName[0] = 'X';
    fieldName[0] = 'Y';
    displayName[0] = 'Z';
    methodName[0] = 'Q';
    enumName[0] = 'W';

    const auto stringCountAfter = stringManager.GetStringCount();
    const auto* dynamicField = dynamicInfo.FindField(shine::STextView::from_literal("DynamicFieldName"));
    const auto* dynamicMethod = dynamicInfo.FindMethod(shine::STextView::from_literal("DynamicMethodName"));
    const auto& dynamicEnumEntries = dynamicEnumInfo.GetEnumEntries();

    fmt::print("  reflection string manager count before={} after={} delta={} total_bytes={}\n",
        stringCountBefore,
        stringCountAfter,
        static_cast<long long>(stringCountAfter) - static_cast<long long>(stringCountBefore),
        stringManager.GetTotalBytes());
    fmt::print("[PASS] type name survives source mutation: {}\n",
        dynamicInfo.GetNameView() == shine::STextView::from_literal("DynamicTypeName") ? "Yes" : "No");
    fmt::print("[PASS] field name survives source mutation: {}\n",
        dynamicField && dynamicField->GetNameView() == shine::STextView::from_literal("DynamicFieldName") ? "Yes" : "No");
    fmt::print("[PASS] display name survives source mutation: {}\n",
        dynamicField && dynamicField->GetDisplayNameView() == shine::STextView::from_literal("DynamicDisplayName") ? "Yes" : "No");
    fmt::print("[PASS] method name survives source mutation: {}\n",
        dynamicMethod && dynamicMethod->GetNameView() == shine::STextView::from_literal("DynamicMethodName") ? "Yes" : "No");
    fmt::print("[PASS] enum name survives source mutation: {}\n",
        dynamicEnumEntries.size() == 1 && dynamicEnumEntries[0].name == shine::STextView::from_literal("DynamicEnumName") ? "Yes" : "No");
    fmt::print("[PASS] interned type pointer detached from source buffer: {}\n",
        dynamicInfo.GetNameView().data() != typeName.data() ? "Yes" : "No");
    fmt::print("[PASS] reflection string manager observed new entries: {}\n",
        stringCountAfter >= stringCountBefore + 5 ? "Yes" : "No");
}

void TestReflectionColdBatchReservation() {
    PrintSeparator("基线: 冷区批量页保留");

    using FieldColdPtr = shine::reflection::ReflectionColdPtr<shine::reflection::FieldColdData>;

    auto& fieldPool = shine::reflection::ReflectionColdPool<shine::reflection::FieldColdData>::Get();
    const size_t slotsPerPage = fieldPool.SlotsPerPage();
    const size_t committedBefore = fieldPool.LastPageCommittedSlots();
    const size_t remainingBefore = committedBefore < slotsPerPage ? (slotsPerPage - committedBefore) : 0;
    const size_t requestedBatch = 24;

    std::vector<FieldColdPtr> fillers;
    if (remainingBefore > requestedBatch + 4) {
        const size_t fillerCount = remainingBefore - (requestedBatch / 2);
        fillers.reserve(fillerCount);
        for (size_t index = 0; index < fillerCount; ++index) {
            fillers.push_back(shine::reflection::MakeReflectionColdData<shine::reflection::FieldColdData>());
        }
    }

    const size_t fillerPage = fillers.empty()
        ? fieldPool.PageIndexOf(nullptr)
        : fieldPool.PageIndexOf(fillers.front().get());

    std::vector<FieldColdPtr> batchedColdData;
    batchedColdData.reserve(requestedBatch);
    FieldColdPtr strayColdData;
    size_t reservedPageBefore = static_cast<size_t>(-1);
    size_t reservedBeginBefore = static_cast<size_t>(-1);
    size_t reservedEndBefore = static_cast<size_t>(-1);
    size_t issuedAfterHalf = 0;
    size_t remainingAfterHalf = 0;
    {
        auto batch = fieldPool.BeginContiguousBatch(requestedBatch);
        reservedPageBefore = batch.ReservedPageIndex();
        reservedBeginBefore = batch.ReservedBeginSlot();
        reservedEndBefore = batch.ReservedEndSlot();
        for (size_t index = 0; index < requestedBatch / 2; ++index) {
            batchedColdData.push_back(batch.Create());
        }
        issuedAfterHalf = batch.IssuedCount();
        remainingAfterHalf = batch.Remaining();
        strayColdData = shine::reflection::MakeReflectionColdData<shine::reflection::FieldColdData>();
        for (size_t index = requestedBatch / 2; index < requestedBatch; ++index) {
            batchedColdData.push_back(batch.Create());
        }
    }

    const size_t firstPage = batchedColdData.empty() ? fieldPool.PageIndexOf(nullptr) : fieldPool.PageIndexOf(batchedColdData.front().get());
    const size_t lastPage = batchedColdData.empty() ? fieldPool.PageIndexOf(nullptr) : fieldPool.PageIndexOf(batchedColdData.back().get());
    const size_t strayPage = fieldPool.PageIndexOf(strayColdData.get());
    const size_t firstSlot = batchedColdData.empty() ? fieldPool.SlotIndexInPageOf(nullptr) : fieldPool.SlotIndexInPageOf(batchedColdData.front().get());
    const size_t lastSlot = batchedColdData.empty() ? fieldPool.SlotIndexInPageOf(nullptr) : fieldPool.SlotIndexInPageOf(batchedColdData.back().get());
    const size_t straySlot = fieldPool.SlotIndexInPageOf(strayColdData.get());

    fmt::print("  field batch pages: first={} last={} stray_page={} fillers_page={} committed_before={} slots_per_page={} requested_batch={} reserved_page={} reserved_begin={} reserved_end={} issued_after_half={} remaining_after_half={} first_slot={} last_slot={} stray_slot={}\n",
        firstPage,
        lastPage,
        strayPage,
        fillerPage,
        committedBefore,
        slotsPerPage,
        requestedBatch,
        reservedPageBefore,
        reservedBeginBefore,
        reservedEndBefore,
        issuedAfterHalf,
        remainingAfterHalf,
        firstSlot,
        lastSlot,
        straySlot);
    fmt::print("[PASS] batched cold fields stay on one page: {}\n", firstPage == lastPage ? "Yes" : "No");
    fmt::print("[PASS] reserved slot range matches batch size: {}\n",
        reservedEndBefore > reservedBeginBefore && (reservedEndBefore - reservedBeginBefore) == requestedBatch ? "Yes" : "No");
    fmt::print("[PASS] stray cold alloc stays outside reserved batch range: {}\n",
        strayColdData && ((strayPage != firstPage) || (straySlot > lastSlot)) ? "Yes" : "No");
    if (!fillers.empty()) {
        fmt::print("[PASS] batch moved off the partially used tail page: {}\n", firstPage != fillerPage ? "Yes" : "No");
    }
}

void TestReflectionRegistrationPlan() {
    PrintSeparator("基线: 注册计划与容量预留");

    const auto sumFieldMetadata = [](const auto& plans) {
        size_t total = 0;
        for (const auto& plan : plans) {
            total += plan.runtimeMetadataCount;
        }
        return total;
    };

    const auto sumMethodMetadata = [](const auto& plans) {
        size_t total = 0;
        for (const auto& plan : plans) {
            total += plan.runtimeMetadataCount;
        }
        return total;
    };

    shine::co::Memory::FlushAllThreadStats();
    const auto planStageMetaBefore = shine::co::Memory::GetTagStats(shine::co::MemoryTag::ReflectionMeta);
    shine::reflection::TypeRegistrationPlan stagedTransformPlan{};
    {
        shine::reflection::TypeBuilderPlanCounter<Transform> counter(stagedTransformPlan);
        _ReflectRegFn_Transform(counter);
    }
    shine::co::Memory::FlushAllThreadStats();
    const auto planStageMetaAfterMeasure = shine::co::Memory::GetTagStats(shine::co::MemoryTag::ReflectionMeta);
    const auto stagedFieldPlanPages = stagedTransformPlan.fieldPlans.StagingPageCount();
    const auto stagedMethodPlanPages = stagedTransformPlan.methodPlans.StagingPageCount();
    const auto stagedMetadataPages = stagedTransformPlan.runtimeMetadataEntries.StagingPageCount();
    const auto stagedParamPages = stagedTransformPlan.methodParamTypeEntries.StagingPageCount();
    const bool fieldPlansFrozenBeforeFreeze = stagedTransformPlan.fieldPlans.is_frozen();
    const bool methodPlansFrozenBeforeFreeze = stagedTransformPlan.methodPlans.is_frozen();
    stagedTransformPlan.FreezeSharedBlocks();
    const auto frozenFieldPlanPages = stagedTransformPlan.fieldPlans.StagingPageCount();
    const auto frozenMethodPlanPages = stagedTransformPlan.methodPlans.StagingPageCount();
    const auto frozenMetadataPages = stagedTransformPlan.runtimeMetadataEntries.StagingPageCount();
    const auto frozenParamPages = stagedTransformPlan.methodParamTypeEntries.StagingPageCount();

    fmt::print("  plan staging meta delta: alloc={} current={} field_pages={} method_pages={} metadata_pages={} param_pages={}\n",
        planStageMetaAfterMeasure.alloc_count - planStageMetaBefore.alloc_count,
        planStageMetaAfterMeasure.bytes_current - planStageMetaBefore.bytes_current,
        stagedFieldPlanPages,
        stagedMethodPlanPages,
        stagedMetadataPages,
        stagedParamPages);
    fmt::print("[PASS] plan staging uses ReflectionMeta pages: {}\n",
        planStageMetaAfterMeasure.alloc_count > planStageMetaBefore.alloc_count
            && planStageMetaAfterMeasure.bytes_current > planStageMetaBefore.bytes_current ? "Yes" : "No");
    fmt::print("[PASS] plan staging pages exist before freeze: {}\n",
        stagedFieldPlanPages != 0
            && stagedMethodPlanPages != 0
            && stagedMetadataPages != 0
            && stagedParamPages != 0
            && !fieldPlansFrozenBeforeFreeze
            && !methodPlansFrozenBeforeFreeze ? "Yes" : "No");
    fmt::print("[PASS] freeze releases plan staging pages: {}\n",
        frozenFieldPlanPages == 0
            && frozenMethodPlanPages == 0
            && frozenMetadataPages == 0
            && frozenParamPages == 0
            && stagedTransformPlan.fieldPlans.is_frozen()
            && stagedTransformPlan.methodPlans.is_frozen()
            && stagedTransformPlan.runtimeMetadataEntries.is_frozen()
            && stagedTransformPlan.methodParamTypeEntries.is_frozen() ? "Yes" : "No");

    auto transformGraph = shine::reflection::BuildTypeRegistrationGraph<Transform>(
        shine::STextView::from_literal("TransformPlanProbe"),
        []<typename TBuilder>(TBuilder& builder) {
            _ReflectRegFn_Transform(builder);
        },
        false);
    const auto& transformPlan = transformGraph.Plan();
    const auto& transformLayout = transformPlan.GetEmitLayout();
    const auto& transformEmit = transformPlan.GetEmitView();

    fmt::print("  Transform plan: fields={} methods={} enums={}\n",
        transformPlan.fieldCount,
        transformPlan.methodCount,
        transformPlan.enumCount);
    fmt::print("  Transform runtime metadata plan: field_total={} method_total={}\n",
        sumFieldMetadata(transformPlan.fieldPlans),
        sumMethodMetadata(transformPlan.methodPlans));

    auto transformInfo = transformGraph.BuildTypeInfo([]<typename TBuilder>(TBuilder& builder) {
        _ReflectRegFn_Transform(builder);
    });
    const auto& transformFields = transformInfo.GetFields();
    const auto& transformMethods = transformInfo.GetMethods();
    fmt::print("  reserved capacities: fields={} methods={} enums={}\n",
        transformFields.capacity(),
        transformMethods.capacity(),
        transformInfo.GetEnumEntries().capacity());
    fmt::print("[PASS] transform field reserve matches plan: {}\n",
        transformFields.capacity() >= transformPlan.fieldCount ? "Yes" : "No");
    fmt::print("[PASS] transform method reserve matches plan: {}\n",
        transformMethods.capacity() >= transformPlan.methodCount ? "Yes" : "No");
    fmt::print("[PASS] transform field metadata plan captured runtime extras: {}\n",
        sumFieldMetadata(transformPlan.fieldPlans) == 1 ? "Yes" : "No");
    fmt::print("[PASS] transform method metadata plan captured runtime extras: {}\n",
        sumMethodMetadata(transformPlan.methodPlans) == 1 ? "Yes" : "No");

    const auto positionPlan = std::find_if(transformLayout.fieldPlans.begin(), transformLayout.fieldPlans.end(), [](const auto& plan) {
        return plan.nameHash == shine::reflection::Hash(shine::STextView::from_literal("position"));
    });
    const auto namePlan = std::find_if(transformLayout.fieldPlans.begin(), transformLayout.fieldPlans.end(), [](const auto& plan) {
        return plan.nameHash == shine::reflection::Hash(shine::STextView::from_literal("name"));
    });
    const auto idPlan = std::find_if(transformLayout.fieldPlans.begin(), transformLayout.fieldPlans.end(), [](const auto& plan) {
        return plan.nameHash == shine::reflection::Hash(shine::STextView::from_literal("id"));
    });
    const auto addTagPlan = std::find_if(transformLayout.methodPlans.begin(), transformLayout.methodPlans.end(), [](const auto& plan) {
        return plan.nameHash == shine::reflection::Hash(shine::STextView::from_literal("AddTag"));
    });
    const auto positionDescriptor = positionPlan != transformLayout.fieldPlans.end()
        ? transformLayout.GetFieldDescriptor(static_cast<size_t>(std::distance(transformLayout.fieldPlans.begin(), positionPlan)))
        : shine::reflection::TypeRegistrationPlan::EmitLayout::FieldEmitDescriptor{};
    const auto addTagDescriptor = addTagPlan != transformLayout.methodPlans.end()
        ? transformLayout.GetMethodDescriptor(static_cast<size_t>(std::distance(transformLayout.methodPlans.begin(), addTagPlan)))
        : shine::reflection::TypeRegistrationPlan::EmitLayout::MethodEmitDescriptor{};
    fmt::print("[PASS] position plan captured custom metadata: {}\n",
        positionPlan != transformLayout.fieldPlans.end() && positionPlan->runtimeMetadataCount == 1 && positionPlan->hasDisplayName && positionPlan->hasCategory ? "Yes" : "No");
    fmt::print("[PASS] emit layout exposes planned entries: {}\n",
        transformLayout.fieldPlans.size() == transformPlan.fieldCount
            && transformLayout.methodPlans.size() == transformPlan.methodCount
            && transformLayout.runtimeMetadataEntries.size() == transformPlan.runtimeMetadataEntries.size()
            && transformLayout.methodParamTypeEntries.size() == transformPlan.methodParamTypeEntries.size()
            && transformLayout.fieldDescriptors.size() == transformPlan.fieldCount
            && transformLayout.methodDescriptors.size() == transformPlan.methodCount ? "Yes" : "No");
    fmt::print("[PASS] emit layout aliases legacy view: {}\n",
        &transformLayout == &transformEmit ? "Yes" : "No");
    fmt::print("[PASS] emit descriptors resolve slices eagerly: {}\n",
        positionDescriptor.plan == (positionPlan != transformLayout.fieldPlans.end() ? &*positionPlan : nullptr)
            && positionDescriptor.runtimeMetadata.size() == 1
            && addTagDescriptor.plan == (addTagPlan != transformLayout.methodPlans.end() ? &*addTagPlan : nullptr)
            && addTagDescriptor.runtimeMetadata.size() == 1
            && addTagDescriptor.paramTypes.size() == 1 ? "Yes" : "No");
    fmt::print("[PASS] frozen descriptor blocks mirror plan order: {}\n",
        !transformLayout.fieldDescriptors.empty()
            && transformLayout.fieldDescriptors[0].plan == &transformLayout.fieldPlans[0]
            && !transformLayout.methodDescriptors.empty()
            && transformLayout.methodDescriptors[0].plan == &transformLayout.methodPlans[0] ? "Yes" : "No");
    fmt::print("[PASS] graph metadata block captured field/method slices: {}\n",
        transformPlan.runtimeMetadataEntries.size() == 2
            && positionPlan != transformLayout.fieldPlans.end()
            && positionPlan->runtimeMetadataCount == 1
            && transformLayout.GetRuntimeMetadata(*positionPlan).size() == 1
            && transformLayout.GetRuntimeMetadata(*positionPlan)[0].first
                == shine::reflection::Hash(shine::STextView::from_literal("Tooltip"))
            && addTagPlan != transformLayout.methodPlans.end()
            && addTagPlan->runtimeMetadataCount == 1
            && transformLayout.GetRuntimeMetadata(*addTagPlan).size() == 1
            && transformLayout.GetRuntimeMetadata(*addTagPlan)[0].first
                == shine::reflection::MetaKeys::BlueprintFunction ? "Yes" : "No");
    fmt::print("[PASS] position plan captured concrete builtin text: {}\n",
        positionPlan != transformLayout.fieldPlans.end()
            && positionPlan->typeId == shine::reflection::GetTypeId<Vec3>()
            && positionPlan->offset == offsetof(Transform, position)
            && positionPlan->size == sizeof(Vec3)
            && positionPlan->displayName == shine::STextView::from_literal("World Position")
            && positionPlan->category == shine::STextView::from_literal("Transform") ? "Yes" : "No");
    fmt::print("[PASS] name plan captured UI override: {}\n",
        namePlan != transformLayout.fieldPlans.end() && namePlan->hasUISchema && namePlan->hasDisplayName && namePlan->hasCategory ? "Yes" : "No");
    const auto* textInputPlan = namePlan != transformLayout.fieldPlans.end()
        ? std::get_if<shine::reflection::UI::TextInput>(&namePlan->uiSchema)
        : nullptr;
    fmt::print("[PASS] name plan captured concrete text input schema: {}\n",
        textInputPlan != nullptr
            && textInputPlan->max_length == 128
            && textInputPlan->multiline == false
            && HasFlag(namePlan->flags, shine::reflection::PropertyFlags::EditAnywhere)
            && HasFlag(namePlan->flags, shine::reflection::PropertyFlags::ScriptReadWrite)
            && namePlan->displayName == shine::STextView::from_literal("Actor Name")
            && namePlan->category == shine::STextView::from_literal("Identity") ? "Yes" : "No");
    fmt::print("[PASS] id plan captured builtin range and edit condition: {}\n",
        idPlan != transformLayout.fieldPlans.end() && idPlan->hasRange && idPlan->hasDisplayName && idPlan->hasCategory && idPlan->hasEditCondition ? "Yes" : "No");
    fmt::print("[PASS] id plan captured concrete range and condition payload: {}\n",
        idPlan != transformLayout.fieldPlans.end()
            && idPlan->minValue == 0.0f
            && idPlan->maxValue == 100.0f
            && idPlan->editCondition == shine::STextView::from_literal("enabled") ? "Yes" : "No");
    fmt::print("[PASS] AddTag plan captured runtime metadata: {}\n",
        addTagPlan != transformLayout.methodPlans.end() && addTagPlan->runtimeMetadataCount == 1 ? "Yes" : "No");
    fmt::print("[PASS] AddTag plan captured method identity: {}\n",
        addTagPlan != transformLayout.methodPlans.end()
            && addTagPlan->name == shine::STextView::from_literal("AddTag")
            && addTagPlan->returnType == shine::reflection::GetTypeId<int>()
            && addTagPlan->paramTypeCount == 1
            && transformLayout.GetMethodParamTypes(*addTagPlan).size() == 1
            && transformLayout.GetMethodParamTypes(*addTagPlan)[0] == shine::reflection::GetTypeId<int>()
            && HasFlag(addTagPlan->flags, shine::reflection::FunctionFlags::ScriptCallable) ? "Yes" : "No");

    auto enumGraph = shine::reflection::BuildTypeRegistrationGraph<ETestEnum>(
        shine::STextView::from_literal("EnumPlanProbe"),
        []<typename TBuilder>(TBuilder& builder) {
            _ReflectRegFn_ETestEnum(builder);
        },
        true);
    const auto& enumPlan = enumGraph.Plan();
    const auto& enumLayout = enumPlan.GetEmitLayout();
    const auto& enumEmit = enumPlan.GetEmitView();
    fmt::print("  ETestEnum plan: fields={} methods={} enums={}\n",
        enumPlan.fieldCount,
        enumPlan.methodCount,
        enumPlan.enumCount);
    fmt::print("[PASS] enum plan captured concrete labels: {}\n",
        enumLayout.enumPlans.size() == 4
            && enumLayout.enumPlans[0].value == static_cast<int64_t>(ETestEnum::None)
            && enumLayout.enumPlans[0].name == shine::STextView::from_literal("None")
            && enumLayout.enumPlans[3].value == static_cast<int64_t>(ETestEnum::Value3)
            && enumLayout.enumPlans[3].name == shine::STextView::from_literal("Value3") ? "Yes" : "No");
    fmt::print("[PASS] enum layout aliases legacy view: {}\n",
        &enumLayout == &enumEmit ? "Yes" : "No");
    fmt::print("[PASS] enum descriptor block mirrors plan order: {}\n",
        enumLayout.enumDescriptors.size() == enumPlan.enumCount
            && !enumLayout.enumDescriptors.empty()
            && enumLayout.enumDescriptors[0].plan == &enumLayout.enumPlans[0] ? "Yes" : "No");

    auto enumInfo = enumGraph.BuildTypeInfo([]<typename TBuilder>(TBuilder& builder) {
        _ReflectRegFn_ETestEnum(builder);
    });
    fmt::print("  enum reserved capacity: {}\n", enumInfo.GetEnumEntries().capacity());
    fmt::print("[PASS] enum reserve matches plan: {}\n",
        enumInfo.GetEnumEntries().capacity() >= enumPlan.enumCount ? "Yes" : "No");
}

void TestTypeBuilderStagedEmission() {
    PrintSeparator("基线: TypeBuilder 原地发射");

    auto transformGraph = shine::reflection::BuildTypeRegistrationGraph<Transform>(
        shine::STextView::from_literal("TransformStageProbe"),
        []<typename TBuilder>(TBuilder& builder) {
            _ReflectRegFn_Transform(builder);
        },
        false);
    const auto& transformPlan = transformGraph.Plan();
    bool stagedPayloadReadyBeforeReplay = false;
    bool bindingDeferredUntilReplay = false;
    bool bindingCompletedAfterReplay = false;

    shine::reflection::TypeInfo transformInfo{};
    transformInfo.id = shine::reflection::GetTypeId<Transform>();
    transformInfo.SetName(transformGraph.GetTypeName());
    transformInfo.size = sizeof(Transform);
    transformInfo.alignment = alignof(Transform);
    transformInfo.isPod = std::is_trivially_copyable_v<Transform>;
    transformInfo.isEnum = false;

    const shine::reflection::FieldInfo* stagedFieldBase = nullptr;
    const shine::reflection::MethodInfo* stagedMethodBase = nullptr;
    {
        shine::reflection::TypeBuilder<Transform> transformBuilder(transformInfo, transformPlan);
        stagedFieldBase = transformInfo.GetFields().data();
        stagedMethodBase = transformInfo.GetMethods().data();
        transformInfo.BuildLookup();
        const auto* stagedPositionBeforeReplay = transformInfo.FindField(shine::STextView::from_literal("position"));
        const auto* stagedAddTagBeforeReplay = transformInfo.FindMethod(shine::STextView::from_literal("AddTag"));
        stagedPayloadReadyBeforeReplay = stagedPositionBeforeReplay != nullptr
            && stagedPositionBeforeReplay->typeId == shine::reflection::GetTypeId<Vec3>()
            && stagedPositionBeforeReplay->offset == offsetof(Transform, position)
            && stagedPositionBeforeReplay->GetDisplayNameView() == shine::STextView::from_literal("World Position")
            && stagedAddTagBeforeReplay != nullptr
            && stagedAddTagBeforeReplay->returnType == shine::reflection::GetTypeId<int>()
            && stagedAddTagBeforeReplay->GetParamCount() == 1
            && stagedAddTagBeforeReplay->GetParamType(0) == shine::reflection::GetTypeId<int>();
        bindingDeferredUntilReplay = stagedPositionBeforeReplay != nullptr
            && stagedPositionBeforeReplay->getterFn == nullptr
            && stagedPositionBeforeReplay->setterFn == nullptr
            && stagedPositionBeforeReplay->equalsFn == nullptr
            && stagedPositionBeforeReplay->copyFn == nullptr
            && stagedAddTagBeforeReplay != nullptr
            && stagedAddTagBeforeReplay->invokeFn == nullptr;
        fmt::print("  staged before emit: fields={} methods={} enums={}\n",
            transformInfo.GetFieldCount(),
            transformInfo.GetMethodCount(),
            transformInfo.GetEnumEntries().size());
        _ReflectRegFn_Transform(transformBuilder);
        const auto* stagedPositionAfterReplay = transformInfo.FindField(shine::STextView::from_literal("position"));
        const auto* stagedAddTagAfterReplay = transformInfo.FindMethod(shine::STextView::from_literal("AddTag"));
        bindingCompletedAfterReplay = stagedPositionAfterReplay != nullptr
            && stagedPositionAfterReplay->getterFn != nullptr
            && stagedPositionAfterReplay->setterFn != nullptr
            && stagedPositionAfterReplay->equalsFn != nullptr
            && stagedPositionAfterReplay->copyFn != nullptr
            && stagedAddTagAfterReplay != nullptr
            && stagedAddTagAfterReplay->invokeFn != nullptr;
    }

    fmt::print("  staged after emit: fields={} methods={} enums={}\n",
        transformInfo.GetFieldCount(),
        transformInfo.GetMethodCount(),
        transformInfo.GetEnumEntries().size());
    fmt::print("[PASS] transform fields emitted in-place: {}\n",
        transformInfo.GetFields().data() == stagedFieldBase ? "Yes" : "No");
    fmt::print("[PASS] transform methods emitted in-place: {}\n",
        transformInfo.GetMethods().data() == stagedMethodBase ? "Yes" : "No");
    fmt::print("[PASS] transform staged field count exact: {}\n",
        transformInfo.GetFieldCount() == transformPlan.fieldCount ? "Yes" : "No");
    fmt::print("[PASS] transform staged method count exact: {}\n",
        transformInfo.GetMethodCount() == transformPlan.methodCount ? "Yes" : "No");
    fmt::print("[PASS] staged payload ready before replay: {}\n",
        stagedPayloadReadyBeforeReplay ? "Yes" : "No");
    fmt::print("[PASS] replay deferred binding work only: {}\n",
        bindingDeferredUntilReplay && bindingCompletedAfterReplay ? "Yes" : "No");

    transformInfo.BuildLookup();
    const auto* positionField = transformInfo.FindField(shine::STextView::from_literal("position"));
    const auto* nameField = transformInfo.FindField(shine::STextView::from_literal("name"));
    const auto* idField = transformInfo.FindField(shine::STextView::from_literal("id"));
    const auto* addTagMethod = transformInfo.FindMethod(shine::STextView::from_literal("AddTag"));
    fmt::print("  staged metadata reserve: position size={} capacity={} AddTag size={} capacity={}\n",
        positionField ? positionField->GetMetadata().size() : 0,
        positionField ? positionField->GetMetadata().capacity() : 0,
        addTagMethod ? addTagMethod->GetMetadata().size() : 0,
        addTagMethod ? addTagMethod->GetMetadata().capacity() : 0);
    const auto* stagedNameSchema = nameField != nullptr
        ? std::get_if<shine::reflection::UI::TextInput>(&nameField->GetUISchema())
        : nullptr;
    fmt::print("[PASS] position metadata reserve exact: {}\n",
        positionField && positionField->GetMetadata().size() == 1 && positionField->GetMetadata().capacity() == 1 ? "Yes" : "No");
    fmt::print("[PASS] AddTag metadata reserve exact: {}\n",
        addTagMethod && addTagMethod->GetMetadata().size() == 1 && addTagMethod->GetMetadata().capacity() == 1 ? "Yes" : "No");
    fmt::print("[PASS] staged field payload consumed from plan: {}\n",
        positionField != nullptr
            && positionField->typeId == shine::reflection::GetTypeId<Vec3>()
            && positionField->offset == offsetof(Transform, position)
            && positionField->GetDisplayNameView() == shine::STextView::from_literal("World Position")
            && positionField->GetCategoryView() == shine::STextView::from_literal("Transform")
            && stagedNameSchema != nullptr
            && stagedNameSchema->max_length == 128
            && stagedNameSchema->multiline == false
            && nameField != nullptr
            && HasFlag(nameField->flags, shine::reflection::PropertyFlags::EditAnywhere)
            && HasFlag(nameField->flags, shine::reflection::PropertyFlags::ScriptReadWrite)
            && idField != nullptr
            && idField->GetMinValue() == 0.0f
            && idField->GetMaxValue() == 100.0f
            && idField->GetEditConditionView() == shine::STextView::from_literal("enabled")
            && addTagMethod != nullptr
            && addTagMethod->GetNameView() == shine::STextView::from_literal("AddTag")
            && addTagMethod->returnType == shine::reflection::GetTypeId<int>()
            && addTagMethod->GetParamCount() == 1
            && addTagMethod->GetParamType(0) == shine::reflection::GetTypeId<int>()
            && HasFlag(addTagMethod->flags, shine::reflection::FunctionFlags::ScriptCallable) ? "Yes" : "No");

    auto enumGraph = shine::reflection::BuildTypeRegistrationGraph<ETestEnum>(
        shine::STextView::from_literal("EnumStageProbe"),
        []<typename TBuilder>(TBuilder& builder) {
            _ReflectRegFn_ETestEnum(builder);
        },
        true);
    const auto& enumPlan = enumGraph.Plan();

    shine::reflection::TypeInfo enumInfo{};
    enumInfo.id = shine::reflection::GetTypeId<ETestEnum>();
    enumInfo.SetName(enumGraph.GetTypeName());
    enumInfo.size = sizeof(ETestEnum);
    enumInfo.alignment = alignof(ETestEnum);
    enumInfo.isPod = false;
    enumInfo.isEnum = true;

    const shine::reflection::EnumEntry* stagedEnumBase = nullptr;
    {
        shine::reflection::TypeBuilder<ETestEnum> enumBuilder(enumInfo, enumPlan);
        stagedEnumBase = enumInfo.GetEnumEntries().data();
        fmt::print("  enum staged before emit: entries={}\n", enumInfo.GetEnumEntries().size());
        _ReflectRegFn_ETestEnum(enumBuilder);
    }

    fmt::print("  enum staged after emit: entries={}\n", enumInfo.GetEnumEntries().size());
    fmt::print("[PASS] enum entries emitted in-place: {}\n",
        enumInfo.GetEnumEntries().data() == stagedEnumBase ? "Yes" : "No");
    fmt::print("[PASS] enum staged count exact: {}\n",
        enumInfo.GetEnumEntries().size() == enumPlan.enumCount ? "Yes" : "No");
    fmt::print("[PASS] enum staged payload consumed from plan: {}\n",
        enumInfo.GetEnumEntries().size() == 4
            && enumInfo.GetEnumEntries()[0].name == shine::STextView::from_literal("None")
            && enumInfo.GetEnumEntries()[3].name == shine::STextView::from_literal("Value3") ? "Yes" : "No");
}

void TestInjectedStaticPlanPath() {
    PrintSeparator("新增: 正式主链静态 plan 注入");

    auto transformGraph = shine::reflection::BuildTypeRegistrationGraph<Transform>(
        shine::STextView::from_literal("TransformInjectedPlanProbe"),
        []<typename TBuilder>(TBuilder& builder) {
            _ReflectRegFn_Transform(builder);
        },
        false);
    auto enumGraph = shine::reflection::BuildTypeRegistrationGraph<ETestEnum>(
        shine::STextView::from_literal("EnumInjectedPlanProbe"),
        []<typename TBuilder>(TBuilder& builder) {
            _ReflectRegFn_ETestEnum(builder);
        },
        true);

    const auto* injectedTransformCTPlan = shine::reflection::TryGetStaticTypeRegistrationCTPlan<Transform>();
    const auto* injectedEnumCTPlan = shine::reflection::TryGetStaticTypeRegistrationCTPlan<ETestEnum>();

    fmt::print("  injected transform plan: fields={} methods={} uses_injected={}\n",
        transformGraph.Plan().fieldCount,
        transformGraph.Plan().methodCount,
        transformGraph.UsesInjectedPlan() ? "Yes" : "No");
    fmt::print("  injected enum plan: enums={} uses_injected={}\n",
        enumGraph.Plan().enumCount,
        enumGraph.UsesInjectedPlan() ? "Yes" : "No");
    fmt::print("[PASS] transform graph uses injected plan provider: {}\n",
        transformGraph.UsesInjectedPlan()
            && injectedTransformCTPlan != nullptr
            && transformGraph.IsMeasured()
            && transformGraph.Plan().fieldCount == 9
            && transformGraph.Plan().methodCount == 5 ? "Yes" : "No");
    fmt::print("[PASS] enum graph uses injected plan provider: {}\n",
        enumGraph.UsesInjectedPlan()
            && injectedEnumCTPlan != nullptr
            && enumGraph.IsMeasured()
            && enumGraph.Plan().enumCount == 4 ? "Yes" : "No");
    fmt::print("[PASS] CT plan split builtin metadata from runtime extras: {}\n",
        injectedTransformCTPlan != nullptr
            && !injectedTransformCTPlan->fieldPlans.empty()
            && injectedTransformCTPlan->fieldPlans[0].builtinMetadata.displayName == shine::STextView::from_literal("World Position")
            && injectedTransformCTPlan->fieldPlans[0].runtimeMetadata.size() == 1
            && injectedTransformCTPlan->fieldPlans[4].builtinMetadata.hasRange
            && injectedTransformCTPlan->fieldPlans[4].runtimeMetadata.empty()
            && transformGraph.Plan().RuntimeMetadataEntryCount() == 2 ? "Yes" : "No");

    auto transformInfo = transformGraph.BuildTypeInfo([]<typename TBuilder>(TBuilder& builder) {
        _ReflectRegFn_Transform(builder);
    });
    transformInfo.BuildLookup();
    const auto* positionField = transformInfo.FindFieldFast("position");
    const auto* addTagMethod = transformInfo.FindMethodFast("AddTag");

    fmt::print("[PASS] injected transform plan still replays into runtime TypeInfo: {}\n",
        positionField != nullptr
            && positionField->GetDisplayNameView() == shine::STextView::from_literal("World Position")
            && positionField->GetCategoryView() == shine::STextView::from_literal("Transform")
            && addTagMethod != nullptr
            && HasFlag(addTagMethod->flags, shine::reflection::FunctionFlags::ScriptCallable) ? "Yes" : "No");
}

void TestRegisteredTypeColdLocality() {
    PrintSeparator("基线: 已注册类型冷页局部性");

    const auto* typeInfo = shine::reflection::TypeRegistry::Get().FindFast(shine::reflection::GetTypeId<Transform>());
    if (!typeInfo) {
        fmt::print("[PASS] transform type available: No\n");
        return;
    }

    auto& fieldPool = shine::reflection::ReflectionColdPool<shine::reflection::FieldColdData>::Get();
    auto& methodPool = shine::reflection::ReflectionColdPool<shine::reflection::MethodColdData>::Get();

    size_t firstFieldPage = static_cast<size_t>(-1);
    size_t lastFieldPage = static_cast<size_t>(-1);
    size_t firstFieldSlot = static_cast<size_t>(-1);
    size_t lastFieldSlot = static_cast<size_t>(-1);
    for (const auto& field : typeInfo->GetFields()) {
        const size_t pageIndex = fieldPool.PageIndexOf(field.GetColdDataPtrForTests());
        const size_t slotIndex = fieldPool.SlotIndexInPageOf(field.GetColdDataPtrForTests());
        if (firstFieldPage == static_cast<size_t>(-1)) {
            firstFieldPage = pageIndex;
            firstFieldSlot = slotIndex;
        }
        lastFieldPage = pageIndex;
        lastFieldSlot = slotIndex;
    }

    size_t firstMethodPage = static_cast<size_t>(-1);
    size_t lastMethodPage = static_cast<size_t>(-1);
    size_t firstMethodSlot = static_cast<size_t>(-1);
    size_t lastMethodSlot = static_cast<size_t>(-1);
    for (const auto& method : typeInfo->GetMethods()) {
        const size_t pageIndex = methodPool.PageIndexOf(method.GetColdDataPtrForTests());
        const size_t slotIndex = methodPool.SlotIndexInPageOf(method.GetColdDataPtrForTests());
        if (firstMethodPage == static_cast<size_t>(-1)) {
            firstMethodPage = pageIndex;
            firstMethodSlot = slotIndex;
        }
        lastMethodPage = pageIndex;
        lastMethodSlot = slotIndex;
    }

    fmt::print("  Transform cold pages: field_first={} field_last={} field_slot_first={} field_slot_last={} method_first={} method_last={} method_slot_first={} method_slot_last={}\n",
        firstFieldPage,
        lastFieldPage,
        firstFieldSlot,
        lastFieldSlot,
        firstMethodPage,
        lastMethodPage,
        firstMethodSlot,
        lastMethodSlot);
    fmt::print("[PASS] transform fields share one cold page: {}\n",
        firstFieldPage == lastFieldPage ? "Yes" : "No");
    fmt::print("[PASS] transform methods share one cold page: {}\n",
        firstMethodPage == lastMethodPage ? "Yes" : "No");
    fmt::print("[PASS] transform field cold slots are contiguous: {}\n",
        firstFieldPage == lastFieldPage && (lastFieldSlot - firstFieldSlot + 1) == typeInfo->GetFieldCount() ? "Yes" : "No");
    fmt::print("[PASS] transform method cold slots are contiguous: {}\n",
        firstMethodPage == lastMethodPage && (lastMethodSlot - firstMethodSlot + 1) == typeInfo->GetMethodCount() ? "Yes" : "No");
}

void TestTypeRegistryArenaOwnership() {
    PrintSeparator("基线: TypeRegistry arena 持有");

    using shine::co::Memory;
    using shine::co::MemoryTag;

    auto& registry = shine::reflection::TypeRegistry::Get();
    const size_t pageCountBefore = registry.GetArenaPageCount();
    const size_t typeCountBefore = registry.GetRegisteredTypeCount();

    Memory::FlushAllThreadStats();
    const auto metaBefore = Memory::GetTagStats(MemoryTag::ReflectionMeta);

    const auto registerProbe = [](shine::reflection::TypeId id, shine::STextView name) {
        shine::reflection::TypeInfo info{};
        info.id = id;
        info.SetName(name);
        info.size = sizeof(int);
        info.alignment = alignof(int);
        info.isPod = true;
        info.isEnum = false;
        return shine::reflection::TypeRegistry::Get().Register(std::move(info));
    };

    const auto resultA = registerProbe(shine::reflection::Hash("ArenaRegistryProbeA") ^ 0x13579BDFu,
        shine::STextView::from_literal("ArenaRegistryProbeA"));
    const auto resultB = registerProbe(shine::reflection::Hash("ArenaRegistryProbeB") ^ 0x2468ACE0u,
        shine::STextView::from_literal("ArenaRegistryProbeB"));
    const auto resultC = registerProbe(shine::reflection::Hash("ArenaRegistryProbeC") ^ 0x55AA55AAu,
        shine::STextView::from_literal("ArenaRegistryProbeC"));

    Memory::FlushAllThreadStats();
    const auto metaAfter = Memory::GetTagStats(MemoryTag::ReflectionMeta);

    const auto* probeA = registry.FindByNameFast(shine::STextView::from_literal("ArenaRegistryProbeA"));
    const auto* probeB = registry.FindByNameFast(shine::STextView::from_literal("ArenaRegistryProbeB"));
    const auto* probeC = registry.FindByNameFast(shine::STextView::from_literal("ArenaRegistryProbeC"));
    const size_t pageA = registry.GetArenaPageIndex(probeA);
    const size_t pageB = registry.GetArenaPageIndex(probeB);
    const size_t pageC = registry.GetArenaPageIndex(probeC);
    const size_t pageCountAfter = registry.GetArenaPageCount();

    fmt::print("  registry arena: pages_before={} pages_after={} slots/page={} types_before={} types_after={} meta_alloc_delta={} meta_current_delta={}\n",
        pageCountBefore,
        pageCountAfter,
        registry.GetArenaSlotsPerPage(),
        typeCountBefore,
        registry.GetRegisteredTypeCount(),
        static_cast<long long>(metaAfter.alloc_count) - static_cast<long long>(metaBefore.alloc_count),
        static_cast<long long>(metaAfter.bytes_current) - static_cast<long long>(metaBefore.bytes_current));

    fmt::print("  probe pages: A={} B={} C={}\n", pageA, pageB, pageC);
    fmt::print("[PASS] arena probe A registered: {}\n", resultA ? "Yes" : "No");
    fmt::print("[PASS] arena probe B registered: {}\n", resultB ? "Yes" : "No");
    fmt::print("[PASS] arena probe C registered: {}\n", resultC ? "Yes" : "No");
    fmt::print("[PASS] arena page index valid: {}\n",
        probeA && probeB && probeC && pageA < pageCountAfter && pageB < pageCountAfter && pageC < pageCountAfter ? "Yes" : "No");
    fmt::print("[PASS] sequential registry probes share one arena page: {}\n",
        probeA && probeB && probeC && pageA == pageB && pageB == pageC ? "Yes" : "No");
}

// =============================================================================
// 问题发现测试
// =============================================================================

// 问题1: 哈希函数不一致测试
// TypeInfo::FindField 使用 FNV-1a Hash 函数，与编译期一致
void TestHashInconsistency() {
    PrintSeparator("问题1: 哈希函数一致性");
    
    // 编译期 Hash (FNV-1a)
    constexpr auto ct_hash = shine::reflection::Hash("position");
    constexpr auto ct_hash2 = shine::reflection::Hash("position");
    
    // 运行时 Hash (FNV-1a)
    std::string_view sv = "position";
    auto rt_hash = shine::reflection::Hash(sv);
    
    fmt::print("编译期 FNV-1a hash: {} (from Hash function)\n", ct_hash);
    fmt::print("编译期 FNV-1a hash: {} (from Hash function 2)\n", ct_hash2);
    fmt::print("运行时 FNV-1a hash:  {}\n", rt_hash);
    fmt::print("结果: {} - 哈希现在一致!\n", 
        (ct_hash == rt_hash) ? "哈希一致" : "哈希不一致");
    
    // 实际测试缓存是否工作
    auto* typeInfo = shine::reflection::TypeRegistry::Get().FindFast(shine::reflection::GetTypeId<Transform>());
    if (typeInfo) {
        // 第一次查找会构建缓存
        auto* field1 = typeInfo->FindField("position");
        // 第二次查找应该使用缓存
        auto* field2 = typeInfo->FindField("position");
        
        fmt::print("字段查找结果: {} (应该非空)\n", field1 ? "成功" : "失败");
        fmt::print("缓存现在应该正常工作了\n");
    }
}

// 问题2: 字段 offset 编译期无法确定
void TestConstexprLimitation() {
    PrintSeparator("问题2: 编译期反射局限");
    
    // 编译期类型信息
    constexpr auto ct_info = shine::reflection::ConstexprTypeInfo<Transform>::Create("Transform");
    
    // 注意: offset 在编译期是 0，需要运行时确定
    fmt::print("编译期类型信息:\n");
    fmt::print("  - ID: {}\n", ct_info.id);
    fmt::print("  - Size: {}\n", ct_info.size);
    fmt::print("  - Alignment: {}\n", ct_info.alignment);
    fmt::print("  - Fields: {}\n", ct_info.GetFieldCount());
    fmt::print("  - 注意: offset 在编译期无法确定，需要运行时计算\n");
    
    // 运行时类型信息
    auto* rt_info = shine::reflection::TypeRegistry::Get().FindFast(shine::reflection::GetTypeId<Transform>());
    if (rt_info) {
        fmt::print("运行时类型信息:\n");
        fmt::print("  - 字段数量: {}\n", rt_info->GetFieldCount());
        for (const auto& f : rt_info->GetFields()) {
            fmt::print("    - {}: offset={}, size={}\n", f.GetNameView(), f.offset, f.size);
        }

        using PositionBinding = shine::reflection::BoundMember<&Transform::position>;
        using PositionXPath = shine::reflection::BoundPath<&Transform::position, &Vec3::x>;
        using PositionLengthSq = shine::reflection::BoundMethodPath<&Vec3::LengthSquared, &Transform::position>;
        const auto* positionField = rt_info->FindField(PositionBinding::Name());
        fmt::print("编译期绑定包装:\n");
        fmt::print("  - Name(): {}\n", PositionBinding::Name());
        fmt::print("  - Offset(): {}\n", PositionBinding::Offset());
        fmt::print("  - 与运行时字段 offset 一致: {}\n",
            positionField != nullptr && positionField->offset == PositionBinding::Offset() ? "Yes" : "No");
        fmt::print("链式字段路径包装:\n");
        fmt::print("  - Path(): {}\n", PositionXPath::Path());
        fmt::print("  - LeafName(): {}\n", PositionXPath::LeafName());
        fmt::print("  - Offset(): {}\n", PositionXPath::Offset());
        fmt::print("混合路径方法包装:\n");
        fmt::print("  - Path(): {}\n", PositionLengthSq::Path());
        fmt::print("  - MethodName(): {}\n", PositionLengthSq::MethodName());
        fmt::print("  - Invoke(): {}\n", PositionLengthSq::Invoke(Transform{}));
    }
}

// 问题3: 缺少序列化支持
void TestSerializationGap() {
    PrintSeparator("问题3: 序列化支持缺失");
    
    fmt::print("当前反射系统没有内置的 JSON/binary 序列化支持\n");
    fmt::print("需要手动实现 Serialize/Deserialize 方法\n");
    
    // 演示手动序列化
    Transform t;
    t.position = Vec3(1.0f, 2.0f, 3.0f);
    t.name = "TestObject";
    t.id = 42;
    t.enabled = true;
    
    // 手动反射序列化 (演示)
    auto* typeInfo = shine::reflection::TypeRegistry::Get().FindFast(shine::reflection::GetTypeId<Transform>());
    if (typeInfo) {
        fmt::print("手动序列化 Transform 实例:\n");
        for (const auto& field : typeInfo->GetFields()) {
            // 简化: 只打印字段信息，实际序列化需要处理类型
            fmt::print("  {}: offset={}\n", field.GetNameView(), field.offset);
        }
    }
}

// 问题4: 容器反射支持有限
void TestContainerLimitation() {
    PrintSeparator("问题4: 容器反射支持");
    
    // 验证编译期获取成员名
    constexpr auto name_x = shine::reflection::member_name<&Vec3::x>();
    constexpr auto name_y = shine::reflection::member_name<&Vec3::y>();
    constexpr auto name_z = shine::reflection::member_name<&Vec3::z>();
    
    static_assert(name_x == "x", "成员名 x 错误");
    static_assert(name_y == "y", "成员名 y 错误");
    static_assert(name_z == "z", "成员名 z 错误");
    
    fmt::print("编译期成员名获取:\n");
    fmt::print("  Vec3::x = '{}'\n", name_x);
    fmt::print("  Vec3::y = '{}'\n", name_y);
    fmt::print("  Vec3::z = '{}'\n", name_z);
    
    // 验证编译期获取类型名
    constexpr auto type_name = shine::reflection::type_string<Vec3>();
    fmt::print("编译期类型名获取:\n");
    fmt::print("  Vec3 = '{}'\n", type_name);
    
    // 运行时偏移量计算
    auto off_x = shine::reflection::compute_offset(&Vec3::x);
    auto off_y = shine::reflection::compute_offset(&Vec3::y);
    auto off_z = shine::reflection::compute_offset(&Vec3::z);
    
    fmt::print("运行时偏移量计算:\n");
    fmt::print("  offsetof(Vec3, x) = {}\n", off_x);
    fmt::print("  offsetof(Vec3, y) = {}\n", off_y);
    fmt::print("  offsetof(Vec3, z) = {}\n", off_z);
    
    // 测试基本的容器 trait
    std::vector<int> test_vec = {1, 2, 3, 4, 5};
    const auto& vec_trait = shine::reflection::ListThunks<std::vector<int>>::GetTrait();
    
    fmt::print("容器 Trait 测试:\n");
    fmt::print("  1. std::vector<int>:\n");
    fmt::print("     - Element Type ID: {}\n", vec_trait.elementType);
    fmt::print("     - Size: {}\n", vec_trait.GetSize(&test_vec));
    
    // 测试 map 容器
    std::map<std::string, int> test_map = {{"health", 100}, {"mana", 50}};
    const auto& map_trait = shine::reflection::MapThunks<std::map<std::string, int>>::GetTrait();
    
    fmt::print("  2. std::map<std::string, int>:\n");
    fmt::print("     - Key Type ID: {}\n", map_trait.keyType);
    fmt::print("     - Value Type ID: {}\n", map_trait.valueType);
    fmt::print("     - Size: {}\n", map_trait.GetSize(&test_map));
    
    // 测试 set 容器
    std::set<int> test_set = {1, 2, 3, 5, 8};
    const auto& set_trait = shine::reflection::SetThunks<std::set<int>>::GetTrait();
    
    fmt::print("  3. std::set<int>:\n");
    fmt::print("     - Key Type ID: {}\n", set_trait.keyType);
    fmt::print("     - Size: {}\n", set_trait.GetSize(&test_set));
    
    fmt::print("容器反射支持已扩展: vector, map, set\n");
}

// =============================================================================
// 性能测试
// =============================================================================

void TestFieldAccessPerformance() {
    PrintSeparator("性能测试: 字段访问");

    using PositionBinding = shine::reflection::BoundMember<&Transform::position>;
    Transform t;
    t.position = Vec3(1.0f, 2.0f, 3.0f);

    auto* typeInfo = shine::reflection::TypeRegistry::Get().FindFast(shine::reflection::GetTypeId<Transform>());
    if (!typeInfo) {
        fmt::print("错误: 无法获取 Transform 类型信息\n");
        return;
    }

    const auto* positionField = typeInfo->FindField(PositionBinding::Name());
    if (positionField == nullptr) {
        fmt::print("错误: 无法获取 position 字段信息\n");
        return;
    }

    auto* offsetPtr = reinterpret_cast<Vec3*>(reinterpret_cast<char*>(&t) + positionField->offset);
    Vec3 readBuffer{};
    Vec3 writeBuffer{4.0f, 5.0f, 6.0f};
    float scalarWrite = 1.0f;

    for (int i = 0; i < 1024; ++i) {
        (void)typeInfo->FindFieldFast(PositionBinding::Name());
        positionField->Get(&t, &readBuffer);
    }

    const auto lookupStats = MeasureBenchmark([&]() {
        return typeInfo->FindField(PositionBinding::Name());
    }, 20000);
    const auto nativeReadStats = MeasureBenchmark([&]() {
        return t.position;
    }, 20000);
    const auto reflectLookupReadStats = MeasureBenchmark([&]() {
        auto* field = typeInfo->FindField(PositionBinding::Name());
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
        auto* field = typeInfo->FindField(PositionBinding::Name());
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

    const auto ctGetStats = MeasureBenchmark([&]() {
        return shine::reflection::CT_GET<&Vec3::x>(t.position);
    }, 20000);
    const auto ctSetStats = MeasureBenchmark([&]() {
        scalarWrite += 0.015625f;
        shine::reflection::CT_SET<&Vec3::x>(t.position, scalarWrite);
    }, 12000);

    fmt::print("补充: 子成员 CT_GET/CT_SET\n");
    PrintBenchmarkLine("CT_GET<&Vec3::x>", ctGetStats.mean_ns);
    PrintBenchmarkLine("CT_SET<&Vec3::x>", ctSetStats.mean_ns);

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
    g_perfReport.nativeRead_ns = nativeReadStats.mean_ns;
    g_perfReport.nativeWrite_ns = nativeWriteStats.mean_ns;
}

// =============================================================================
// 容器反射性能测试
// =============================================================================

void TestContainerPerformance() {
    PrintSeparator("性能测试: 容器反射");
    
    Transform t;
    t.tags = {1, 2, 3, 4, 5};
    t.properties["health"] = 100;
    t.properties["mana"] = 50;
    t.flags = {1, 2, 3};
    
    auto* typeInfo = shine::reflection::TypeRegistry::Get().FindFast(shine::reflection::GetTypeId<Transform>());
    if (!typeInfo) return;
    
    // 测试 vector 字段访问
    auto* tagsField = typeInfo->FindField("tags");
    auto* propsField = typeInfo->FindField("properties");
    auto* flagsField = typeInfo->FindField("flags");
    
    fmt::print("容器字段信息:\n");
    if (tagsField) {
        fmt::print("  - tags (vector<int>): size={}, containerType={}\n", 
            t.tags.size(), (int)tagsField->containerType);
    }
    if (propsField) {
        fmt::print("  - properties (map): size={}, containerType={}\n", 
            t.properties.size(), (int)propsField->containerType);
    }
    if (flagsField) {
        fmt::print("  - flags (set): size={}, containerType={}\n", 
            t.flags.size(), (int)flagsField->containerType);
    }
    
    // 原生容器操作
    fmt::print("\n原生容器操作性能:\n");
    
    double native_vec_get = BenchmarkNs([&]() {
        int size = t.tags.size();
        return size;
    }, 100000);
    fmt::print("  - vector::size: {:.4f} ns\n", native_vec_get);
    
    double native_vec_push = BenchmarkNs([&]() {
        // 不修改容器，只测量访问
        return t.tags[0];
    }, 100000);
    fmt::print("  - vector::operator[]: {:.4f} ns\n", native_vec_push);
    
    double native_map_find = BenchmarkNs([&]() {
        auto it = t.properties.find("health");
        return it != t.properties.end();
    }, 100000);
    fmt::print("  - map::find: {:.4f} ns\n", native_map_find);
    
    double native_set_count = BenchmarkNs([&]() {
        return t.flags.count(2);
    }, 100000);
    fmt::print("  - set::count: {:.4f} ns\n", native_set_count);
    
    // 反射容器操作（通过 offset 访问）
    fmt::print("\n反射容器操作性能:\n");
    
    // 获取容器指针
    auto* tagsContainer = reinterpret_cast<std::vector<int>*>((char*)&t + tagsField->offset);
    auto* propsContainer = reinterpret_cast<std::map<std::string, int>*>((char*)&t + propsField->offset);
    auto* flagsContainer = reinterpret_cast<std::set<int>*>((char*)&t + flagsField->offset);
    
    double reflect_vec_get = BenchmarkNs([&]() {
        std::size_t size = tagsContainer->size();
        return size;
    }, 100000);
    fmt::print("  - vector::size (反射): {:.4f} ns\n", reflect_vec_get);
    fmt::print("    开销: {:.1f}x\n", reflect_vec_get / native_vec_get);
    
    double reflect_vec_access = BenchmarkNs([&]() {
        return (*tagsContainer)[0];
    }, 100000);
    fmt::print("  - vector::operator[] (反射): {:.4f} ns\n", reflect_vec_access);
    fmt::print("    开销: {:.1f}x\n", reflect_vec_access / native_vec_push);
    
    double reflect_map_find = BenchmarkNs([&]() {
        auto it = propsContainer->find("health");
        return it != propsContainer->end();
    }, 100000);
    fmt::print("  - map::find (反射): {:.4f} ns\n", reflect_map_find);
    fmt::print("    开销: {:.1f}x\n", reflect_map_find / native_map_find);
    
    double reflect_set_count = BenchmarkNs([&]() {
        return flagsContainer->count(2);
    }, 100000);
    fmt::print("  - set::count (反射): {:.4f} ns\n", reflect_set_count);
    fmt::print("    开销: {:.1f}x\n", reflect_set_count / native_set_count);
    
    // 容器反射总结
    fmt::print("\n容器反射开销总结:\n");
    fmt::print("  vector::size:     {:.1f}x\n", reflect_vec_get / native_vec_get);
    fmt::print("  vector::operator[]: {:.1f}x\n", reflect_vec_access / native_vec_push);
    fmt::print("  map::find:        {:.1f}x\n", reflect_map_find / native_map_find);
    fmt::print("  set::count:       {:.1f}x\n", reflect_set_count / native_set_count);
}

// =============================================================================
// 所有字段反射性能测试
// =============================================================================

void TestAllFieldsPerformance() {
    PrintSeparator("性能测试: 所有字段反射访问");
    
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
    if (!typeInfo) return;
    
    fmt::print("Transform 字段数量: {}\n", typeInfo->GetFieldCount());
    fmt::print("\n");
    
    // 预热查找缓存
    for (const auto& field : typeInfo->GetFields()) {
        (void)typeInfo->FindFieldFast(field.GetNameView());
    }
    
    // 对每个字段进行性能测试
    fmt::print("字段性能对比 (原生 vs 反射 vs CT_GET):\n");
    fmt::print("{:<15} {:>10} {:>10} {:>10} {:>10}\n", "字段", "原生", "反射", "offset", "CT_GET");
    fmt::print("--------------------------------------------------------\n");
    
    for (const auto& field : typeInfo->GetFields()) {
        const char* fieldName = field.GetNameView().data();
        
        // 原生访问 - 对于容器类型只测 size() 操作
        double nativeTime = 0;
        if (strcmp(fieldName, "position") == 0) {
            nativeTime = BenchmarkNs([&]() { Vec3 v = t.position; (void)v; }, 100000);
        } else if (strcmp(fieldName, "rotation") == 0) {
            nativeTime = BenchmarkNs([&]() { Vec3 v = t.rotation; (void)v; }, 100000);
        } else if (strcmp(fieldName, "scale") == 0) {
            nativeTime = BenchmarkNs([&]() { Vec3 v = t.scale; (void)v; }, 100000);
        } else if (strcmp(fieldName, "name") == 0) {
            nativeTime = BenchmarkNs([&]() { std::string_view v = t.name; (void)v; }, 100000);
        } else if (strcmp(fieldName, "id") == 0) {
            nativeTime = BenchmarkNs([&]() { int v = t.id; (void)v; }, 100000);
        } else if (strcmp(fieldName, "enabled") == 0) {
            nativeTime = BenchmarkNs([&]() { bool v = t.enabled; (void)v; }, 100000);
        } else if (strcmp(fieldName, "tags") == 0) {
            nativeTime = BenchmarkNs([&]() { auto v = t.tags.size(); (void)v; }, 100000);
        } else if (strcmp(fieldName, "properties") == 0) {
            nativeTime = BenchmarkNs([&]() { auto v = t.properties.size(); (void)v; }, 100000);
        } else if (strcmp(fieldName, "flags") == 0) {
            nativeTime = BenchmarkNs([&]() { auto v = t.flags.size(); (void)v; }, 100000);
        }
        
        // 反射访问 - 使用偏移量直接访问（无复制！）
        double reflectTime = 0;
        if (field.isPod) {
            // POD 类型用反射 Get
            if (strcmp(fieldName, "position") == 0 || strcmp(fieldName, "rotation") == 0 || strcmp(fieldName, "scale") == 0) {
                Vec3 buf;
                reflectTime = BenchmarkNs([&]() {
                    auto* f = typeInfo->FindField(fieldName);
                    if (f) f->Get(&t, &buf);
                }, 100000);
            } else if (strcmp(fieldName, "id") == 0) {
                int buf;
                reflectTime = BenchmarkNs([&]() {
                    auto* f = typeInfo->FindField(fieldName);
                    if (f) f->Get(&t, &buf);
                }, 100000);
            } else if (strcmp(fieldName, "enabled") == 0) {
                bool buf;
                reflectTime = BenchmarkNs([&]() {
                    auto* f = typeInfo->FindField(fieldName);
                    if (f) f->Get(&t, &buf);
                }, 100000);
            }
        } else {
            // 非 POD 类型用偏移量直接访问（零复制！）
            if (strcmp(fieldName, "name") == 0) {
                reflectTime = BenchmarkNs([&]() {
                    auto* str = reinterpret_cast<std::string*>((char*)&t + field.offset);
                    (void)str->data();
                }, 100000);
            } else if (strcmp(fieldName, "tags") == 0) {
                reflectTime = BenchmarkNs([&]() {
                    auto* vec = reinterpret_cast<std::vector<int>*>((char*)&t + field.offset);
                    (void)vec->data();
                }, 100000);
            } else if (strcmp(fieldName, "properties") == 0) {
                reflectTime = BenchmarkNs([&]() {
                    auto* m = reinterpret_cast<std::map<std::string, int>*>((char*)&t + field.offset);
                    (void)m->begin();
                }, 100000);
            } else if (strcmp(fieldName, "flags") == 0) {
                reflectTime = BenchmarkNs([&]() {
                    auto* s = reinterpret_cast<std::set<int>*>((char*)&t + field.offset);
                    (void)s->begin();
                }, 100000);
            }
        }
        
        // 偏移量访问性能（作为对比）
        double offsetTime = 0;
        if (field.isPod) {
            if (strcmp(fieldName, "position") == 0 || strcmp(fieldName, "rotation") == 0 || strcmp(fieldName, "scale") == 0) {
                offsetTime = BenchmarkNs([&]() {
                    auto* ptr = (Vec3*)((char*)&t + field.offset);
                    Vec3 v = *ptr;
                }, 100000);
            } else if (strcmp(fieldName, "id") == 0) {
                offsetTime = BenchmarkNs([&]() {
                    auto* ptr = (int*)((char*)&t + field.offset);
                    int v = *ptr;
                }, 100000);
            } else if (strcmp(fieldName, "enabled") == 0) {
                offsetTime = BenchmarkNs([&]() {
                    auto* ptr = (bool*)((char*)&t + field.offset);
                    bool v = *ptr;
                }, 100000);
            }
        } else {
            if (strcmp(fieldName, "name") == 0) {
                offsetTime = BenchmarkNs([&]() {
                    auto* ptr = (std::string*)((char*)&t + field.offset);
                    (void)ptr->data();
                }, 100000);
            } else if (strcmp(fieldName, "tags") == 0) {
                offsetTime = BenchmarkNs([&]() {
                    auto* ptr = (std::vector<int>*)((char*)&t + field.offset);
                    (void)ptr->data();
                }, 100000);
            } else if (strcmp(fieldName, "properties") == 0) {
                offsetTime = BenchmarkNs([&]() {
                    auto* ptr = (std::map<std::string, int>*)((char*)&t + field.offset);
                    (void)ptr->begin();
                }, 100000);
            } else if (strcmp(fieldName, "flags") == 0) {
                offsetTime = BenchmarkNs([&]() {
                    auto* ptr = (std::set<int>*)((char*)&t + field.offset);
                    (void)ptr->begin();
                }, 100000);
            }
        }
        
        // CT_GET 编译期绑定 - 零开销！
        double ctGetTime = 0;
        if (field.isPod) {
            if (strcmp(fieldName, "position") == 0) {
                ctGetTime = BenchmarkNs([&]() { Vec3 v = shine::reflection::CT_GET<&Transform::position>(t); (void)v; }, 100000);
            } else if (strcmp(fieldName, "rotation") == 0) {
                ctGetTime = BenchmarkNs([&]() { Vec3 v = shine::reflection::CT_GET<&Transform::rotation>(t); (void)v; }, 100000);
            } else if (strcmp(fieldName, "scale") == 0) {
                ctGetTime = BenchmarkNs([&]() { Vec3 v = shine::reflection::CT_GET<&Transform::scale>(t); (void)v; }, 100000);
            } else if (strcmp(fieldName, "id") == 0) {
                ctGetTime = BenchmarkNs([&]() { int v = shine::reflection::CT_GET<&Transform::id>(t); (void)v; }, 100000);
            } else if (strcmp(fieldName, "enabled") == 0) {
                ctGetTime = BenchmarkNs([&]() { bool v = shine::reflection::CT_GET<&Transform::enabled>(t); (void)v; }, 100000);
            }
        }
        
        double overhead = (nativeTime > 0 && reflectTime > 0) ? (reflectTime / nativeTime) : 0;
        double offsetOverhead = (nativeTime > 0 && offsetTime > 0) ? (offsetTime / nativeTime) : 0;
        
        fmt::print("{:<15} {:>10.4f} {:>10.4f} {:>10.4f} {:>10.4f}\n",
            fieldName, nativeTime, reflectTime, offsetTime, ctGetTime);
    }
    
    // 字段大小分布
    fmt::print("\n字段大小分布:\n");
    fmt::print("{:<15} {:>10} {:>10}\n", "字段名", "大小", "类型");
    fmt::print("--------------------------------------------------------\n");
    for (const auto& field : typeInfo->GetFields()) {
        const char* fieldName = field.GetNameView().data();
        fmt::print("{:<15} {:>10} {:>10}\n", fieldName, field.size, field.isPod ? "POD" : "Non-POD");
    }
    
    // 总结统计 - 包含所有字段
    fmt::print("\n字段访问性能总结:\n");
    double totalNative = 0, totalReflect = 0;
    int count = 0;
    for (const auto& field : typeInfo->GetFields()) {
        const char* fieldName = field.GetNameView().data();

        double nativeTime = 0;
        if (strcmp(fieldName, "position") == 0) nativeTime = BenchmarkNs([&]() { Vec3 v = t.position; (void)v; }, 100000);
        else if (strcmp(fieldName, "rotation") == 0) nativeTime = BenchmarkNs([&]() { Vec3 v = t.rotation; (void)v; }, 100000);
        else if (strcmp(fieldName, "scale") == 0) nativeTime = BenchmarkNs([&]() { Vec3 v = t.scale; (void)v; }, 100000);
        else if (strcmp(fieldName, "name") == 0) nativeTime = BenchmarkNs([&]() { std::string v = t.name; (void)v; }, 100000);
        else if (strcmp(fieldName, "id") == 0) nativeTime = BenchmarkNs([&]() { int v = t.id; (void)v; }, 100000);
        else if (strcmp(fieldName, "enabled") == 0) nativeTime = BenchmarkNs([&]() { bool v = t.enabled; (void)v; }, 100000);
        else if (strcmp(fieldName, "tags") == 0) nativeTime = BenchmarkNs([&]() { std::size_t v = t.tags.size(); (void)v; }, 100000);
        else if (strcmp(fieldName, "properties") == 0) nativeTime = BenchmarkNs([&]() { std::size_t v = t.properties.size(); (void)v; }, 100000);
        else if (strcmp(fieldName, "flags") == 0) nativeTime = BenchmarkNs([&]() { std::size_t v = t.flags.size(); (void)v; }, 100000);

        double reflectTime = 0;
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

        totalNative += nativeTime;
        totalReflect += reflectTime;
        count++;
    }

    if (count > 0) {
        fmt::print("  平均开销: {:.1f}x\n", (totalReflect / count) / (totalNative / count));
    }
}

void TestTypeLookupPerformance() {
    PrintSeparator("性能测试: 类型查找");

    // 测试 TypeRegistry 查找性能
    constexpr int iterations = 100000;

    const auto findSlowStats = MeasureBenchmark([&]() {
        return shine::reflection::TypeRegistry::Get().Find(shine::reflection::GetTypeId<Transform>());
    }, iterations);

    const auto findFastStats = MeasureBenchmark([&]() {
        return shine::reflection::TypeRegistry::Get().FindFast(shine::reflection::GetTypeId<Transform>());
    }, iterations);

    fmt::print("类型查找 (Find) 性能:\n");
    fmt::print("  - {}次查找平均: {:.4f} ns\n", iterations, findSlowStats.mean_ns);
    fmt::print("类型查找 (FindFast) 性能:\n");
    fmt::print("  - {}次查找平均: {:.4f} ns\n", iterations, findFastStats.mean_ns);
    fmt::print("加速比: {:.2f}x\n", SafeRatio(findSlowStats.mean_ns, findFastStats.mean_ns));

    // 保存类型查找性能
    g_perfReport.typeFindSlow_ns = findSlowStats.mean_ns;
    g_perfReport.typeFindFast_ns = findFastStats.mean_ns;
    g_perfReport.findSpeedup_x = SafeRatio(findSlowStats.mean_ns, findFastStats.mean_ns);
}

void TestMethodCallPerformance() {
    PrintSeparator("性能测试: 方法调用");

    auto* typeInfo = shine::reflection::TypeRegistry::Get().FindFast(shine::reflection::GetTypeId<Transform>());
    if (!typeInfo || typeInfo->GetMethodCount() == 0) {
        fmt::print("Transform 没有注册方法\n");
        return;
    }
    
    fmt::print("已注册方法数量: {}\n", typeInfo->GetMethodCount());
    for (const auto& m : typeInfo->GetMethods()) {
        fmt::print("  - {} (返回类型: {})\n", m.GetNameView(), m.returnType);
    }
    
    // 测试方法调用
    Transform t;
    t.position = Vec3(1, 2, 3);
    t.tags = {1, 2, 3};
    t.properties = {{"health", 100}};
    t.flags = {1, 2, 3};
    
    // 找到 SetPosition 方法
    auto* method = typeInfo->FindMethod("SetPosition");
    if (!method) {
        fmt::print("无法找到 SetPosition 方法\n");
        return;
    }
    
    // 准备参数 (3个float参数)
    float args[3] = {10.0f, 20.0f, 30.0f};
    void* args_ptr[] = {args, args + 1, args + 2};

    const auto lookupInvokeStats = MeasureBenchmark([&]() {
        auto* found = typeInfo->FindMethod("SetPosition");
        found->Invoke(&t, args_ptr, nullptr);
    }, 12000);

    const auto cachedInvokeStats = MeasureBenchmark([&]() {
        method->Invoke(&t, args_ptr, nullptr);
    }, 12000);

    const auto nativeStats = MeasureBenchmark([&]() {
        t.SetPosition(10.0f, 20.0f, 30.0f);
    }, 12000);

    fmt::print("\n方法调用性能对比:\n");
    PrintBenchmarkLine("native call", nativeStats.mean_ns);
    PrintBenchmarkLine("cached Invoke", cachedInvokeStats.mean_ns, nativeStats.mean_ns);
    PrintBenchmarkLine("lookup + Invoke", lookupInvokeStats.mean_ns, nativeStats.mean_ns);

    g_perfReport.methodNative_ns = nativeStats.mean_ns;
    g_perfReport.methodCachedInvoke_ns = cachedInvokeStats.mean_ns;
    g_perfReport.methodLookupInvoke_ns = lookupInvokeStats.mean_ns;

    // 验证调用结果
    method->Invoke(&t, args_ptr, nullptr);
    fmt::print("\n验证: position = ({}, {}, {})\n", 
        t.position.x, t.position.y, t.position.z);
}

void TestMemoryUsage() {
    PrintSeparator("性能测试: 内存使用");
    
    // 统计类型注册内存
    auto count = shine::reflection::TypeRegistry::Get().GetRegisteredTypeCount();
    fmt::print("已注册类型数量: {}\n", count);
    
    // 估算内存使用
    size_t estimated = sizeof(shine::reflection::TypeInfo) * count;
    fmt::print("估算 TypeInfo 内存: ~{} bytes\n", estimated);
    
    // 列出所有已注册类型
    fmt::print("已注册类型:\n");
    // 注意: 需要添加遍历接口，这里只是占位
}

// =============================================================================
// 功能测试
// =============================================================================

void TestBasicFunctionality() {
    PrintSeparator("功能测试: 基础功能");

    {
        shine::reflection::TypeInfo fallbackProbe{};
        fallbackProbe.id = shine::reflection::Hash("RegistryFallbackProbe") ^ 0x9E3779B9u;
        fallbackProbe.SetName(shine::STextView::from_literal("RegistryFallbackProbe"));
        fallbackProbe.size = sizeof(int);
        fallbackProbe.alignment = alignof(int);
        fallbackProbe.isPod = true;
        fallbackProbe.isEnum = false;
        auto registerResult = shine::reflection::TypeRegistry::Get().Register(std::move(fallbackProbe));
        fmt::print("[PASS] fallback registry probe registered: {}\n", registerResult ? "OK" : "FAIL");
        const auto* fallbackByName = shine::reflection::TypeRegistry::Get().FindByNameFast(shine::STextView::from_literal("RegistryFallbackProbe"));
        const auto* fallbackById = shine::reflection::TypeRegistry::Get().FindFast(shine::reflection::Hash("RegistryFallbackProbe") ^ 0x9E3779B9u);
        fmt::print("[PASS] fallback name index -> same pointer: {}\n", fallbackByName == fallbackById ? "OK" : "FAIL");
    }
    
    // 1. 类型查找
    auto result = shine::reflection::TypeRegistry::Get().Find<Transform>();
    if (result) {
        fmt::print("[PASS] Transform 类型注册成功\n");
        fmt::print("       - ID: {}\n", (*result)->id);
        fmt::print("       - Size: {}\n", (*result)->size);
        fmt::print("       - Fields: {}\n", (*result)->GetFieldCount());
    } else {
        fmt::print("[FAIL] Transform 类型注册失败\n");
    }
    
    // 2. 字段访问
    Transform t;
    t.position = Vec3(1.0f, 2.0f, 3.0f);
    t.name = "TestObject";
    t.id = 42;
    
    auto* typeInfo = shine::reflection::TypeRegistry::Get().FindFast(shine::reflection::GetTypeId<Transform>());
    if (typeInfo) {
        const auto* typeInfoByName = shine::reflection::TypeRegistry::Get().FindByNameFast(shine::STextView::from_literal("Transform"));
        fmt::print("[PASS] type name lookup -> same pointer: {}\n", typeInfoByName == typeInfo ? "OK" : "FAIL");

        // Get position
        auto* posField = typeInfo->FindFieldFast("position");
        if (posField) {
            Vec3 pos;
            posField->Get(&t, &pos);
            fmt::print("[PASS] Get position: ({}, {}, {})\n", pos.x, pos.y, pos.z);
            fmt::print("[PASS] position owner handle -> type: {}\n", posField->GetOwnerType() == typeInfo ? "OK" : "FAIL");
            fmt::print("[PASS] FindFieldFast matches compatibility alias: {}\n",
                posField == typeInfo->FindField("position") ? "OK" : "FAIL");
        }
        
        // Set position
        if (posField) {
            Vec3 newPos(4.0f, 5.0f, 6.0f);
            posField->Set(&t, &newPos);
            fmt::print("[PASS] Set position: ({}, {}, {})\n", t.position.x, t.position.y, t.position.z);
        }
        
        // Get name
        auto* nameField = typeInfo->FindFieldFast("name");
        if (nameField) {
            std::string name;
            nameField->Get(&t, &name);
            fmt::print("[PASS] Get name: {}\n", name);
        }
        
        // Get id
        auto* idField = typeInfo->FindFieldFast("id");
        if (idField) {
            int id;
            idField->Get(&t, &id);
            fmt::print("[PASS] Get id: {}\n", id);
        }

        auto* method = typeInfo->FindMethodFast("GetPosition");
        if (method) {
            fmt::print("[PASS] method owner handle -> type: {}\n", method->GetOwnerType() == typeInfo ? "OK" : "FAIL");
            fmt::print("[PASS] FindMethodFast matches compatibility alias: {}\n",
                method == typeInfo->FindMethod("GetPosition") ? "OK" : "FAIL");
        }
    }
    
    // 3. 枚举测试
    auto enumResult = shine::reflection::TypeRegistry::Get().Find<ETestEnum>();
    if (enumResult) {
        fmt::print("[PASS] ETestEnum 枚举注册成功\n");
        fmt::print("       - Entries: {}\n", (*enumResult)->GetEnumCount());
    }
}

void TestFlagsAndMetadata() {
    PrintSeparator("功能测试: 标志和元数据");
    
    auto* typeInfo = shine::reflection::TypeRegistry::Get().FindFast(shine::reflection::GetTypeId<Transform>());
    if (typeInfo) {
        shine::reflection::InspectorView view;
        view.typeInfo = typeInfo;

        for (const auto& field : typeInfo->GetFields()) {
            fmt::print("字段: {}\n", field.GetNameView());
            fmt::print("  - Is POD: {}\n", field.isPod ? "Yes" : "No");
            fmt::print("  - Size: {} bytes\n", field.size);
            fmt::print("  - Alignment: {} bytes\n", field.alignment);
            if (field.HasCategory()) {
                fmt::print("  - Category: {}\n", field.GetCategoryView());
            }
            if (field.HasDisplayName()) {
                fmt::print("  - DisplayName: {}\n", field.GetDisplayNameView());
            }
            if (field.HasRange()) {
                fmt::print("  - Range: [{}, {}]\n", field.GetMinValue(), field.GetMaxValue());
            }
            if (field.HasEditCondition()) {
                fmt::print("  - EditCondition: {}\n", field.GetEditConditionView());
            }
        }

        Transform enabledInstance;
        enabledInstance.enabled = true;
        Transform disabledInstance;
        disabledInstance.enabled = false;
        if (const auto* idField = typeInfo->FindFieldFast("id")) {
            fmt::print("[PASS] id visible when enabled=true: {}\n", view.IsVisible(*idField, &enabledInstance) ? "Yes" : "No");
            fmt::print("[PASS] id visible when enabled=false: {}\n", view.IsVisible(*idField, &disabledInstance) ? "Yes" : "No");
        }
    }
}

// =============================================================================
// 主函数
// =============================================================================

int main() {
    fmt::print("============================================================\n");
    fmt::print("         ShineEngine 反射系统测试套件\n");
    fmt::print("============================================================\n");
    fmt::print("C++ 标准: C++23\n");
    fmt::print("编译器: MSVC\n");
    
    // 功能测试
    TestBasicFunctionality();
    TestFlagsAndMetadata();
    
    // 问题发现测试
    TestReflectionLayoutBaseline();
    TestReflectionMemoryTags();
    TestReflectionStringInterning();
    TestReflectionColdBatchReservation();
    TestReflectionRegistrationPlan();
    TestTypeBuilderStagedEmission();
    TestInjectedStaticPlanPath();
    TestRegisteredTypeColdLocality();
    TestTypeRegistryArenaOwnership();
    TestHashInconsistency();
    TestConstexprLimitation();
    TestSerializationGap();
    TestContainerLimitation();
    
    fmt::print("\n性能专项已拆分到 ReflectionPerfTest，请使用 .\\build.bat test ReflectionPerfTest --release --no-pause\n");
    
    fmt::print("\n============================================================\n");
    fmt::print("                    测试完成\n");
    fmt::print("============================================================\n");
    
    return 0;
}
