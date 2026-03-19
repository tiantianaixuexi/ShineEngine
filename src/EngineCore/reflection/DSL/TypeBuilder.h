#pragma once

// =============================================================================
// TypeBuilder.h — Runtime Type Builder for Reflection System
// =============================================================================
//
// Provides fluent API for building TypeInfo at runtime from DSL nodes.
// Used by REFLECTION_STRUCT and REFLECT_ENUM macros.
//
// C++23 / MSVC
// =============================================================================

#include "../ReflectionCore.h"
#include "FieldDSL.h"
#include "MethodDSL.h"
#include <type_traits>
#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <array>
#include <cassert>
#include <memory>
#include <span>

namespace shine::reflection {

template <typename T>
static void CopyColdSpan(ReflectionColdVector<T>& destination, std::span<const T> source) {
    destination.resize(source.size());
    for (std::size_t index = 0; index < source.size(); ++index) {
        destination[index] = source[index];
    }
}

template <typename T>
class ReflectionPlanBlock {
private:
    struct StagePage;

public:
    class const_iterator {
    public:
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = const T*;
        using reference = const T&;
        using iterator_category = std::forward_iterator_tag;

        const_iterator() = default;

        [[nodiscard]] reference operator*() const noexcept {
            return (*owner_)[index_];
        }

        [[nodiscard]] pointer operator->() const noexcept {
            return &(*owner_)[index_];
        }

        const_iterator& operator++() noexcept {
            ++index_;
            return *this;
        }

        const_iterator operator++(int) noexcept {
            const_iterator copy = *this;
            ++(*this);
            return copy;
        }

        [[nodiscard]] friend bool operator==(const const_iterator& lhs, const const_iterator& rhs) noexcept {
            return lhs.owner_ == rhs.owner_ && lhs.index_ == rhs.index_;
        }

    private:
        friend class ReflectionPlanBlock;

        const_iterator(const ReflectionPlanBlock* owner, size_t index) noexcept
            : owner_(owner)
            , index_(index) {}

        const ReflectionPlanBlock* owner_ = nullptr;
        size_t index_ = 0;
    };

    ReflectionPlanBlock() = default;

    ~ReflectionPlanBlock() {
        Reset();
    }

    ReflectionPlanBlock(const ReflectionPlanBlock&) = delete;
    ReflectionPlanBlock& operator=(const ReflectionPlanBlock&) = delete;

    ReflectionPlanBlock(ReflectionPlanBlock&& other) noexcept
        : stageHead_(other.stageHead_)
        , stageTail_(other.stageTail_)
        , stagingSize_(other.stagingSize_)
        , frozenData_(other.frozenData_)
        , frozenSize_(other.frozenSize_) {
        other.stageHead_ = nullptr;
        other.stageTail_ = nullptr;
        other.stagingSize_ = 0;
        other.frozenData_ = nullptr;
        other.frozenSize_ = 0;
    }

    ReflectionPlanBlock& operator=(ReflectionPlanBlock&& other) noexcept {
        if (this == &other) {
            return *this;
        }

        Reset();
        stageHead_ = other.stageHead_;
        stageTail_ = other.stageTail_;
        stagingSize_ = other.stagingSize_;
        frozenData_ = other.frozenData_;
        frozenSize_ = other.frozenSize_;
        other.stageHead_ = nullptr;
        other.stageTail_ = nullptr;
        other.stagingSize_ = 0;
        other.frozenData_ = nullptr;
        other.frozenSize_ = 0;
        return *this;
    }

    [[nodiscard]] bool empty() const noexcept {
        return size() == 0;
    }

    [[nodiscard]] bool is_frozen() const noexcept {
        return frozenData_ != nullptr;
    }

    [[nodiscard]] size_t StagingPageCount() const noexcept {
        size_t count = 0;
        for (const StagePage* page = stageHead_; page != nullptr; page = page->next) {
            ++count;
        }
        return count;
    }

    [[nodiscard]] size_t size() const noexcept {
        return frozenData_ != nullptr ? frozenSize_ : stagingSize_;
    }

    [[nodiscard]] const T* data() const noexcept {
        return frozenData_;
    }

    [[nodiscard]] const T& operator[](size_t index) const noexcept {
        assert(index < size() && "ReflectionPlanBlock index out of range");
        if (frozenData_ != nullptr) {
            return frozenData_[index];
        }

        const StagePage* page = stageHead_;
        size_t remaining = index;
        while (page != nullptr) {
            if (remaining < page->used) {
                return page->data[remaining];
            }
            remaining -= page->used;
            page = page->next;
        }

        assert(false && "ReflectionPlanBlock staged page lookup failed");
        return frozenData_[0];
    }

    [[nodiscard]] const_iterator begin() const noexcept {
        return const_iterator(this, 0);
    }

    [[nodiscard]] const_iterator end() const noexcept {
        return const_iterator(this, size());
    }

    [[nodiscard]] std::span<const T> span() const noexcept {
        return frozenData_ != nullptr ? std::span<const T>{frozenData_, frozenSize_} : std::span<const T>{};
    }

    void push_back(const T& value) {
        emplace_back(value);
    }

    void push_back(T&& value) {
        emplace_back(std::move(value));
    }

    template <typename... TArgs>
    T& emplace_back(TArgs&&... args) {
        assert(frozenData_ == nullptr && "ReflectionPlanBlock is frozen");
        StagePage* page = AcquireStagePage();
        assert(page != nullptr && "ReflectionPlanBlock staging allocation failed");
        auto* slot = page->data + page->used;
        ++page->used;
        ++stagingSize_;
        return *std::construct_at(slot, std::forward<TArgs>(args)...);
    }

    void Freeze() {
        if (frozenData_ != nullptr || stagingSize_ == 0) {
            return;
        }

        auto* storage = ReflectionMemoryManager::GetInstance().Allocate<std::byte>(sizeof(T) * stagingSize_, alignof(T));
        assert(storage != nullptr && "ReflectionPlanBlock freeze allocation failed");
        if (!storage) {
            return;
        }

        auto* frozen = reinterpret_cast<T*>(storage);
        T* writeCursor = frozen;
        for (StagePage* page = stageHead_; page != nullptr; page = page->next) {
            writeCursor = std::uninitialized_move_n(page->data, page->used, writeCursor).second;
        }
        frozenData_ = frozen;
        frozenSize_ = stagingSize_;
        ReleaseStagePages();
    }

    void Reset() noexcept {
        if (frozenData_ != nullptr) {
            std::destroy_n(frozenData_, frozenSize_);
            ReflectionMemoryManager::GetInstance().Deallocate(frozenData_);
            frozenData_ = nullptr;
            frozenSize_ = 0;
        }
        ReleaseStagePages();
    }

private:
    struct StagePage {
        StagePage* next = nullptr;
        size_t used = 0;
        T* data = nullptr;
    };

    static constexpr size_t AlignUp(size_t value, size_t alignment) noexcept {
        return (value + alignment - 1) & ~(alignment - 1);
    }

    static constexpr size_t kStageSlotsPerPage = (std::max)(size_t{16}, size_t{16 * 1024} / (sizeof(T) == 0 ? 1 : sizeof(T)));

