#pragma once

// =============================================================================
// MethodDSL.h — Method Domain Specific Language for Reflection
// =============================================================================
//
// Provides fluent API for method registration and metadata configuration.
// Used by REFLECT_METHOD macro to create declarative method descriptors.
//
// C++23 / MSVC
// =============================================================================

#include "../ReflectionCore.h"
#include <string_view>

namespace shine::reflection {

// Helper: extract first parameter type from member-function pointer
template <typename T> struct FnParamExtractor;
template <typename C, typename R, typename P>
struct FnParamExtractor<R(C::*)(P)> { using type = P; };
template <typename C, typename R>
struct FnParamExtractor<R(C::*)()>  { using type = void; };

// Helper: decompose a member-function pointer into return type, class, and parameter pack
template <typename T> struct MethodTraits;

template <typename C, typename R, typename... Args>
struct MethodTraits<R(C::*)(Args...)> {
    using ClassType  = C;
    using ReturnType = R;
    using ParamTuple = std::tuple<Args...>;
    static constexpr std::size_t Arity = sizeof...(Args);
};

template <typename C, typename R, typename... Args>
struct MethodTraits<R(C::*)(Args...) const> {
    using ClassType  = C;
    using ReturnType = R;
    using ParamTuple = std::tuple<Args...>;
    static constexpr std::size_t Arity = sizeof...(Args);
};

namespace DSL {

// ---- MethodDescriptor -------------------------------------------------------

struct MethodDescriptorBase {
    std::string_view  name;
    MetadataContainer metadata;
};

template <std::size_t = 0>
struct MethodDescriptor : MethodDescriptorBase {};

// ---- MethodDSLNode (created by REFLECT_METHOD macro) ------------------------

template <auto MethodPtr>
struct MethodDSLNode {
    static constexpr auto MethodPtrValue = MethodPtr;

    std::string_view    name;
    MethodDescriptor<0> desc;

    explicit MethodDSLNode(std::string_view n) : name(n) { desc.name = n; }

    auto ScriptCallable() const { return *this; }
    auto EditorCallable() const { return *this; }

    template <typename V>
    auto Meta(std::string_view key, V&& val) const {
        auto c = *this;
        c.desc.metadata.push_back({Hash(key), MetadataValue{std::forward<V>(val)}});
        return c;
    }

    /// Overload accepting a pre-computed MetadataKey (consteval-friendly).
    template <typename V>
    auto Meta(MetadataKey key, V&& val) const {
        auto c = *this;
        c.desc.metadata.push_back({key, MetadataValue{std::forward<V>(val)}});
        return c;
    }
};

template <auto M>
MethodDSLNode<M> MakeMethodDSL(std::string_view n) { return MethodDSLNode<M>{n}; }

} // namespace DSL
} // namespace shine::reflection