#pragma once


#include "../ReflectionConcept.h"


namespace shine::reflection {




template <typename T, typename DSLType>
struct Reflection_FieldConllect {

    TypeBuilder<T> &builder;
    DSLType         dsl;
    bool            moved = false;

    constexpr ReflectionFlag flag = ReflectionFlag::None;

    constexpr Reflection_FieldConllect(TypeBuilder<T> &b, DSLType d) : builder(b), dsl(d) {}

    ~Reflection_FieldConllect() {
        if (!moved) {
            builder.template RegisterFieldImpl<typename DSLType::ClassType, typename DSLType::traits, DSLType::traits>(dsl.desc);
        }
    }


    // 注册所有的 flags
    // 使用逗号表达式自动展开
    template <ReflectinFlagConcept... F>
    constexpr auto RegisterFlags(F... value) noexcept {

       ((flag &=value),...);

    }

    // 注册 Meta
    template <ReflectionMetaFlagConcept F>
    constexpr auto RegisterMetaFlag(F key,)

    constexpr Reflection_FieldConllect(Reflection_FieldConllect &&other) : builder(other.builder), dsl(other.dsl) { other.moved = true; }

    constexpr auto FunctionSelect(bool onlyScriptCallable = true) { return Chain(dsl.FunctionSelect(onlyScriptCallable)); }

    template <auto CallbackPtr>
    constexpr auto OnChange() {
        return Chain(dsl.template OnChange<CallbackPtr>());
    }

    template <typename U>
    constexpr auto UI(U &&schema) { return Chain(dsl.UI(std::forward<U>(schema))); }

    template <size_t N, typename V>
    constexpr auto Meta(const char (&key)[N], V &&val) {
        return Chain(dsl.Meta(Hash(key), std::forward<V>(val)));
    }

    // Fallback for runtime strings
    template <typename V>
    constexpr auto Meta(std::string_view key, V &&val) {
        return Chain(dsl.Meta(Hash(key), std::forward<V>(val)));
    }

    template <typename V>
        requires(IsNumberAndFloat<V>)
    constexpr auto Range(V min, V max) { return Chain(dsl.Range(min, max)); }

    template <size_t N>
    constexpr auto DisplayName(const char (&name)[N]) { return Meta("DisplayName", name); }
    constexpr auto DisplayName(std::string_view name) { return Meta("DisplayName", name); }

    template <size_t N>
    constexpr auto Category(const char (&name)[N]) { return Meta("Category", name); }
    constexpr auto Category(std::string_view name) { return Meta("Category", name); }

    template <size_t N>
    constexpr auto EditCondition(const char (&condition)[N]) { return Meta("EditCondition", condition); }
    constexpr auto EditCondition(std::string_view condition) { return Meta("EditCondition", condition); }

private:
    template <typename NewDSL>
    constexpr auto Chain(NewDSL newDSL) {
        moved = true;
        return Reflection_FieldConllect<T, NewDSL>(builder, newDSL);
    }
};


}