    [[nodiscard]] StagePage* AcquireStagePage() {
        if (stageTail_ != nullptr && stageTail_->used < kStageSlotsPerPage) {
            return stageTail_;
        }

        constexpr size_t pageHeaderBytes = AlignUp(sizeof(StagePage), alignof(T));
        constexpr size_t pageBytes = pageHeaderBytes + sizeof(T) * kStageSlotsPerPage;
        auto* rawPage = ReflectionMemoryManager::GetInstance().Allocate<std::byte>(pageBytes, (std::max)(alignof(StagePage), alignof(T)));
        if (!rawPage) {
            return nullptr;
        }

        auto* page = std::construct_at(reinterpret_cast<StagePage*>(rawPage));
        page->data = reinterpret_cast<T*>(rawPage + pageHeaderBytes);
        if (stageTail_ != nullptr) {
            stageTail_->next = page;
        } else {
            stageHead_ = page;
        }
        stageTail_ = page;
        return page;
    }

    void ReleaseStagePages() noexcept {
        StagePage* page = stageHead_;
        while (page != nullptr) {
            for (size_t index = 0; index < page->used; ++index) {
                std::destroy_at(page->data + index);
            }
            StagePage* next = page->next;
            ReflectionMemoryManager::GetInstance().Deallocate(page);
            page = next;
        }
        stageHead_ = nullptr;
        stageTail_ = nullptr;
        stagingSize_ = 0;
    }

    StagePage* stageHead_ = nullptr;
    StagePage* stageTail_ = nullptr;
    size_t stagingSize_ = 0;
    T* frozenData_ = nullptr;
    size_t frozenSize_ = 0;
};

struct TypeRegistrationPlan {
    using RuntimeMetadataEntry = std::pair<MetadataKey, MetadataValue>;
    using RuntimeMetadataSpan = std::span<const RuntimeMetadataEntry>;
    using ParamTypeSpan = std::span<const TypeId>;
    using RuntimeMetadataBlock = ReflectionPlanBlock<RuntimeMetadataEntry>;
    using ParamTypeBlock = ReflectionPlanBlock<TypeId>;

    struct FieldPlan {
        shine::STextView name;
        uint32_t nameHash = 0;
        TypeId typeId = 0;
        ContainerType containerType = ContainerType::None;
        const void* containerTrait = nullptr;
        std::size_t offset = 0;
        std::size_t size = 0;
        std::size_t alignment = 0;
        bool isPod = false;
        PropertyFlags flags = PropertyFlags::None;
        UI::Schema uiSchema = UI::None{};
        shine::STextView displayName;
        shine::STextView category;
        shine::STextView editCondition;
        float minValue = 0.0f;
        float maxValue = 0.0f;
        size_t runtimeMetadataOffset = 0;
        size_t runtimeMetadataCount = 0;
        bool hasUISchema = false;
        bool hasDisplayName = false;
        bool hasCategory = false;
        bool hasEditCondition = false;
        bool hasRange = false;
    };

    struct MethodPlan {
        shine::STextView name;
        uint32_t nameHash = 0;
        TypeId returnType = 0;
        size_t paramTypeOffset = 0;
        size_t paramTypeCount = 0;
        FunctionFlags flags = FunctionFlags::None;
        size_t runtimeMetadataOffset = 0;
        size_t runtimeMetadataCount = 0;
    };

    struct EnumPlan {
        int64_t value = 0;
        shine::STextView name;
    };

    using FieldPlanBlock = ReflectionPlanBlock<FieldPlan>;
    using MethodPlanBlock = ReflectionPlanBlock<MethodPlan>;
    using EnumPlanBlock = ReflectionPlanBlock<EnumPlan>;

    struct EmitLayout {
        struct FieldEmitDescriptor {
            const FieldPlan* plan = nullptr;
            RuntimeMetadataSpan runtimeMetadata{};
        };

        struct MethodEmitDescriptor {
            const MethodPlan* plan = nullptr;
            RuntimeMetadataSpan runtimeMetadata{};
            ParamTypeSpan paramTypes{};
        };

        struct EnumEmitDescriptor {
            const EnumPlan* plan = nullptr;
        };

        std::span<const FieldPlan> fieldPlans{};
        std::span<const MethodPlan> methodPlans{};
        std::span<const EnumPlan> enumPlans{};
        RuntimeMetadataSpan runtimeMetadataEntries{};
        ParamTypeSpan methodParamTypeEntries{};
        std::span<const FieldEmitDescriptor> fieldDescriptors{};
        std::span<const MethodEmitDescriptor> methodDescriptors{};
        std::span<const EnumEmitDescriptor> enumDescriptors{};

        [[nodiscard]] const FieldPlan* GetFieldPlan(size_t fieldIndex) const noexcept {
            return fieldIndex < fieldPlans.size() ? &fieldPlans[fieldIndex] : nullptr;
        }

        [[nodiscard]] const MethodPlan* GetMethodPlan(size_t methodIndex) const noexcept {
            return methodIndex < methodPlans.size() ? &methodPlans[methodIndex] : nullptr;
        }

        [[nodiscard]] const EnumPlan* GetEnumPlan(size_t enumIndex) const noexcept {
            return enumIndex < enumPlans.size() ? &enumPlans[enumIndex] : nullptr;
        }

        [[nodiscard]] FieldEmitDescriptor GetFieldDescriptor(size_t fieldIndex) const noexcept {
            if (fieldIndex < fieldDescriptors.size()) {
                return fieldDescriptors[fieldIndex];
            }
            const auto* plan = GetFieldPlan(fieldIndex);
            return FieldEmitDescriptor{
                plan,
                plan != nullptr ? GetRuntimeMetadata(*plan) : RuntimeMetadataSpan{}
            };
        }

        [[nodiscard]] MethodEmitDescriptor GetMethodDescriptor(size_t methodIndex) const noexcept {
            if (methodIndex < methodDescriptors.size()) {
                return methodDescriptors[methodIndex];
            }
            const auto* plan = GetMethodPlan(methodIndex);
            return MethodEmitDescriptor{
                plan,
                plan != nullptr ? GetRuntimeMetadata(*plan) : RuntimeMetadataSpan{},
                plan != nullptr ? GetMethodParamTypes(*plan) : ParamTypeSpan{}
            };
        }

        [[nodiscard]] EnumEmitDescriptor GetEnumDescriptor(size_t enumIndex) const noexcept {
            if (enumIndex < enumDescriptors.size()) {
                return enumDescriptors[enumIndex];
            }
            return EnumEmitDescriptor{GetEnumPlan(enumIndex)};
        }

        [[nodiscard]] RuntimeMetadataSpan GetRuntimeMetadata(const FieldPlan& fieldPlan) const noexcept {
            return MakeRuntimeMetadataSpan(fieldPlan.runtimeMetadataOffset, fieldPlan.runtimeMetadataCount);
        }

        [[nodiscard]] RuntimeMetadataSpan GetRuntimeMetadata(const MethodPlan& methodPlan) const noexcept {
            return MakeRuntimeMetadataSpan(methodPlan.runtimeMetadataOffset, methodPlan.runtimeMetadataCount);
        }

        [[nodiscard]] ParamTypeSpan GetMethodParamTypes(const MethodPlan& methodPlan) const noexcept {
            if (methodPlan.paramTypeCount == 0) {
                return {};
            }
            return ParamTypeSpan{
                methodParamTypeEntries.data() + static_cast<std::ptrdiff_t>(methodPlan.paramTypeOffset),
                methodPlan.paramTypeCount
            };
        }

