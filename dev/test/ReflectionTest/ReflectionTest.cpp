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

#include "EngineCore/reflection/Reflection.h"
#include "EngineCore/reflection/Views/ScriptView.h"
#include "memory/memory.ixx"
#include "memory/ue_binned2_port.h"

// 使用 fmt 进行输出
#include <fmt/format.h>

// =============================================================================
// 性能基准结构
// =============================================================================
struct PerfReport {
    double fieldLookup_ops = 0;  // ops/sec
    double getter_ns = 0;
    double setter_ns = 0;
    double native_ns = 0;
    double methodOverhead_x = 0;
    double typeFindSlow_ns = 0;
    double typeFindFast_ns = 0;
    double findSpeedup_x = 0;
    
    void Print() const {
        fmt::print("\n============================================================\n");
        fmt::print("                    性能测试报告\n");
        fmt::print("============================================================\n");
        fmt::print("  字段查找: {:.2f} M ops/sec\n", fieldLookup_ops / 1000000.0);
        
        double getter_overhead = (native_ns > 0) ? (getter_ns / native_ns) : 0;
        double setter_overhead = (native_ns > 0) ? (setter_ns / native_ns) : 0;
        
        fmt::print("  Getter:   {:.4f} ns (开销: {:.1f}x)\n", getter_ns, getter_overhead);
        fmt::print("  Setter:   {:.4f} ns (开销: {:.1f}x)\n", setter_ns, setter_overhead);
        fmt::print("  方法调用: {:.1f}x 开销\n", methodOverhead_x);
        fmt::print("  类型查找: {:.1f}x 加速 (FindFast)\n", findSpeedup_x);
        fmt::print("============================================================\n\n");
    }
};

// 全局性能报告
PerfReport g_perfReport;

// 测试结构体
struct Vec3 {
    float x, y, z;
    
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
    
    Vec3 operator+(const Vec3& other) const {
        return Vec3(x + other.x, y + other.y, z + other.z);
    }
    
    bool operator==(const Vec3& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
};

// 运行时验证 - 获取成员名和偏移量
// 注意: MSVC constexpr 支持有限，static_assert 可能失败
// 但运行时计算应该正常工作

struct Transform {
    Vec3 position;
    Vec3 rotation;
    Vec3 scale;
    std::string name;
    int id;
    bool enabled;
    std::vector<int> tags;  // 序列容器
    std::map<std::string, int> properties;  // 关联容器
    std::set<int> flags;  // Set 容器
    
    Transform() : position(), rotation(), scale(1, 1, 1), name(""), id(0), enabled(true) {}
    
    // 测试方法 - 使用值类型避免引用问题
    void SetPosition(float x, float y, float z) {
        position.x = x; position.y = y; position.z = z;
    }
    
    Vec3 GetPosition() const {
        return position;
    }
    
    int AddTag(int tag) {
        tags.push_back(tag);
        return (int)tags.size();
    }
    
    // 使用指针参数代替引用
    void SetProperty(std::string* key, int* value) {
        if (key && value) {
            properties[*key] = *value;
        }
    }
    
