#pragma once


namespace shine::reflection
{

    template <typename T, typename DSLType>
    struct MethodInjector_Impl {
        TypeBuilder<T> &builder;
        DSLType         dsl;
        bool            moved = false;
        MethodInjector_Impl(TypeBuilder<T> &b, DSLType d) : builder(b), dsl(d) {}

        // Fix: Call RegisterMethodImpl directly to avoid recursion
        ~MethodInjector_Impl() {
            if (!moved) {
                builder.template RegisterMethodImpl<typename DSLType::ClassType, DSLType::MethodPtr, typename DSLType::ReturnType, typename DSLType::ArgsTuple>(dsl.desc);
            }
        }
        MethodInjector_Impl(MethodInjector_Impl &&other) : builder(other.builder), dsl(other.dsl) { other.moved = true; }
        auto ScriptCallable() { return Chain(dsl.ScriptCallable()); }
        auto EditorCallable() { return Chain(dsl.EditorCallable()); }
        template <typename V>
        auto Meta(std::string_view key, V &&val) { return Chain(dsl.Meta(Hash(key), std::forward<V>(val))); }
        template <size_t N>
        auto DisplayName(const char (&name)[N]) { return Meta("DisplayName", name); }
        auto DisplayName(std::string_view name) { return Meta("DisplayName", name); }

    private:
        template <typename NewDSL>
        auto Chain(NewDSL newDSL) {
            moved = true;
            return MethodInjector_Impl<T, NewDSL>(builder, newDSL);
        }
    };

} // namespace shine::reflection