    private:
        [[nodiscard]] RuntimeMetadataSpan MakeRuntimeMetadataSpan(size_t offset, size_t count) const noexcept {
            if (count == 0) {
                return {};
            }
            return RuntimeMetadataSpan{
                runtimeMetadataEntries.data() + static_cast<std::ptrdiff_t>(offset),
                count
            };
        }
    };

    using EmitView = EmitLayout;

    size_t fieldCount = 0;
    size_t methodCount = 0;
    size_t enumCount = 0;
    FieldPlanBlock fieldPlans;
    MethodPlanBlock methodPlans;
    EnumPlanBlock enumPlans;
    RuntimeMetadataBlock runtimeMetadataEntries;
    ParamTypeBlock methodParamTypeEntries;
    ReflectionPlanBlock<EmitView::FieldEmitDescriptor> fieldEmitDescriptors;
    ReflectionPlanBlock<EmitView::MethodEmitDescriptor> methodEmitDescriptors;
    ReflectionPlanBlock<EmitView::EnumEmitDescriptor> enumEmitDescriptors;
    mutable EmitLayout emitLayout_{};

    void Reset() noexcept {
        fieldCount = 0;
        methodCount = 0;
        enumCount = 0;
        fieldPlans.Reset();
        methodPlans.Reset();
        enumPlans.Reset();
        runtimeMetadataEntries.Reset();
        methodParamTypeEntries.Reset();
        fieldEmitDescriptors.Reset();
        methodEmitDescriptors.Reset();
        enumEmitDescriptors.Reset();
        emitLayout_ = {};
    }

    void FreezeSharedBlocks() {
        fieldPlans.Freeze();
        methodPlans.Freeze();
        enumPlans.Freeze();
        runtimeMetadataEntries.Freeze();
        methodParamTypeEntries.Freeze();
        BuildEmitDescriptors();
        fieldEmitDescriptors.Freeze();
        methodEmitDescriptors.Freeze();
        enumEmitDescriptors.Freeze();
        RefreshEmitLayout();
    }

    [[nodiscard]] std::span<const FieldPlan> GetFieldPlans() const noexcept {
        return fieldPlans.span();
    }

    [[nodiscard]] std::span<const MethodPlan> GetMethodPlans() const noexcept {
        return methodPlans.span();
    }

    [[nodiscard]] std::span<const EnumPlan> GetEnumPlans() const noexcept {
        return enumPlans.span();
    }

    [[nodiscard]] const EmitLayout& GetEmitLayout() const noexcept {
        RefreshEmitLayout();
        return emitLayout_;
    }

    [[nodiscard]] const EmitView& GetEmitView() const noexcept {
        return GetEmitLayout();
    }

    [[nodiscard]] const FieldPlan* GetFieldPlan(size_t fieldIndex) const noexcept {
        return fieldIndex < fieldPlans.size() ? &fieldPlans[fieldIndex] : nullptr;
    }

    [[nodiscard]] const MethodPlan* GetMethodPlan(size_t methodIndex) const noexcept {
        return methodIndex < methodPlans.size() ? &methodPlans[methodIndex] : nullptr;
    }

    [[nodiscard]] const EnumPlan* GetEnumPlan(size_t enumIndex) const noexcept {
        return enumIndex < enumPlans.size() ? &enumPlans[enumIndex] : nullptr;
    }

    [[nodiscard]] RuntimeMetadataSpan GetRuntimeMetadata(const FieldPlan& fieldPlan) const noexcept {
        return GetEmitLayout().GetRuntimeMetadata(fieldPlan);
    }

    [[nodiscard]] RuntimeMetadataSpan GetRuntimeMetadata(const MethodPlan& methodPlan) const noexcept {
        return GetEmitLayout().GetRuntimeMetadata(methodPlan);
    }

    [[nodiscard]] ParamTypeSpan GetMethodParamTypes(const MethodPlan& methodPlan) const noexcept {
        return GetEmitLayout().GetMethodParamTypes(methodPlan);
    }

private:
    [[nodiscard]] EmitLayout MakeEmitLayout() const noexcept {
        return EmitLayout{
            GetFieldPlans(),
            GetMethodPlans(),
            GetEnumPlans(),
            runtimeMetadataEntries.span(),
            methodParamTypeEntries.span(),
            fieldEmitDescriptors.span(),
            methodEmitDescriptors.span(),
            enumEmitDescriptors.span()
        };
    }

    void RefreshEmitLayout() const noexcept {
        emitLayout_ = MakeEmitLayout();
    }

    void BuildEmitDescriptors() {
        if (!fieldEmitDescriptors.empty() || !methodEmitDescriptors.empty() || !enumEmitDescriptors.empty()) {
            return;
        }

        const auto emitLayout = MakeEmitLayout();
        for (const auto& fieldPlan : fieldPlans) {
            fieldEmitDescriptors.push_back(EmitView::FieldEmitDescriptor{
                &fieldPlan,
                emitLayout.GetRuntimeMetadata(fieldPlan)
            });
        }

        for (const auto& methodPlan : methodPlans) {
            methodEmitDescriptors.push_back(EmitView::MethodEmitDescriptor{
                &methodPlan,
                emitLayout.GetRuntimeMetadata(methodPlan),
                emitLayout.GetMethodParamTypes(methodPlan)
            });
        }

        for (const auto& enumPlan : enumPlans) {
            enumEmitDescriptors.push_back(EmitView::EnumEmitDescriptor{&enumPlan});
        }
    }
};

template <typename T>
class TypeRegistrationGraph;

// Traits detection
template<typename T> struct is_sequence_container : std::false_type {};
template<typename T> struct is_associative_container : std::false_type {};
template<typename T> struct container_trait_provider { static const void* get() { return nullptr; } };

template<typename T, typename A> struct is_sequence_container<std::vector<T, A>> : std::true_type {};
template<typename T, typename A> struct container_trait_provider<std::vector<T, A>> { static const void* get() { return &ListThunks<std::vector<T, A>>::GetTrait(); } };

template<typename K, typename V, typename C, typename A> struct is_associative_container<std::map<K, V, C, A>> : std::true_type {};
template<typename K, typename V, typename C, typename A> struct container_trait_provider<std::map<K, V, C, A>> { static const void* get() { return &MapThunks<std::map<K, V, C, A>>::GetTrait(); } };

template<typename K, typename V, typename H, typename E, typename A> struct is_associative_container<std::unordered_map<K, V, H, E, A>> : std::true_type {};
template<typename K, typename V, typename H, typename E, typename A> struct container_trait_provider<std::unordered_map<K, V, H, E, A>> { static const void* get() { return &MapThunks<std::unordered_map<K, V, H, E, A>>::GetTrait(); } };

template<typename T, typename C, typename A> struct is_associative_container<std::set<T, C, A>> : std::true_type {};
template<typename T, typename C, typename A> struct container_trait_provider<std::set<T, C, A>> { static const void* get() { return &SetThunks<std::set<T, C, A>>::GetTrait(); } };

template<typename T, typename H, typename E, typename A> struct is_associative_container<std::unordered_set<T, H, E, A>> : std::true_type {};
template<typename T, typename H, typename E, typename A> struct container_trait_provider<std::unordered_set<T, H, E, A>> { static const void* get() { return &SetThunks<std::unordered_set<T, H, E, A>>::GetTrait(); } };

// For std::array we need ArrayThunks
template <typename ArrayType>
struct ArrayThunks {
    static std::size_t GetSize(const void* p) { return std::tuple_size_v<ArrayType>; }
    static void* GetElement(void* p, std::size_t i) { return &(*static_cast<ArrayType*>(p))[i]; }
    static const void* GetElementConst(const void* p, std::size_t i) { return &(*static_cast<const ArrayType*>(p))[i]; }
    static void Resize(void* p, std::size_t) { /* no-op */ }
    static const SequenceTrait& GetTrait() {
        static const SequenceTrait trait{
            GetTypeId<typename ArrayType::value_type>(),
            &GetSize, &GetElement, &GetElementConst, &Resize
        };
        return trait;
    }
};

template<typename T, std::size_t N> struct is_sequence_container<std::array<T, N>> : std::true_type {};
template<typename T, std::size_t N> struct container_trait_provider<std::array<T, N>> { static const void* get() { return &ArrayThunks<std::array<T, N>>::GetTrait(); } };

// =============================================================================
// TypeBuilder — runtime DSL builder
// =============================================================================

template <typename T, typename Limits = void>
struct TypeBuilder {
    using ObjectType = T;