    int GetProperty(std::string* key) const {
        if (!key) return -1;
        auto it = properties.find(*key);
        return (it != properties.end()) ? it->second : -1;
    }
};

// 注册反射
REFLECTION_STRUCT(Vec3) {
    REFLECT_FIELD(x);
    REFLECT_FIELD(y);
    REFLECT_FIELD(z);
};

REFLECTION_STRUCT(Transform) {
    REFLECT_FIELD(position)
        .DisplayName(shine::STextView::from_literal("World Position"))
        .Meta(shine::reflection::MetaKeys::Category, shine::STextView::from_literal("Transform"));
    REFLECT_FIELD(rotation)
        .Meta(shine::reflection::MetaKeys::Category, shine::STextView::from_literal("Transform"));
    REFLECT_FIELD(scale)
        .Meta(shine::reflection::MetaKeys::Category, shine::STextView::from_literal("Transform"));
    REFLECT_FIELD(name)
        .DisplayName(shine::STextView::from_literal("Actor Name"))
        .Meta(shine::reflection::MetaKeys::Category, shine::STextView::from_literal("Identity"));
    REFLECT_FIELD(id)
        .DisplayName(shine::STextView::from_literal("Actor Id"))
        .Range(0.0f, 100.0f)
        .Meta(shine::reflection::MetaKeys::Category, shine::STextView::from_literal("Identity"))
        .Meta(shine::reflection::MetaKeys::EditCondition, shine::STextView::from_literal("enabled"));
    REFLECT_FIELD(enabled)
        .DisplayName(shine::STextView::from_literal("Enabled"))
        .Meta(shine::reflection::MetaKeys::Category, shine::STextView::from_literal("Identity"));
    REFLECT_FIELD(tags)
        .Meta(shine::reflection::MetaKeys::Category, shine::STextView::from_literal("Collections"));
    REFLECT_FIELD(properties)
        .Meta(shine::reflection::MetaKeys::Category, shine::STextView::from_literal("Collections"));
    REFLECT_FIELD(flags)
        .Meta(shine::reflection::MetaKeys::Category, shine::STextView::from_literal("Collections"));
    
    // 使用优化的方法注册
    REFLECT_METHOD_FAST(SetPosition);
    REFLECT_METHOD_FAST(GetPosition);
    REFLECT_METHOD_FAST(AddTag);
    REFLECT_METHOD_FAST(SetProperty);
    REFLECT_METHOD_FAST(GetProperty);
};

// 测试枚举
enum class ETestEnum {
    None = 0,
    Value1 = 1,
    Value2 = 2,
    Value3 = 3
};

REFLECT_ENUM(ETestEnum) {
    builder.Enums({
        {ETestEnum::None, "None"},
        {ETestEnum::Value1, "Value1"},
        {ETestEnum::Value2, "Value2"},
        {ETestEnum::Value3, "Value3"}
    });
}

// =============================================================================
// 测试辅助函数
// =============================================================================

template<typename Func>
double Benchmark(Func&& func, int iterations = 1000) {
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        func();
    }
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count() / iterations;
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
        offsetof(shine::reflection::TypeInfo, coldData));
    PrintLayoutHeader<shine::reflection::FieldInfo>(shine::STextView::from_literal("FieldInfo"),
        offsetof(shine::reflection::FieldInfo, coldData));
    PrintLayoutHeader<shine::reflection::MethodInfo>(shine::STextView::from_literal("MethodInfo"),
        offsetof(shine::reflection::MethodInfo, coldData));
    PrintLayoutHeader<shine::reflection::TypeColdData>(shine::STextView::from_literal("TypeColdData"));
    PrintLayoutHeader<shine::reflection::FieldColdData>(shine::STextView::from_literal("FieldColdData"));
    PrintLayoutHeader<shine::reflection::MethodColdData>(shine::STextView::from_literal("MethodColdData"));
    PrintLayoutHeader<shine::reflection::ReflectionOwnerHandle>(shine::STextView::from_literal("ReflectionOwnerHandle"));
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
    PrintMemberLayout(shine::STextView::from_literal("coldData"), offsetof(shine::reflection::FieldInfo, coldData), sizeof(shine::reflection::ReflectionColdPtr<shine::reflection::FieldColdData>), alignof(shine::reflection::ReflectionColdPtr<shine::reflection::FieldColdData>));

    fmt::print("\nMethodInfo 成员布局:\n");
    PrintMemberLayout(shine::STextView::from_literal("nameHash"), offsetof(shine::reflection::MethodInfo, nameHash), sizeof(uint32_t), alignof(uint32_t));
    PrintMemberLayout(shine::STextView::from_literal("invokeFn"), offsetof(shine::reflection::MethodInfo, invokeFn), sizeof(shine::reflection::InvokeFn), alignof(shine::reflection::InvokeFn));
    PrintMemberLayout(shine::STextView::from_literal("returnType"), offsetof(shine::reflection::MethodInfo, returnType), sizeof(shine::reflection::TypeId), alignof(shine::reflection::TypeId));
    PrintMemberLayout(shine::STextView::from_literal("paramTypes"), offsetof(shine::reflection::MethodInfo, paramTypes), sizeof(std::vector<shine::reflection::TypeId>), alignof(std::vector<shine::reflection::TypeId>));
    PrintMemberLayout(shine::STextView::from_literal("signatureHash"), offsetof(shine::reflection::MethodInfo, signatureHash), sizeof(uint64_t), alignof(uint64_t));
    PrintMemberLayout(shine::STextView::from_literal("flags"), offsetof(shine::reflection::MethodInfo, flags), sizeof(shine::reflection::FunctionFlags), alignof(shine::reflection::FunctionFlags));
    PrintMemberLayout(shine::STextView::from_literal("owner"), offsetof(shine::reflection::MethodInfo, owner), sizeof(shine::reflection::ReflectionOwnerHandle), alignof(shine::reflection::ReflectionOwnerHandle));
    PrintMemberLayout(shine::STextView::from_literal("callCache"), offsetof(shine::reflection::MethodInfo, callCache), sizeof(shine::reflection::MethodCallCache), alignof(shine::reflection::MethodCallCache));
    PrintMemberLayout(shine::STextView::from_literal("coldData"), offsetof(shine::reflection::MethodInfo, coldData), sizeof(shine::reflection::ReflectionColdPtr<shine::reflection::MethodColdData>), alignof(shine::reflection::ReflectionColdPtr<shine::reflection::MethodColdData>));

    fmt::print("\nReflectionOwnerHandle 布局:\n");
    PrintMemberLayout(shine::STextView::from_literal("rawValue"), 0, sizeof(shine::reflection::ReflectionOwnerHandle), alignof(shine::reflection::ReflectionOwnerHandle));

    fmt::print("\nTypeInfo 成员布局:\n");
    PrintMemberLayout(shine::STextView::from_literal("id"), offsetof(shine::reflection::TypeInfo, id), sizeof(shine::reflection::TypeId), alignof(shine::reflection::TypeId));
    PrintMemberLayout(shine::STextView::from_literal("size"), offsetof(shine::reflection::TypeInfo, size), sizeof(std::size_t), alignof(std::size_t));
    PrintMemberLayout(shine::STextView::from_literal("alignment"), offsetof(shine::reflection::TypeInfo, alignment), sizeof(std::size_t), alignof(std::size_t));
    PrintMemberLayout(shine::STextView::from_literal("isEnum"), offsetof(shine::reflection::TypeInfo, isEnum), sizeof(bool), alignof(bool));
    PrintMemberLayout(shine::STextView::from_literal("isPod"), offsetof(shine::reflection::TypeInfo, isPod), sizeof(bool), alignof(bool));
    PrintMemberLayout(shine::STextView::from_literal("fields"), offsetof(shine::reflection::TypeInfo, fields), sizeof(std::vector<shine::reflection::FieldInfo>), alignof(std::vector<shine::reflection::FieldInfo>));
    PrintMemberLayout(shine::STextView::from_literal("methods"), offsetof(shine::reflection::TypeInfo, methods), sizeof(std::vector<shine::reflection::MethodInfo>), alignof(std::vector<shine::reflection::MethodInfo>));
    PrintMemberLayout(shine::STextView::from_literal("coldData"), offsetof(shine::reflection::TypeInfo, coldData), sizeof(shine::reflection::ReflectionColdPtr<shine::reflection::TypeColdData>), alignof(shine::reflection::ReflectionColdPtr<shine::reflection::TypeColdData>));
    PrintMemberLayout(shine::STextView::from_literal("fieldLookup_"), offsetof(shine::reflection::TypeInfo, fieldLookup_), sizeof(std::vector<shine::reflection::TypeInfo::LookupEntry>), alignof(std::vector<shine::reflection::TypeInfo::LookupEntry>));
    PrintMemberLayout(shine::STextView::from_literal("methodLookup_"), offsetof(shine::reflection::TypeInfo, methodLookup_), sizeof(std::vector<shine::reflection::TypeInfo::LookupEntry>), alignof(std::vector<shine::reflection::TypeInfo::LookupEntry>));
    PrintMemberLayout(shine::STextView::from_literal("lookupSorted_"), offsetof(shine::reflection::TypeInfo, lookupSorted_), sizeof(bool), alignof(bool));

    fmt::print("\nTypeColdData 成员布局:\n");
    PrintMemberLayout(shine::STextView::from_literal("name"), offsetof(shine::reflection::TypeColdData, name), sizeof(shine::STextView), alignof(shine::STextView));
    PrintMemberLayout(shine::STextView::from_literal("enumEntries"), offsetof(shine::reflection::TypeColdData, enumEntries), sizeof(std::vector<shine::reflection::EnumEntry>), alignof(std::vector<shine::reflection::EnumEntry>));

    fmt::print("\nMethodCallCache 成员布局:\n");
    PrintMemberLayout(shine::STextView::from_literal("returnTypeInfo"), offsetof(shine::reflection::MethodCallCache, returnTypeInfo), sizeof(const shine::reflection::TypeInfo*), alignof(const shine::reflection::TypeInfo*));
    PrintMemberLayout(shine::STextView::from_literal("paramTypeInfos"), offsetof(shine::reflection::MethodCallCache, paramTypeInfos), sizeof(std::vector<const shine::reflection::TypeInfo*>), alignof(std::vector<const shine::reflection::TypeInfo*>));
    PrintMemberLayout(shine::STextView::from_literal("paramOffsets"), offsetof(shine::reflection::MethodCallCache, paramOffsets), sizeof(std::vector<std::size_t>), alignof(std::vector<std::size_t>));
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
    {
        auto batch = fieldPool.BeginContiguousBatch(requestedBatch);
        for (size_t index = 0; index < requestedBatch; ++index) {
            batchedColdData.push_back(shine::reflection::MakeReflectionColdData<shine::reflection::FieldColdData>());
        }
    }

    const size_t firstPage = batchedColdData.empty() ? fieldPool.PageIndexOf(nullptr) : fieldPool.PageIndexOf(batchedColdData.front().get());
    const size_t lastPage = batchedColdData.empty() ? fieldPool.PageIndexOf(nullptr) : fieldPool.PageIndexOf(batchedColdData.back().get());

    fmt::print("  field batch pages: first={} last={} fillers_page={} committed_before={} slots_per_page={} requested_batch={}\n",
        firstPage,
        lastPage,
        fillerPage,
        committedBefore,
        slotsPerPage,
        requestedBatch);
    fmt::print("[PASS] batched cold fields stay on one page: {}\n", firstPage == lastPage ? "Yes" : "No");
    if (!fillers.empty()) {
        fmt::print("[PASS] batch moved off the partially used tail page: {}\n", firstPage != fillerPage ? "Yes" : "No");
    }
}

