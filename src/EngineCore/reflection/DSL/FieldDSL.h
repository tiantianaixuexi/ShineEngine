#pragma once

// =============================================================================
// FieldDSL.h — Field Domain Specific Language for Reflection
// =============================================================================
//
// Provides fluent API for field registration and metadata configuration.
// Used by REFLECT_FIELD macro to create declarative field descriptors.
//
// C++23 / MSVC
// =============================================================================

#include "../ReflectionCore.h"
#include "MethodDSL.h"

namespace shine::reflection {

// Helper: extract class + member type from member-pointer
template <typename T> struct MemberPtrInfo;
template <typename C, typename M>
struct MemberPtrInfo<M C::*> { using ClassType = C; using MemberType = M; };

// Compute member offset from member pointer (MSVC-compatible)
template <typename C, typename M>
inline std::size_t ComputeOffset(M C::* ptr) {
    return static_cast<std::size_t>(
        reinterpret_cast<const char*>(&(reinterpret_cast<C*>(0)->*ptr)) - reinterpret_cast<const char*>(0));
}

namespace DSL {

// ---- FieldDescriptor --------------------------------------------------------

struct FieldDescriptorBase {
    shine::STextView  name;
    UI::Schema        uiSchema = UI::None{};
    MetadataContainer metadata;
    OnChangeFn        onChange  = nullptr;
};

template <std::size_t = 0>
struct FieldDescriptor : FieldDescriptorBase {};

// ---- FieldDSLNode (created by REFLECT_FIELD macro) --------------------------

template <auto MemberPtr>
struct FieldDSLNode {
    using Info       = MemberPtrInfo<decltype(MemberPtr)>;
    using ClassType  = typename Info::ClassType;
    using MemberType = typename Info::MemberType;

    static constexpr auto MemberPtrValue = MemberPtr;

    shine::STextView   name;
    FieldDescriptor<0> desc;

    explicit FieldDSLNode(shine::STextView n) : name(n) { desc.name = n; }

    // Chaining (returns copy — enables StaticInspector DSL compatibility)
    auto EditAnywhere()    const { return *this; }
    auto ReadOnly()        const { return *this; }
    auto ScriptReadWrite() const { return *this; }

    template <typename S>
    auto UI(S&& schema) const {
        auto c = *this;
        c.desc.uiSchema = std::forward<S>(schema);
        return c;
    }

    template <typename V>
    auto Meta(shine::STextView key, V&& val) const {
        auto c = *this;
        c.desc.metadata.push_back({Hash(key), MakeMetadataValue(std::forward<V>(val))});
        return c;
    }

    /// Overload accepting a pre-computed MetadataKey (consteval-friendly).
    template <typename V>
    auto Meta(MetadataKey key, V&& val) const {
        auto c = *this;
        c.desc.metadata.push_back({key, MakeMetadataValue(std::forward<V>(val))});
        return c;
    }

    template <typename V>
    auto Range(V lo, V hi) const {
        auto c = *this;
        c.desc.metadata.push_back({MetaKeys::Min, MetadataValue{static_cast<float>(lo)}});
        c.desc.metadata.push_back({MetaKeys::Max, MetadataValue{static_cast<float>(hi)}});
        return c;
    }

    auto FunctionSelect(bool onlyScript = true) const {
        auto c = *this;
        c.desc.uiSchema = shine::reflection::UI::FunctionSelector{onlyScript};
        return c;
    }

    auto DisplayName(shine::STextView dn) const {
        auto c = *this;
        c.desc.metadata.push_back({MetaKeys::DisplayName, MetadataValue{dn}});
        return c;
    }

    template <auto Cb>
    auto OnChange() const {
        using Param = typename FnParamExtractor<decltype(Cb)>::type;
        auto c = *this;
        c.desc.onChange = [](void* inst, const void* old) {
            auto* obj = static_cast<ClassType*>(inst);
            if constexpr (std::is_same_v<Param, void>) {
                (obj->*Cb)();
            } else {
                if (old) (obj->*Cb)(*static_cast<const Param*>(old));
            }
        };
        return c;
    }
};

} // namespace DSL
} // namespace shine::reflection