    TypeInfo& info;

    TypeBuilder(const TypeBuilder&) = delete;
    TypeBuilder& operator=(const TypeBuilder&) = delete;
    TypeBuilder(TypeBuilder&&) = delete;
    TypeBuilder& operator=(TypeBuilder&&) = delete;

    explicit TypeBuilder(TypeInfo& ti, const TypeRegistrationPlan& plan = EmptyPlan())
        : info(ti)
        , emitLayout_(&plan.GetEmitLayout())
        , fieldBatch_(ReflectionColdPool<FieldColdData>::Get().BeginContiguousBatch(plan.fieldCount))
        , methodBatch_(ReflectionColdPool<MethodColdData>::Get().BeginContiguousBatch(plan.methodCount)) {
        auto& fields = info.MutableFields();
        auto& methods = info.MutableMethods();
        if (plan.fieldCount != 0) {
            fieldPlanBaseIndex_ = fields.size();
            fields.resize(fieldPlanBaseIndex_ + plan.fieldCount);
            fieldsPreSized_ = true;
            fieldWriteIndex_ = fieldPlanBaseIndex_;
        }
        if (plan.methodCount != 0) {
            methodPlanBaseIndex_ = methods.size();
            methods.resize(methodPlanBaseIndex_ + plan.methodCount);
            methodsPreSized_ = true;
            methodWriteIndex_ = methodPlanBaseIndex_;
        }
        if (plan.enumCount != 0) {
            auto& enumEntries = info.MutableEnumEntries();
            enumPlanBaseIndex_ = enumEntries.size();
            enumEntries.resize(enumPlanBaseIndex_ + plan.enumCount);
            enumsPreSized_ = true;
            enumWriteIndex_ = enumPlanBaseIndex_;
        }

        EmitPlannedLayout();
    }

    ~TypeBuilder() {
        if (fieldsPreSized_) {
            info.MutableFields().resize(fieldWriteIndex_);
        }
        if (methodsPreSized_) {
            info.MutableMethods().resize(methodWriteIndex_);
        }
        if (enumsPreSized_) {
            info.MutableEnumEntries().resize(enumWriteIndex_);
        }
    }

    // ---- FieldBuilder (fluent API, C++23: unified Meta via template) --------

    struct FieldBuilder {
        FieldInfo&    field;
        TypeBuilder&  builder;
        FieldColdData& coldData;
        const TypeRegistrationPlan::FieldPlan* plan = nullptr;

        FieldBuilder& Range(float lo, float hi) {
            if (plan != nullptr) {
                return *this;
            }
            coldData.builtinMetadata.minValue = lo;
            coldData.builtinMetadata.maxValue = hi;
            coldData.builtinMetadata.hasRange = true;
            return *this;
        }

        FieldBuilder& UI(UI::Schema s) {
            if (plan != nullptr) {
                return *this;
            }
            coldData.uiSchema = std::move(s);
            return *this;
        }
        FieldBuilder& EditAnywhere()         { if (plan == nullptr) { field.flags |= PropertyFlags::EditAnywhere; } return *this; }
        FieldBuilder& ReadOnly()             { if (plan == nullptr) { field.flags |= PropertyFlags::ReadOnly; } return *this; }
        FieldBuilder& ScriptRead()           { if (plan == nullptr) { field.flags |= PropertyFlags::ScriptRead; } return *this; }
        FieldBuilder& ScriptWrite()          { if (plan == nullptr) { field.flags |= PropertyFlags::ScriptWrite; } return *this; }
        FieldBuilder& ScriptReadWrite()      { if (plan == nullptr) { field.flags |= PropertyFlags::ScriptReadWrite; } return *this; }
        FieldBuilder& Transient()            { if (plan == nullptr) { field.flags |= PropertyFlags::Transient; } return *this; }
        FieldBuilder& SaveGame()             { if (plan == nullptr) { field.flags |= PropertyFlags::SaveGame; } return *this; }
        FieldBuilder& FunctionSelect() {
            if (plan != nullptr) {
                return *this;
            }
            coldData.uiSchema = UI::FunctionSelector{};
            return *this;
        }
        FieldBuilder& DisplayName(shine::STextView dn) {
            if (plan != nullptr) {
                return *this;
            }
            field.SetDisplayName(dn);
            return *this;
        }

        /// Single template replaces five per-type Meta overloads (C++23).
        template <typename V>
        FieldBuilder& Meta(shine::STextView key, V&& value) {
            if (plan != nullptr) {
                return *this;
            }
            const auto metadataKey = Hash(key);
            auto metadataValue = MakeMetadataValue(std::forward<V>(value));
            if (!field.TrySetBuiltinMetadata(metadataKey, metadataValue)) {
                coldData.metadata.push_back({metadataKey, std::move(metadataValue)});
            }
            return *this;
        }

        /// Overload accepting a pre-computed MetadataKey (consteval-friendly).
        template <typename V>
        FieldBuilder& Meta(MetadataKey key, V&& value) {
            if (plan != nullptr) {
                return *this;
            }
            auto metadataValue = MakeMetadataValue(std::forward<V>(value));
            if (!field.TrySetBuiltinMetadata(key, metadataValue)) {
                coldData.metadata.push_back({key, std::move(metadataValue)});
            }
            return *this;
        }

        template <auto Cb>
        FieldBuilder& OnChange() {
            using Param = typename FnParamExtractor<decltype(Cb)>::type;
            field.onChangeFn = [](void* inst, const void* old) {
                auto* obj = static_cast<T*>(inst);
                if constexpr (std::is_same_v<Param, void>) {
                    (obj->*Cb)();
                } else {
                    if (old) (obj->*Cb)(*static_cast<const Param*>(old));
                }
            };
            return *this;
        }
    };