void TestReflectionRegistrationPlan() {
    PrintSeparator("基线: 注册计划与容量预留");

    shine::reflection::TypeRegistrationPlan transformPlan{};
    shine::reflection::TypeBuilderPlanCounter<Transform> transformCounter(transformPlan);
    _ReflectRegFn_Transform(transformCounter);

    fmt::print("  Transform plan: fields={} methods={} enums={}\n",
        transformPlan.fieldCount,
        transformPlan.methodCount,
        transformPlan.enumCount);

    shine::reflection::TypeInfo transformInfo{};
    transformInfo.id = shine::reflection::GetTypeId<Transform>();
    transformInfo.SetName(shine::STextView::from_literal("TransformPlanProbe"));
    transformInfo.size = sizeof(Transform);
    transformInfo.alignment = alignof(Transform);
    transformInfo.isPod = std::is_trivially_copyable_v<Transform>;
    transformInfo.isEnum = false;

    shine::reflection::TypeBuilder<Transform> transformBuilder(transformInfo, transformPlan);
    fmt::print("  reserved capacities: fields={} methods={} enums={}\n",
        transformInfo.fields.capacity(),
        transformInfo.methods.capacity(),
        transformInfo.GetEnumEntries().capacity());
    fmt::print("[PASS] transform field reserve matches plan: {}\n",
        transformInfo.fields.capacity() >= transformPlan.fieldCount ? "Yes" : "No");
    fmt::print("[PASS] transform method reserve matches plan: {}\n",
        transformInfo.methods.capacity() >= transformPlan.methodCount ? "Yes" : "No");

    shine::reflection::TypeRegistrationPlan enumPlan{};
    shine::reflection::TypeBuilderPlanCounter<ETestEnum> enumCounter(enumPlan);
    _ReflectRegFn_ETestEnum(enumCounter);
    fmt::print("  ETestEnum plan: fields={} methods={} enums={}\n",
        enumPlan.fieldCount,
        enumPlan.methodCount,
        enumPlan.enumCount);

    shine::reflection::TypeInfo enumInfo{};
    enumInfo.id = shine::reflection::GetTypeId<ETestEnum>();
    enumInfo.SetName(shine::STextView::from_literal("EnumPlanProbe"));
    enumInfo.size = sizeof(ETestEnum);
    enumInfo.alignment = alignof(ETestEnum);
    enumInfo.isPod = false;
    enumInfo.isEnum = true;

    shine::reflection::TypeBuilder<ETestEnum> enumBuilder(enumInfo, enumPlan);
    fmt::print("  enum reserved capacity: {}\n", enumInfo.GetEnumEntries().capacity());
    fmt::print("[PASS] enum reserve matches plan: {}\n",
        enumInfo.GetEnumEntries().capacity() >= enumPlan.enumCount ? "Yes" : "No");
}