    template <auto MemberPtr>
    FieldBuilder RegisterFieldFromDSL(DSL::FieldDSLNode<MemberPtr> node) {
        using MType = typename DSL::FieldDSLNode<MemberPtr>::MemberType;
        using CType = typename DSL::FieldDSLNode<MemberPtr>::ClassType;

        const size_t fieldPlanIndex = fieldWriteIndex_ - fieldPlanBaseIndex_;
        const auto fieldDescriptor = emitLayout_->GetFieldDescriptor(fieldPlanIndex);
        const auto* fieldPlan = fieldDescriptor.plan;
        auto& f = fieldPlan != nullptr ? AcquireBoundFieldSlot() : AcquireFieldSlot();

        if (fieldPlan == nullptr) {
            f = FieldInfo{};
            auto fieldColdData = CreateFieldColdData(node.name, fieldDescriptor);
            f.nameHash  = Hash(node.name);
            f.flags     = PropertyFlags::None;
            f.typeId    = GetTypeId<MType>();
            f.owner     = ReflectionOwnerHandle{};
            f.offset    = ComputeOffset<CType, MType>(MemberPtr);
            f.size      = sizeof(MType);
            f.alignment = alignof(MType);
            f.isPod     = std::is_trivially_copyable_v<MType>;
            f.SetColdData(std::move(fieldColdData));

            if constexpr (is_sequence_container<MType>::value) {
                f.containerType = ContainerType::Sequence;
                f.containerTrait = container_trait_provider<MType>::get();
            } else if constexpr (is_associative_container<MType>::value) {
                f.containerType = ContainerType::Associative;
                f.containerTrait = container_trait_provider<MType>::get();
            }
        }

        // Optimized getter/setter - 零间接调用
        f.getterFn = [](const void* inst, void* out) {
            const auto& val = static_cast<const CType*>(inst)->*MemberPtr;
            if constexpr (std::is_trivially_copyable_v<MType>)
                std::memcpy(out, &val, sizeof(MType));
            else
                *static_cast<MType*>(out) = val;
        };

        f.setterFn = [](void* inst, const void* in) {
            if constexpr (std::is_trivially_copyable_v<MType>)
                std::memcpy(&(static_cast<CType*>(inst)->*MemberPtr), in, sizeof(MType));
            else
                static_cast<CType*>(inst)->*MemberPtr = *static_cast<const MType*>(in);
        };

        f.equalsFn = [](const void* a, const void* b, std::size_t) -> bool {
            if constexpr (requires(const MType& x, const MType& y) { x == y; })
                return *static_cast<const MType*>(a) == *static_cast<const MType*>(b);
            else
                return std::memcmp(a, b, sizeof(MType)) == 0;
        };

        f.copyFn = [](void* dst, const void* src, std::size_t) {
            if constexpr (std::is_trivially_copyable_v<MType>)
                std::memcpy(dst, src, sizeof(MType));
            else
                *static_cast<MType*>(dst) = *static_cast<const MType*>(src);
        };

        return FieldBuilder{f, *this, f.MutableColdData(), fieldPlan};
    }

    // ---- MethodBuilder (fluent API, C++23: unified Meta) --------------------

    struct MethodBuilder {
        MethodInfo&  method;
        TypeBuilder& builder;
        MethodColdData& coldData;
        const TypeRegistrationPlan::MethodPlan* plan = nullptr;

        MethodBuilder& ScriptCallable() { if (plan == nullptr) { method.flags |= FunctionFlags::ScriptCallable; } return *this; }
        MethodBuilder& EditorCallable() { if (plan == nullptr) { method.flags |= FunctionFlags::EditorCallable; } return *this; }

        template <typename V>
        MethodBuilder& Meta(shine::STextView key, V&& value) {
            if (plan != nullptr) {
                return *this;
            }
            auto metadataValue = MakeMetadataValue(std::forward<V>(value));
            coldData.metadata.push_back({Hash(key), std::move(metadataValue)});
            return *this;
        }

        /// Overload accepting a pre-computed MetadataKey (consteval-friendly).
        template <typename V>
        MethodBuilder& Meta(MetadataKey key, V&& value) {
            if (plan != nullptr) {
                return *this;
            }
            auto metadataValue = MakeMetadataValue(std::forward<V>(value));
            coldData.metadata.push_back({key, std::move(metadataValue)});
            return *this;
        }
    };

    template <auto MethodPtr>
    MethodBuilder RegisterMethodFromDSL(DSL::MethodDSLNode<MethodPtr> node) {
        using Traits = MethodTraits<decltype(MethodPtr)>;

        const size_t methodPlanIndex = methodWriteIndex_ - methodPlanBaseIndex_;
        const auto methodDescriptor = emitLayout_->GetMethodDescriptor(methodPlanIndex);
        const auto* methodPlan = methodDescriptor.plan;
        auto& m = methodPlan != nullptr ? AcquireBoundMethodSlot() : AcquireMethodSlot();

        if (methodPlan == nullptr) {
            m = MethodInfo{};
            auto methodColdData = CreateMethodColdData(node.name, methodDescriptor);
            m.nameHash   = Hash(node.name);
            m.flags      = FunctionFlags::None;
            m.returnType = GetTypeId<typename Traits::ReturnType>();
            m.owner      = ReflectionOwnerHandle{};
            m.SetColdData(std::move(methodColdData));
            auto& paramTypes = m.MutableParamTypes();
            paramTypes.reserve(Traits::Arity);
            [&]<std::size_t... I>(std::index_sequence<I...>) {
                ((paramTypes.push_back(
                    GetTypeId<std::tuple_element_t<I, typename Traits::ParamTuple>>())), ...);
            }(std::make_index_sequence<Traits::Arity>{});
        }

        // Generate type-safe invoke thunk that unpacks void** args
        m.invokeFn = [](void* inst, void** args, void* ret) {
            auto* obj = static_cast<T*>(inst);

            // Unpack args into correctly-typed values and call
            [&]<std::size_t... I>(std::index_sequence<I...>) {
                if constexpr (std::is_void_v<typename Traits::ReturnType>) {
                    (obj->*MethodPtr)(
                        *static_cast<std::remove_reference_t<std::tuple_element_t<I, typename Traits::ParamTuple>>*>(
                            args ? args[I] : nullptr)...);
                } else {
                    auto result = (obj->*MethodPtr)(
                        *static_cast<std::remove_reference_t<std::tuple_element_t<I, typename Traits::ParamTuple>>*>(
                            args ? args[I] : nullptr)...);
                    if (ret) *static_cast<typename Traits::ReturnType*>(ret) = result;
                }
            }(std::make_index_sequence<Traits::Arity>{});
        };

        return MethodBuilder{m, *this, m.MutableColdData(), methodPlan};
    }

    // ---- Enum registration --------------------------------------------------

    struct EnumPair { T value; shine::STextView name; };

    void Enums(std::initializer_list<EnumPair> entries) {
        info.isEnum = true;
        const auto enumPlans = emitLayout_->enumPlans;
        if (!enumPlans.empty() && enumPlans.size() == entries.size()) {
            enumWriteIndex_ += enumPlans.size();
            return;
        }

        for (const auto& runtimeEntry : entries) {
            auto& entry = AcquireEnumSlot();
            entry = EnumEntry{static_cast<int64_t>(runtimeEntry.value), InternReflectionText(runtimeEntry.name)};
        }
    }

private:
    void EmitPlannedLayout() {
        if (emitLayout_ == nullptr) {
            return;
        }

        for (size_t fieldIndex = 0; fieldIndex < emitLayout_->fieldPlans.size(); ++fieldIndex) {
            EmitPlannedField(fieldPlanBaseIndex_ + fieldIndex, emitLayout_->GetFieldDescriptor(fieldIndex));
        }

        for (size_t methodIndex = 0; methodIndex < emitLayout_->methodPlans.size(); ++methodIndex) {
            EmitPlannedMethod(methodPlanBaseIndex_ + methodIndex, emitLayout_->GetMethodDescriptor(methodIndex));
        }

        for (size_t enumIndex = 0; enumIndex < emitLayout_->enumPlans.size(); ++enumIndex) {
            EmitPlannedEnum(enumPlanBaseIndex_ + enumIndex, emitLayout_->GetEnumDescriptor(enumIndex));
        }
    }

    void EmitPlannedField(size_t fieldIndex, const TypeRegistrationPlan::EmitLayout::FieldEmitDescriptor& fieldDescriptor) {
        const auto* fieldPlan = fieldDescriptor.plan;
        if (fieldPlan == nullptr) {
            return;
        }

        auto& field = info.MutableFields()[fieldIndex];
        field = FieldInfo{};
        field.nameHash = fieldPlan->nameHash;
        field.flags = fieldPlan->flags;
        field.typeId = fieldPlan->typeId;
        field.containerType = fieldPlan->containerType;
        field.containerTrait = fieldPlan->containerTrait;
        field.owner = ReflectionOwnerHandle{};
        field.offset = fieldPlan->offset;
        field.size = fieldPlan->size;
        field.alignment = fieldPlan->alignment;
        field.isPod = fieldPlan->isPod;
        field.SetColdData(CreateFieldColdData(fieldPlan->name, fieldDescriptor));
    }

    void EmitPlannedMethod(size_t methodIndex, const TypeRegistrationPlan::EmitLayout::MethodEmitDescriptor& methodDescriptor) {
        const auto* methodPlan = methodDescriptor.plan;
        if (methodPlan == nullptr) {
            return;
        }

        auto& method = info.MutableMethods()[methodIndex];
        method = MethodInfo{};
        method.nameHash = methodPlan->nameHash;
        method.flags = methodPlan->flags;
        method.returnType = methodPlan->returnType;
        method.owner = ReflectionOwnerHandle{};
        method.SetColdData(CreateMethodColdData(methodPlan->name, methodDescriptor));
    }

    void EmitPlannedEnum(size_t enumIndex, const TypeRegistrationPlan::EmitLayout::EnumEmitDescriptor& enumDescriptor) {
        const auto* enumPlan = enumDescriptor.plan;
        if (enumPlan == nullptr) {
            return;
        }

        auto& entry = info.MutableEnumEntries()[enumIndex];
        entry = EnumEntry{enumPlan->value, enumPlan->name};
    }

    [[nodiscard]] ReflectionColdPtr<FieldColdData> CreateFieldColdData(
        shine::STextView fieldName,
        const TypeRegistrationPlan::EmitView::FieldEmitDescriptor& fieldDescriptor) {
        auto coldData = fieldBatch_ ? fieldBatch_.Create() : MakeReflectionColdData<FieldColdData>();
        const auto* fieldPlan = fieldDescriptor.plan;
        if (fieldPlan != nullptr) {
            coldData->name = fieldPlan->name;
            coldData->uiSchema = fieldPlan->uiSchema;
            coldData->builtinMetadata.category = fieldPlan->category;
            coldData->builtinMetadata.displayName = fieldPlan->displayName;
            coldData->builtinMetadata.editCondition = fieldPlan->editCondition;
            coldData->builtinMetadata.minValue = fieldPlan->minValue;
            coldData->builtinMetadata.maxValue = fieldPlan->maxValue;
            coldData->builtinMetadata.hasRange = fieldPlan->hasRange;
            CopyColdSpan(coldData->metadata, fieldDescriptor.runtimeMetadata);
            return coldData;
        }

        coldData->name = InternReflectionText(fieldName);
        return coldData;
    }

    [[nodiscard]] ReflectionColdPtr<MethodColdData> CreateMethodColdData(
        shine::STextView methodName,
        const TypeRegistrationPlan::EmitView::MethodEmitDescriptor& methodDescriptor) {
        auto coldData = methodBatch_ ? methodBatch_.Create() : MakeReflectionColdData<MethodColdData>();
        const auto* methodPlan = methodDescriptor.plan;
        if (methodPlan != nullptr) {
            coldData->name = methodPlan->name;
            CopyColdSpan(coldData->paramTypes, methodDescriptor.paramTypes);
            CopyColdSpan(coldData->metadata, methodDescriptor.runtimeMetadata);
            return coldData;
        }
        coldData->name = InternReflectionText(methodName);
        return coldData;
    }

    [[nodiscard]] const TypeRegistrationPlan::FieldPlan* GetFieldPlan(size_t fieldIndex) const noexcept {
        return emitLayout_->GetFieldPlan(fieldIndex);
    }

    [[nodiscard]] const TypeRegistrationPlan::MethodPlan* GetMethodPlan(size_t methodIndex) const noexcept {
        return emitLayout_->GetMethodPlan(methodIndex);
    }

    FieldInfo& AcquireBoundFieldSlot() {
        return info.MutableFields()[fieldWriteIndex_++];
    }

    MethodInfo& AcquireBoundMethodSlot() {
        return info.MutableMethods()[methodWriteIndex_++];
    }

    FieldInfo& AcquireFieldSlot() {
        auto& fields = info.MutableFields();
        if (fieldWriteIndex_ < fields.size()) {
            return fields[fieldWriteIndex_++];
        }
        fields.emplace_back();
        ++fieldWriteIndex_;
        return fields.back();
    }

    MethodInfo& AcquireMethodSlot() {
        auto& methods = info.MutableMethods();
        if (methodWriteIndex_ < methods.size()) {
            return methods[methodWriteIndex_++];
        }
        methods.emplace_back();
        ++methodWriteIndex_;
        return methods.back();
    }

    EnumEntry& AcquireEnumSlot() {
        auto& enumEntries = info.MutableEnumEntries();
        if (enumWriteIndex_ < enumEntries.size()) {
            return enumEntries[enumWriteIndex_++];
        }
        enumEntries.emplace_back();
        ++enumWriteIndex_;
        return enumEntries.back();
    }

    size_t fieldWriteIndex_ = 0;
    size_t methodWriteIndex_ = 0;
    size_t enumWriteIndex_ = 0;
    size_t fieldPlanBaseIndex_ = 0;
    size_t methodPlanBaseIndex_ = 0;
    size_t enumPlanBaseIndex_ = 0;
    bool fieldsPreSized_ = false;
    bool methodsPreSized_ = false;
    bool enumsPreSized_ = false;
    static const TypeRegistrationPlan& EmptyPlan() {
        static const TypeRegistrationPlan emptyPlan{};
        return emptyPlan;
    }

    const TypeRegistrationPlan::EmitLayout* emitLayout_ = nullptr;
    typename ReflectionColdPool<FieldColdData>::ContiguousBatch fieldBatch_;
    typename ReflectionColdPool<MethodColdData>::ContiguousBatch methodBatch_;
};

template <typename T>
struct TypeBuilderPlanCounter {
    using ObjectType = T;

    TypeRegistrationPlan& plan;

    explicit TypeBuilderPlanCounter(TypeRegistrationPlan& registrationPlan)
        : plan(registrationPlan) {}

    struct FieldBuilder {
        TypeRegistrationPlan* registrationPlan = nullptr;
        TypeRegistrationPlan::FieldPlan* plan = nullptr;