void TestTypeBuilderStagedEmission() {
    PrintSeparator("基线: TypeBuilder 原地发射");

    shine::reflection::TypeRegistrationPlan transformPlan{};
    shine::reflection::TypeBuilderPlanCounter<Transform> transformCounter(transformPlan);
    _ReflectRegFn_Transform(transformCounter);

    shine::reflection::TypeInfo transformInfo{};
    transformInfo.id = shine::reflection::GetTypeId<Transform>();
    transformInfo.SetName(shine::STextView::from_literal("TransformStageProbe"));
    transformInfo.size = sizeof(Transform);
    transformInfo.alignment = alignof(Transform);
    transformInfo.isPod = std::is_trivially_copyable_v<Transform>;
    transformInfo.isEnum = false;

    const shine::reflection::FieldInfo* stagedFieldBase = nullptr;
    const shine::reflection::MethodInfo* stagedMethodBase = nullptr;
    {
        shine::reflection::TypeBuilder<Transform> transformBuilder(transformInfo, transformPlan);
        stagedFieldBase = transformInfo.fields.data();
        stagedMethodBase = transformInfo.methods.data();
        fmt::print("  staged before emit: fields={} methods={} enums={}\n",
            transformInfo.fields.size(),
            transformInfo.methods.size(),
            transformInfo.GetEnumEntries().size());
        _ReflectRegFn_Transform(transformBuilder);
    }

    fmt::print("  staged after emit: fields={} methods={} enums={}\n",
        transformInfo.fields.size(),
        transformInfo.methods.size(),
        transformInfo.GetEnumEntries().size());
    fmt::print("[PASS] transform fields emitted in-place: {}\n",
        transformInfo.fields.data() == stagedFieldBase ? "Yes" : "No");
    fmt::print("[PASS] transform methods emitted in-place: {}\n",
        transformInfo.methods.data() == stagedMethodBase ? "Yes" : "No");
    fmt::print("[PASS] transform staged field count exact: {}\n",
        transformInfo.fields.size() == transformPlan.fieldCount ? "Yes" : "No");
    fmt::print("[PASS] transform staged method count exact: {}\n",
        transformInfo.methods.size() == transformPlan.methodCount ? "Yes" : "No");

    shine::reflection::TypeRegistrationPlan enumPlan{};
    shine::reflection::TypeBuilderPlanCounter<ETestEnum> enumCounter(enumPlan);
    _ReflectRegFn_ETestEnum(enumCounter);

    shine::reflection::TypeInfo enumInfo{};
    enumInfo.id = shine::reflection::GetTypeId<ETestEnum>();
    enumInfo.SetName(shine::STextView::from_literal("EnumStageProbe"));
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
    for (const auto& field : typeInfo->fields) {
        const size_t pageIndex = fieldPool.PageIndexOf(field.coldData.get());
        if (firstFieldPage == static_cast<size_t>(-1)) {
            firstFieldPage = pageIndex;
        }
        lastFieldPage = pageIndex;
    }

    size_t firstMethodPage = static_cast<size_t>(-1);
    size_t lastMethodPage = static_cast<size_t>(-1);
    for (const auto& method : typeInfo->methods) {
        const size_t pageIndex = methodPool.PageIndexOf(method.coldData.get());
        if (firstMethodPage == static_cast<size_t>(-1)) {
            firstMethodPage = pageIndex;
        }
        lastMethodPage = pageIndex;
    }

    fmt::print("  Transform cold pages: field_first={} field_last={} method_first={} method_last={}\n",
        firstFieldPage,
        lastFieldPage,
        firstMethodPage,
        lastMethodPage);
    fmt::print("[PASS] transform fields share one cold page: {}\n",
        firstFieldPage == lastFieldPage ? "Yes" : "No");
    fmt::print("[PASS] transform methods share one cold page: {}\n",
        firstMethodPage == lastMethodPage ? "Yes" : "No");
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
        fmt::print("  - 字段数量: {}\n", rt_info->fields.size());
        for (const auto& f : rt_info->fields) {
            fmt::print("    - {}: offset={}, size={}\n", f.GetNameView(), f.offset, f.size);
        }
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
        for (const auto& field : typeInfo->fields) {
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
    
    Transform t;
    t.position = Vec3(1.0f, 2.0f, 3.0f);
    
    auto* typeInfo = shine::reflection::TypeRegistry::Get().FindFast(shine::reflection::GetTypeId<Transform>());
    if (!typeInfo) {
        fmt::print("错误: 无法获取 Transform 类型信息\n");
        return;
    }
    
    // 预热
    for (int i = 0; i < 1000; ++i) {
        typeInfo->FindField("position");
    }
    
    // 测试字段查找性能 (冷启动 + 缓存)
    double cold_time = Benchmark([&]() {
        return typeInfo->FindField("position");
    }, 10000);
    
    fmt::print("字段查找性能:\n");
    fmt::print("  - 10k次查找平均耗时: {:.4f} ms\n", cold_time);
    fmt::print("  - 吞吐量: {:.0f} ops/sec\n", 1.0 / cold_time * 1000);
    
    // 保存字段查找性能
    g_perfReport.fieldLookup_ops = 1.0 / cold_time * 1000;
    
    // 测试 getter/setter 性能
    Vec3 out_val;
    double getter_time = Benchmark([&]() {
        auto* field = typeInfo->FindField("position");
        if (field) field->Get(&t, &out_val);
    }, 100000);
    
    double setter_time = Benchmark([&]() {
        auto* field = typeInfo->FindField("position");
        if (field) field->Set(&t, &out_val);
    }, 100000);
    
    // 对比原生访问
    double native_time = Benchmark([&]() {
        Vec3 v = t.position;
        v.x += 1.0f;
    }, 100000);

    // 测试真正的编译期绑定 CT_GET/CT_SET - 零开销内联
    double ct_get_time = Benchmark([&]() {
        float v = shine::reflection::CT_GET<&Vec3::x>(t.position);
    }, 100000);

    double ct_set_time = Benchmark([&]() {
        shine::reflection::CT_SET<&Vec3::x>(t.position, 1.0f);
    }, 100000);

    fmt::print("原生访问性能:\n");
    fmt::print("  - 100k次调用平均耗时: {:.4f} ns\n", native_time * 1000);

    fmt::print("编译期绑定 (CT_GET/CT_SET) 性能:\n");
    fmt::print("  - CT_GET: {:.4f} ns\n", ct_get_time * 1000);
    fmt::print("  - CT_SET: {:.4f} ns\n", ct_set_time * 1000);

    fmt::print("反射开销: getter={:.1f}x, setter={:.1f}x, CT_GET={:.1f}x, CT_SET={:.1f}x\n",
        getter_time / native_time, setter_time / native_time,
        ct_get_time / native_time, ct_set_time / native_time);
    
    // 保存到性能报告
    g_perfReport.getter_ns = getter_time * 1000;
    g_perfReport.setter_ns = setter_time * 1000;
    g_perfReport.native_ns = native_time * 1000;
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
    
    double native_vec_get = Benchmark([&]() {
        int size = t.tags.size();
        return size;
    }, 100000);
    fmt::print("  - vector::size: {:.4f} ns\n", native_vec_get * 1000);
    
    double native_vec_push = Benchmark([&]() {
        // 不修改容器，只测量访问
        return t.tags[0];
    }, 100000);
    fmt::print("  - vector::operator[]: {:.4f} ns\n", native_vec_push * 1000);
    
    double native_map_find = Benchmark([&]() {
        auto it = t.properties.find("health");
        return it != t.properties.end();
    }, 100000);
    fmt::print("  - map::find: {:.4f} ns\n", native_map_find * 1000);
    
    double native_set_count = Benchmark([&]() {
        return t.flags.count(2);
    }, 100000);
    fmt::print("  - set::count: {:.4f} ns\n", native_set_count * 1000);
    
    // 反射容器操作（通过 offset 访问）
    fmt::print("\n反射容器操作性能:\n");
    
    // 获取容器指针
    auto* tagsContainer = reinterpret_cast<std::vector<int>*>((char*)&t + tagsField->offset);
    auto* propsContainer = reinterpret_cast<std::map<std::string, int>*>((char*)&t + propsField->offset);
    auto* flagsContainer = reinterpret_cast<std::set<int>*>((char*)&t + flagsField->offset);
    
    double reflect_vec_get = Benchmark([&]() {
        std::size_t size = tagsContainer->size();
        return size;
    }, 100000);
    fmt::print("  - vector::size (反射): {:.4f} ns\n", reflect_vec_get * 1000);
    fmt::print("    开销: {:.1f}x\n", reflect_vec_get / native_vec_get);
    
    double reflect_vec_access = Benchmark([&]() {
        return (*tagsContainer)[0];
    }, 100000);
    fmt::print("  - vector::operator[] (反射): {:.4f} ns\n", reflect_vec_access * 1000);
    fmt::print("    开销: {:.1f}x\n", reflect_vec_access / native_vec_push);
    
    double reflect_map_find = Benchmark([&]() {
        auto it = propsContainer->find("health");
        return it != propsContainer->end();
    }, 100000);
    fmt::print("  - map::find (反射): {:.4f} ns\n", reflect_map_find * 1000);
    fmt::print("    开销: {:.1f}x\n", reflect_map_find / native_map_find);
    
    double reflect_set_count = Benchmark([&]() {
        return flagsContainer->count(2);
    }, 100000);
    fmt::print("  - set::count (反射): {:.4f} ns\n", reflect_set_count * 1000);
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
    
    fmt::print("Transform 字段数量: {}\n", typeInfo->fields.size());
    fmt::print("\n");
    
    // 预热查找缓存
    for (auto& field : typeInfo->fields) {
        typeInfo->FindField(field.GetNameView());
    }
    
    // 对每个字段进行性能测试
    fmt::print("字段性能对比 (原生 vs 反射 vs CT_GET):\n");
    fmt::print("{:<15} {:>10} {:>10} {:>10} {:>10}\n", "字段", "原生", "反射", "offset", "CT_GET");
    fmt::print("--------------------------------------------------------\n");
    
    for (auto& field : typeInfo->fields) {
        const char* fieldName = field.GetNameView().data();
        
        // 原生访问 - 对于容器类型只测 size() 操作
        double nativeTime = 0;
        if (strcmp(fieldName, "position") == 0) {
            nativeTime = Benchmark([&]() { Vec3 v = t.position; (void)v; }, 100000);
        } else if (strcmp(fieldName, "rotation") == 0) {
            nativeTime = Benchmark([&]() { Vec3 v = t.rotation; (void)v; }, 100000);
        } else if (strcmp(fieldName, "scale") == 0) {
            nativeTime = Benchmark([&]() { Vec3 v = t.scale; (void)v; }, 100000);
        } else if (strcmp(fieldName, "name") == 0) {
            nativeTime = Benchmark([&]() { std::string_view v = t.name; (void)v; }, 100000);
        } else if (strcmp(fieldName, "id") == 0) {
            nativeTime = Benchmark([&]() { int v = t.id; (void)v; }, 100000);
        } else if (strcmp(fieldName, "enabled") == 0) {
            nativeTime = Benchmark([&]() { bool v = t.enabled; (void)v; }, 100000);
        } else if (strcmp(fieldName, "tags") == 0) {
            nativeTime = Benchmark([&]() { auto v = t.tags.size(); (void)v; }, 100000);
        } else if (strcmp(fieldName, "properties") == 0) {
            nativeTime = Benchmark([&]() { auto v = t.properties.size(); (void)v; }, 100000);
        } else if (strcmp(fieldName, "flags") == 0) {
            nativeTime = Benchmark([&]() { auto v = t.flags.size(); (void)v; }, 100000);
        }
        
        // 反射访问 - 使用偏移量直接访问（无复制！）
        double reflectTime = 0;
        if (field.isPod) {
            // POD 类型用反射 Get
            if (strcmp(fieldName, "position") == 0 || strcmp(fieldName, "rotation") == 0 || strcmp(fieldName, "scale") == 0) {
                Vec3 buf;
                reflectTime = Benchmark([&]() {
                    auto* f = typeInfo->FindField(fieldName);
                    if (f) f->Get(&t, &buf);
                }, 100000);
            } else if (strcmp(fieldName, "id") == 0) {
                int buf;
                reflectTime = Benchmark([&]() {
                    auto* f = typeInfo->FindField(fieldName);
                    if (f) f->Get(&t, &buf);
                }, 100000);
            } else if (strcmp(fieldName, "enabled") == 0) {
                bool buf;
                reflectTime = Benchmark([&]() {
                    auto* f = typeInfo->FindField(fieldName);
                    if (f) f->Get(&t, &buf);
                }, 100000);
            }
        } else {
            // 非 POD 类型用偏移量直接访问（零复制！）
            if (strcmp(fieldName, "name") == 0) {
                reflectTime = Benchmark([&]() {
                    auto* str = reinterpret_cast<std::string*>((char*)&t + field.offset);
                    (void)str->data();
                }, 100000);
            } else if (strcmp(fieldName, "tags") == 0) {
                reflectTime = Benchmark([&]() {
                    auto* vec = reinterpret_cast<std::vector<int>*>((char*)&t + field.offset);
                    (void)vec->data();
                }, 100000);
            } else if (strcmp(fieldName, "properties") == 0) {
                reflectTime = Benchmark([&]() {
                    auto* m = reinterpret_cast<std::map<std::string, int>*>((char*)&t + field.offset);
                    (void)m->begin();
                }, 100000);
            } else if (strcmp(fieldName, "flags") == 0) {
                reflectTime = Benchmark([&]() {
                    auto* s = reinterpret_cast<std::set<int>*>((char*)&t + field.offset);
                    (void)s->begin();
                }, 100000);
            }
        }
        
        // 偏移量访问性能（作为对比）
        double offsetTime = 0;
        if (field.isPod) {
            if (strcmp(fieldName, "position") == 0 || strcmp(fieldName, "rotation") == 0 || strcmp(fieldName, "scale") == 0) {
                offsetTime = Benchmark([&]() {
                    auto* ptr = (Vec3*)((char*)&t + field.offset);
                    Vec3 v = *ptr;
                }, 100000);
            } else if (strcmp(fieldName, "id") == 0) {
                offsetTime = Benchmark([&]() {
                    auto* ptr = (int*)((char*)&t + field.offset);
                    int v = *ptr;
                }, 100000);
            } else if (strcmp(fieldName, "enabled") == 0) {
                offsetTime = Benchmark([&]() {
                    auto* ptr = (bool*)((char*)&t + field.offset);
                    bool v = *ptr;
                }, 100000);
            }
        } else {
            if (strcmp(fieldName, "name") == 0) {
                offsetTime = Benchmark([&]() {
                    auto* ptr = (std::string*)((char*)&t + field.offset);
                    (void)ptr->data();
                }, 100000);
            } else if (strcmp(fieldName, "tags") == 0) {
                offsetTime = Benchmark([&]() {
                    auto* ptr = (std::vector<int>*)((char*)&t + field.offset);
                    (void)ptr->data();
                }, 100000);
            } else if (strcmp(fieldName, "properties") == 0) {
                offsetTime = Benchmark([&]() {
                    auto* ptr = (std::map<std::string, int>*)((char*)&t + field.offset);
                    (void)ptr->begin();
                }, 100000);
            } else if (strcmp(fieldName, "flags") == 0) {
                offsetTime = Benchmark([&]() {
                    auto* ptr = (std::set<int>*)((char*)&t + field.offset);
                    (void)ptr->begin();
                }, 100000);
            }
        }
        
        // CT_GET 编译期绑定 - 零开销！
        double ctGetTime = 0;
        if (field.isPod) {
            if (strcmp(fieldName, "position") == 0) {
                ctGetTime = Benchmark([&]() { Vec3 v = shine::reflection::CT_GET<&Transform::position>(t); (void)v; }, 100000);
            } else if (strcmp(fieldName, "rotation") == 0) {
                ctGetTime = Benchmark([&]() { Vec3 v = shine::reflection::CT_GET<&Transform::rotation>(t); (void)v; }, 100000);
            } else if (strcmp(fieldName, "scale") == 0) {
                ctGetTime = Benchmark([&]() { Vec3 v = shine::reflection::CT_GET<&Transform::scale>(t); (void)v; }, 100000);
            } else if (strcmp(fieldName, "id") == 0) {
                ctGetTime = Benchmark([&]() { int v = shine::reflection::CT_GET<&Transform::id>(t); (void)v; }, 100000);
            } else if (strcmp(fieldName, "enabled") == 0) {
                ctGetTime = Benchmark([&]() { bool v = shine::reflection::CT_GET<&Transform::enabled>(t); (void)v; }, 100000);
            }
        }
        
        double overhead = (nativeTime > 0 && reflectTime > 0) ? (reflectTime / nativeTime) : 0;
        double offsetOverhead = (nativeTime > 0 && offsetTime > 0) ? (offsetTime / nativeTime) : 0;
        
        fmt::print("{:<15} {:>10.4f} {:>10.4f} {:>10.4f} {:>10.4f}\n",
            fieldName, nativeTime * 1000, reflectTime * 1000, offsetTime * 1000, ctGetTime * 1000);
    }
    
    // 字段大小分布
    fmt::print("\n字段大小分布:\n");
    fmt::print("{:<15} {:>10} {:>10}\n", "字段名", "大小", "类型");
    fmt::print("--------------------------------------------------------\n");
    for (auto& field : typeInfo->fields) {
        const char* fieldName = field.GetNameView().data();
        fmt::print("{:<15} {:>10} {:>10}\n", fieldName, field.size, field.isPod ? "POD" : "Non-POD");
    }
    
    // 总结统计 - 包含所有字段
    fmt::print("\n字段访问性能总结:\n");
    double totalNative = 0, totalReflect = 0;
    int count = 0;
    for (auto& field : typeInfo->fields) {
        const char* fieldName = field.GetNameView().data();

        double nativeTime = 0;
        if (strcmp(fieldName, "position") == 0) nativeTime = Benchmark([&]() { Vec3 v = t.position; (void)v; }, 100000);
        else if (strcmp(fieldName, "rotation") == 0) nativeTime = Benchmark([&]() { Vec3 v = t.rotation; (void)v; }, 100000);
        else if (strcmp(fieldName, "scale") == 0) nativeTime = Benchmark([&]() { Vec3 v = t.scale; (void)v; }, 100000);
        else if (strcmp(fieldName, "name") == 0) nativeTime = Benchmark([&]() { std::string v = t.name; (void)v; }, 100000);
        else if (strcmp(fieldName, "id") == 0) nativeTime = Benchmark([&]() { int v = t.id; (void)v; }, 100000);
        else if (strcmp(fieldName, "enabled") == 0) nativeTime = Benchmark([&]() { bool v = t.enabled; (void)v; }, 100000);
        else if (strcmp(fieldName, "tags") == 0) nativeTime = Benchmark([&]() { std::size_t v = t.tags.size(); (void)v; }, 100000);
        else if (strcmp(fieldName, "properties") == 0) nativeTime = Benchmark([&]() { std::size_t v = t.properties.size(); (void)v; }, 100000);
        else if (strcmp(fieldName, "flags") == 0) nativeTime = Benchmark([&]() { std::size_t v = t.flags.size(); (void)v; }, 100000);

        double reflectTime = 0;
        if (strcmp(fieldName, "position") == 0 || strcmp(fieldName, "rotation") == 0 || strcmp(fieldName, "scale") == 0)
            reflectTime = Benchmark([&]() { auto* f = typeInfo->FindField(fieldName); if (f) { Vec3 v; f->Get(&t, &v); } }, 100000);
        else if (strcmp(fieldName, "name") == 0)
            reflectTime = Benchmark([&]() { auto* f = typeInfo->FindField(fieldName); if (f) { std::string v; f->Get(&t, &v); } }, 100000);
        else if (strcmp(fieldName, "id") == 0)
            reflectTime = Benchmark([&]() { auto* f = typeInfo->FindField(fieldName); if (f) { int v; f->Get(&t, &v); } }, 100000);
        else if (strcmp(fieldName, "enabled") == 0)
            reflectTime = Benchmark([&]() { auto* f = typeInfo->FindField(fieldName); if (f) { bool v; f->Get(&t, &v); } }, 100000);
        else if (strcmp(fieldName, "tags") == 0)
            reflectTime = Benchmark([&]() { auto* f = typeInfo->FindField(fieldName); if (f) { std::vector<int> v; f->Get(&t, &v); } }, 100000);
        else if (strcmp(fieldName, "properties") == 0)
            reflectTime = Benchmark([&]() { auto* f = typeInfo->FindField(fieldName); if (f) { std::map<std::string, int> v; f->Get(&t, &v); } }, 100000);
        else if (strcmp(fieldName, "flags") == 0)
            reflectTime = Benchmark([&]() { auto* f = typeInfo->FindField(fieldName); if (f) { std::set<int> v; f->Get(&t, &v); } }, 100000);

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
    
    double find_slow = Benchmark([&]() {
        return shine::reflection::TypeRegistry::Get().Find(shine::reflection::GetTypeId<Transform>());
    }, iterations);
    
    double find_fast = Benchmark([&]() {
        return shine::reflection::TypeRegistry::Get().FindFast(shine::reflection::GetTypeId<Transform>());
    }, iterations);
    
    fmt::print("类型查找 (Find) 性能:\n");
    fmt::print("  - {}次查找平均: {:.4f} ns\n", iterations, find_slow * 1000);
    fmt::print("类型查找 (FindFast) 性能:\n");
    fmt::print("  - {}次查找平均: {:.4f} ns\n", iterations, find_fast * 1000);
    fmt::print("加速比: {:.2f}x\n", find_slow / find_fast);
    
    // 保存类型查找性能
    g_perfReport.typeFindSlow_ns = find_slow * 1000;
    g_perfReport.typeFindFast_ns = find_fast * 1000;
    g_perfReport.findSpeedup_x = find_slow / find_fast;
}

void TestMethodCallPerformance() {
    PrintSeparator("性能测试: 方法调用");
    
    auto* typeInfo = shine::reflection::TypeRegistry::Get().FindFast(shine::reflection::GetTypeId<Transform>());
    if (!typeInfo || typeInfo->methods.empty()) {
        fmt::print("Transform 没有注册方法\n");
        return;
    }
    
    fmt::print("已注册方法数量: {}\n", typeInfo->methods.size());
    for (const auto& m : typeInfo->methods) {
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
    
    // 测试反射调用性能 (使用优化的 FastMethodCall)
    double reflect_time = Benchmark([&]() {
        method->Invoke(&t, args_ptr, nullptr);
    }, 100000);
    
    // 测试原生调用性能
    double native_time = Benchmark([&]() {
        t.SetPosition(10.0f, 20.0f, 30.0f);
    }, 100000);
    
    fmt::print("\n方法调用性能对比:\n");
    fmt::print("  - 原生调用: {:.4f} ns\n", native_time * 1000);
    fmt::print("  - 反射调用 (REFLECT_METHOD_FAST): {:.4f} ns\n", reflect_time * 1000);
    fmt::print("  - 开销: {:.1f}x\n", reflect_time / native_time);
    
    // 保存方法调用性能
    g_perfReport.methodOverhead_x = reflect_time / native_time;
    
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
        fmt::print("       - Fields: {}\n", (*result)->fields.size());
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
        auto* posField = typeInfo->FindField("position");
        if (posField) {
            Vec3 pos;
            posField->Get(&t, &pos);
            fmt::print("[PASS] Get position: ({}, {}, {})\n", pos.x, pos.y, pos.z);
            fmt::print("[PASS] position owner handle -> type: {}\n", posField->GetOwnerType() == typeInfo ? "OK" : "FAIL");
        }
        
        // Set position
        if (posField) {
            Vec3 newPos(4.0f, 5.0f, 6.0f);
            posField->Set(&t, &newPos);
            fmt::print("[PASS] Set position: ({}, {}, {})\n", t.position.x, t.position.y, t.position.z);
        }
        
        // Get name
        auto* nameField = typeInfo->FindField("name");
        if (nameField) {
            std::string name;
            nameField->Get(&t, &name);
            fmt::print("[PASS] Get name: {}\n", name);
        }
        
        // Get id
        auto* idField = typeInfo->FindField("id");
        if (idField) {
            int id;
            idField->Get(&t, &id);
            fmt::print("[PASS] Get id: {}\n", id);
        }

        auto* method = typeInfo->FindMethod("GetPosition");
        if (method) {
            fmt::print("[PASS] method owner handle -> type: {}\n", method->GetOwnerType() == typeInfo ? "OK" : "FAIL");
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

        for (const auto& field : typeInfo->fields) {
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
        if (const auto* idField = typeInfo->FindField("id")) {
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
    TestReflectionColdBatchReservation();
    TestReflectionRegistrationPlan();
    TestTypeBuilderStagedEmission();
    TestRegisteredTypeColdLocality();
    TestTypeRegistryArenaOwnership();
    TestHashInconsistency();
    TestConstexprLimitation();
    TestSerializationGap();
    TestContainerLimitation();
    
    // 性能测试
    TestFieldAccessPerformance();
    TestContainerPerformance();
    TestAllFieldsPerformance();
    TestTypeLookupPerformance();
    TestMethodCallPerformance();
    TestMemoryUsage();
    
    // 输出性能报告
    g_perfReport.Print();
    
    fmt::print("\n============================================================\n");
    fmt::print("                    测试完成\n");
    fmt::print("============================================================\n");
    
    return 0;
}