        FieldBuilder& Range(float lo, float hi) {
            if (plan != nullptr) {
                plan->hasRange = true;
                plan->minValue = lo;
                plan->maxValue = hi;
            }
            return *this;
        }
        FieldBuilder& UI(UI::Schema schema) {
            if (plan != nullptr) {
                plan->hasUISchema = true;
                plan->uiSchema = InternSchema(std::move(schema));
            }
            return *this;
        }
        FieldBuilder& EditAnywhere() { AddFlag(PropertyFlags::EditAnywhere); return *this; }
        FieldBuilder& ReadOnly() { AddFlag(PropertyFlags::ReadOnly); return *this; }
        FieldBuilder& ScriptRead() { AddFlag(PropertyFlags::ScriptRead); return *this; }
        FieldBuilder& ScriptWrite() { AddFlag(PropertyFlags::ScriptWrite); return *this; }
        FieldBuilder& ScriptReadWrite() { AddFlag(PropertyFlags::ScriptReadWrite); return *this; }
        FieldBuilder& Transient() { AddFlag(PropertyFlags::Transient); return *this; }
        FieldBuilder& SaveGame() { AddFlag(PropertyFlags::SaveGame); return *this; }
        FieldBuilder& FunctionSelect() {
            if (plan != nullptr) {
                plan->hasUISchema = true;
                plan->uiSchema = UI::FunctionSelector{};
            }
            return *this;
        }
        FieldBuilder& DisplayName(shine::STextView name) {
            if (plan != nullptr) {
                plan->hasDisplayName = true;
                plan->displayName = InternReflectionText(name);
            }
            return *this;
        }

        template <typename V>
        FieldBuilder& Meta(shine::STextView key, V&& value) {
            RecordMetadata(Hash(key), MakeMetadataValue(std::forward<V>(value)));
            return *this;
        }

        template <typename V>
        FieldBuilder& Meta(MetadataKey key, V&& value) {
            RecordMetadata(key, MakeMetadataValue(std::forward<V>(value)));
            return *this;
        }

        template <auto Cb>
        FieldBuilder& OnChange() { return *this; }

    private:
        void AddFlag(PropertyFlags flag) {
            if (plan != nullptr) {
                plan->flags |= flag;
            }
        }

        static UI::Schema InternSchema(UI::Schema schema) {
            if (auto* filePicker = std::get_if<UI::FilePicker>(&schema)) {
                filePicker->filter = InternReflectionText(filePicker->filter);
            }
            return schema;
        }

        void RecordMetadata(MetadataKey key, MetadataValue value) {
            if (plan == nullptr) {
                return;
            }

            if (key == MetaKeys::Category) {
                plan->hasCategory = true;
                plan->category = ExtractText(value);
                return;
            }
            if (key == MetaKeys::DisplayName) {
                plan->hasDisplayName = true;
                plan->displayName = ExtractText(value);
                return;
            }
            if (key == MetaKeys::EditCondition) {
                plan->hasEditCondition = true;
                plan->editCondition = ExtractText(value);
                return;
            }
            if (key == MetaKeys::Min) {
                plan->hasRange = true;
                plan->minValue = ExtractFloat(value, plan->minValue);
                return;
            }
            if (key == MetaKeys::Max) {
                plan->hasRange = true;
                plan->maxValue = ExtractFloat(value, plan->maxValue);
                return;
            }

            if (plan->runtimeMetadataCount == 0 && registrationPlan != nullptr) {
                plan->runtimeMetadataOffset = registrationPlan->runtimeMetadataEntries.size();
            }

            ++plan->runtimeMetadataCount;
            if (registrationPlan != nullptr) {
                registrationPlan->runtimeMetadataEntries.push_back({key, NormalizeMetadataValue(std::move(value))});
            }
        }

        [[nodiscard]] static shine::STextView ExtractText(const MetadataValue& value) {
            if (const auto* text = std::get_if<shine::STextView>(&value)) {
                return InternReflectionText(*text);
            }
            if (const auto* stringValue = std::get_if<shine::SString>(&value)) {
                return InternReflectionText(*stringValue);
            }
            return {};
        }

        [[nodiscard]] static float ExtractFloat(const MetadataValue& value, float fallback) {
            if (const auto* floatValue = std::get_if<float>(&value)) {
                return *floatValue;
            }
            if (const auto* doubleValue = std::get_if<double>(&value)) {
                return static_cast<float>(*doubleValue);
            }
            if (const auto* intValue = std::get_if<int>(&value)) {
                return static_cast<float>(*intValue);
            }
            return fallback;
        }

        [[nodiscard]] static MetadataValue NormalizeMetadataValue(MetadataValue value) {
            if (const auto* text = std::get_if<shine::STextView>(&value)) {
                return MetadataValue{InternReflectionText(*text)};
            }
            if (const auto* stringValue = std::get_if<shine::SString>(&value)) {
                return MetadataValue{InternReflectionText(*stringValue)};
            }
            return value;
        }

        [[nodiscard]] static bool IsBuiltinFieldMetadataKey(MetadataKey key) noexcept {
            return key == MetaKeys::Category
                || key == MetaKeys::DisplayName
                || key == MetaKeys::EditCondition
                || key == MetaKeys::Min
                || key == MetaKeys::Max;
        }
    };

    struct MethodBuilder {
        MethodInfo method{};
        TypeRegistrationPlan* registrationPlan = nullptr;
        TypeRegistrationPlan::MethodPlan* plan = nullptr;

        MethodBuilder& ScriptCallable() {
            if (plan != nullptr) {
                plan->flags |= FunctionFlags::ScriptCallable;
            }
            return *this;
        }
        MethodBuilder& EditorCallable() {
            if (plan != nullptr) {
                plan->flags |= FunctionFlags::EditorCallable;
            }
            return *this;
        }

        template <typename V>
        MethodBuilder& Meta(shine::STextView key, V&& value) {
            if (plan != nullptr) {
                if (plan->runtimeMetadataCount == 0 && registrationPlan != nullptr) {
                    plan->runtimeMetadataOffset = registrationPlan->runtimeMetadataEntries.size();
                }
                ++plan->runtimeMetadataCount;
                if (registrationPlan != nullptr) {
                    registrationPlan->runtimeMetadataEntries.push_back({Hash(key), NormalizeMetadataValue(MakeMetadataValue(std::forward<V>(value)))});
                }
            }
            return *this;
        }

        template <typename V>
        MethodBuilder& Meta(MetadataKey key, V&& value) {
            if (plan != nullptr) {
                if (plan->runtimeMetadataCount == 0 && registrationPlan != nullptr) {
                    plan->runtimeMetadataOffset = registrationPlan->runtimeMetadataEntries.size();
                }
                ++plan->runtimeMetadataCount;
                if (registrationPlan != nullptr) {
                    registrationPlan->runtimeMetadataEntries.push_back({key, NormalizeMetadataValue(MakeMetadataValue(std::forward<V>(value)))});
                }
            }
            return *this;
        }

    private:
        [[nodiscard]] static MetadataValue NormalizeMetadataValue(MetadataValue value) {
            if (const auto* text = std::get_if<shine::STextView>(&value)) {
                return MetadataValue{InternReflectionText(*text)};
            }
            if (const auto* stringValue = std::get_if<shine::SString>(&value)) {
                return MetadataValue{InternReflectionText(*stringValue)};
            }
            return value;
        }
    };

    template <auto MemberPtr>
    FieldBuilder RegisterFieldFromDSL(const DSL::FieldDSLNode<MemberPtr>& node) {
        using MType = typename DSL::FieldDSLNode<MemberPtr>::MemberType;
        using CType = typename DSL::FieldDSLNode<MemberPtr>::ClassType;

        ++plan.fieldCount;
        auto& fieldPlan = plan.fieldPlans.emplace_back();
        fieldPlan.name = InternReflectionText(node.name);
        fieldPlan.nameHash = Hash(node.name);
        fieldPlan.typeId = GetTypeId<MType>();
        fieldPlan.offset = ComputeOffset<CType, MType>(MemberPtr);
        fieldPlan.size = sizeof(MType);
        fieldPlan.alignment = alignof(MType);
        fieldPlan.isPod = std::is_trivially_copyable_v<MType>;
        if constexpr (is_sequence_container<MType>::value) {
            fieldPlan.containerType = ContainerType::Sequence;
            fieldPlan.containerTrait = container_trait_provider<MType>::get();
        } else if constexpr (is_associative_container<MType>::value) {
            fieldPlan.containerType = ContainerType::Associative;
            fieldPlan.containerTrait = container_trait_provider<MType>::get();
        }
        return FieldBuilder{&plan, &fieldPlan};
    }

    template <auto MethodPtr>
    MethodBuilder RegisterMethodFromDSL(const DSL::MethodDSLNode<MethodPtr>& node) {
        using Traits = MethodTraits<decltype(MethodPtr)>;

        ++plan.methodCount;
        auto& methodPlan = plan.methodPlans.emplace_back();
        methodPlan.name = InternReflectionText(node.name);
        methodPlan.nameHash = Hash(node.name);
        methodPlan.returnType = GetTypeId<typename Traits::ReturnType>();
        methodPlan.paramTypeOffset = plan.methodParamTypeEntries.size();
        methodPlan.paramTypeCount = Traits::Arity;
        [&]<std::size_t... I>(std::index_sequence<I...>) {
            (plan.methodParamTypeEntries.push_back(
                GetTypeId<std::tuple_element_t<I, typename Traits::ParamTuple>>()), ...);
        }(std::make_index_sequence<Traits::Arity>{});
        return MethodBuilder{MethodInfo{}, &plan, &methodPlan};
    }

    struct EnumPair { T value; shine::STextView name; };

    void Enums(std::initializer_list<EnumPair> entries) {
        plan.enumCount += entries.size();
        for (const auto& entry : entries) {
            plan.enumPlans.push_back(TypeRegistrationPlan::EnumPlan{
                static_cast<int64_t>(entry.value),
                InternReflectionText(entry.name)
            });
        }
    }
};

template <typename T>
class TypeRegistrationGraph {
public:
    using ObjectType = T;

    explicit TypeRegistrationGraph(shine::STextView typeName, bool isEnum = std::is_enum_v<T>)
        : typeName_(InternReflectionText(typeName))
        , isEnum_(isEnum) {}

    template <typename RegisterFn>
    void Measure(RegisterFn&& registerFn) {
        plan_.Reset();
        TypeBuilderPlanCounter<T> counter(plan_);
        std::forward<RegisterFn>(registerFn)(counter);
        plan_.FreezeSharedBlocks();
        measured_ = true;
    }

    template <typename RegisterFn>
    [[nodiscard]] TypeInfo BuildTypeInfo(RegisterFn&& registerFn) const {
        TypeInfo info{};
        info.id = GetTypeId<T>();
        info.SetName(typeName_);
        info.size = sizeof(T);
        info.alignment = alignof(T);
        info.isPod = std::is_trivially_copyable_v<T>;
        info.isEnum = isEnum_;

        TypeBuilder<T> builder(info, plan_);
        std::forward<RegisterFn>(registerFn)(builder);
        return info;
    }

    [[nodiscard]] const TypeRegistrationPlan& Plan() const noexcept {
        return plan_;
    }

    [[nodiscard]] shine::STextView GetTypeName() const noexcept {
        return typeName_;
    }

    [[nodiscard]] bool IsMeasured() const noexcept {
        return measured_;
    }

private:
    shine::STextView typeName_;
    bool isEnum_ = false;
    TypeRegistrationPlan plan_{};
    bool measured_ = false;
};

template <typename T, typename RegisterFn>
[[nodiscard]] TypeRegistrationGraph<T> BuildTypeRegistrationGraph(shine::STextView typeName, RegisterFn&& registerFn, bool isEnum = std::is_enum_v<T>) {
    TypeRegistrationGraph<T> graph(typeName, isEnum);
    graph.Measure(std::forward<RegisterFn>(registerFn));
    return graph;
}

// =============================================================================
// FastMethodCall — Optimized method invocation with C++ index sequences
// =============================================================================

namespace detail {

// 辅助模板：移除引用
template<typename T>
using RemoveRef = std::remove_reference_t<T>;

// 便捷函数模板 - 使用模板参数传递方法指针，支持任意参数
template<auto MethodPtr>
[[gnu::always_inline]] inline void FastMethodCall(void* inst, void** args, void* ret) {
    using MP = decltype(MethodPtr);
    using Traits = MethodTraits<MP>;
    using ClassType = typename Traits::ClassType;
    using ReturnType = typename Traits::ReturnType;
    
    auto* obj = static_cast<ClassType*>(inst);
    
    [&]<std::size_t... I>(std::index_sequence<I...>) {
        if constexpr (std::is_void_v<ReturnType>) {
            (obj->*MethodPtr)(
                *static_cast<RemoveRef<std::tuple_element_t<I, typename Traits::ParamTuple>>*>(
                    args ? args[I] : nullptr)...);
        } else {
            auto result = (obj->*MethodPtr)(
                *static_cast<RemoveRef<std::tuple_element_t<I, typename Traits::ParamTuple>>*>(
                    args ? args[I] : nullptr)...);
            if (ret) *static_cast<ReturnType*>(ret) = result;
        }
    }(std::make_index_sequence<Traits::Arity>{});
}

} // namespace detail

// =============================================================================
// 注册方法时使用优化的调用器
// =============================================================================

// 使用示例:
// builder.RegisterMethodFast< &MyClass::MyMethod >("MyMethod")

template <auto MethodPtr>
struct FastMethodRegistration {
    template<typename T>
    static auto Register(T& builder, shine::STextView name) {
        //using MP = decltype(MethodPtr);
        
        // 使用 DSL 创建方法节点
        auto mb = builder.RegisterMethodFromDSL(
            DSL::MakeMethodDSL<MethodPtr>(name));
        
        // 替换为优化的调用器
        mb.method.invokeFn = &detail::FastMethodCall<MethodPtr>;
        
        return mb;  // 返回 MethodBuilder 支持链式调用
    }
};

// =============================================================================
// BuildTypeInfo — constructs a basic TypeInfo at runtime init
// =============================================================================

template <typename T>
constexpr TypeInfo BuildTypeInfo(shine::STextView name) {
    TypeInfo i{};
    i.id        = GetTypeId<T>();
    i.SetName(name);
    i.size      = sizeof(T);
    i.alignment = alignof(T);
    i.isPod     = std::is_trivially_copyable_v<T>;
    i.isEnum    = std::is_enum_v<T>;
    return i;
}

// Legacy alias
template <typename T>
constexpr TypeInfo BuildTypeInfoCT(shine::STextView name) { return BuildTypeInfo<T>(name); }

} // namespace shine::reflection
