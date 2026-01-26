#pragma once


#include <type_traits>

#include "EngineCore/reflection/Reflection.h"

namespace shine::reflection {



    


// Define TypeBuilder first (but with forward declared methods if needed, or inline)
    template <typename T>
    struct TypeBuilder {

        using ObjectType = std::remove_reference_t<T>;
        TypeInfo info;

        template <typename F>
        constexpr auto CreateInstanceFunction(F f, T &Builder) {
            return [f](auto &&...args) constexpr {
                // before
                return f(std::forward<decltype(args)>(args)...);
                // after
            };
        }

        constexpr TypeBuilder(std::string_view name) {

            info.name      = name;
            info.id        = GetTypeId<T>();
            info.size      = sizeof(T);
            info.alignment = alignof(T);

            info.create = []() -> void * {
                co::MemoryScope scope(co::MemoryTag::Reflection);
                return new T();
            };
            info.destroy = [](void *ptr) {
                co::MemoryScope scope(co::MemoryTag::Reflection);
                delete static_cast<T *>(ptr);
            };
            info.construct = [](void *ptr) {
                co::MemoryScope scope(co::MemoryTag::Reflection);
                new (ptr) T();
            };
            info.destruct = [](void *ptr) {
                co::MemoryScope scope(co::MemoryTag::Reflection);
                static_cast<T *>(ptr)->~T();
            };
            info.copy = [](void *dst, const void *src) {
                co::MemoryScope scope(co::MemoryTag::Reflection);
                if constexpr (std::is_copy_assignable_v<T>) {
                    *static_cast<T *>(dst) = *static_cast<const T *>(src);
                } else if constexpr (std::is_same_v<T, SString>) {
                    // SString is move-only, manual deep copy for reflection
                    auto *d = static_cast<T *>(dst);
                    auto *s = static_cast<const T *>(src);
                    d->clear();
                    d->append(s->view());
                }
            };
            info.isTrivial = std::is_trivially_copyable_v<T>;
        }
        template <typename BaseType>
        void Base() { info.baseTypeId = GetTypeId<BaseType>(); }
        void Managed() { info.isManaged = true; }
        void Enum(std::string_view name, int64_t value) {
            info.isEnum = true;
            info.enumEntries.push_back({value, name});
        }

        // Convenience method for batch registration
        // Usage: builder.Enums({ {Enum::Val, "Name"}, ... });
        template <typename EnumType>
        void Enums(std::initializer_list<std::pair<EnumType, std::string_view>> items) {
            info.isEnum = true;
            for (const auto &[val, name] : items) {
                info.enumEntries.push_back({static_cast<int64_t>(val), name});
            }
        }

        // Fix: Allow deduction guides or simpler overload if template deduction fails
        void Enums(std::initializer_list<std::pair<T, std::string_view>> items) {
            info.isEnum = true;
            for (const auto &[val, name] : items) {
                info.enumEntries.push_back({static_cast<int64_t>(val), name});
            }
        }

        template <typename DSLType>
        constexpr auto RegisterMethodFromDSL(const DSLType &dsl);

        template <typename DSLType>
        constexpr auto RegisterFieldFromDSL(const DSLType &dsl);

        void Register() { GetPendingTypes().push_back(std::move(info)); }

    private:
        template <typename Class, typename MemberType, MemberType Class::*Ptr, size_t N>
        void RegisterFieldImpl(const DSL::FieldDescriptor<N> &desc) {
            FieldInfo field;
            field.name      = desc.name;
            field.typeId    = desc.typeId;
            field.offset    = desc.offset;
            field.size      = desc.size;
            field.alignment = desc.alignment;
            field.isPod     = desc.isPod;
            field.flags     = desc.flags;
            field.uiSchema  = desc.uiSchema;
            field.onChange  = desc.onChange;
            for (const auto &m : desc.metadata) {
                auto it = std::lower_bound(field.metadata.begin(), field.metadata.end(), m.key, [](const auto &pair, TypeId k) { return pair.first < k; });
                field.metadata.insert(it, {m.key, m.value});
            }
            if constexpr (IsVector<MemberType>) {
                field.containerType  = ContainerType::Sequence;
                field.containerTrait = &VectorThunks<MemberType>::trait;
            } else if constexpr (IsList<MemberType>) {
                field.containerType  = ContainerType::Sequence;
                field.containerTrait = &ListThunks<MemberType>::trait;
            } else if constexpr (IsMap<MemberType> || IsUnorderedMap<MemberType>) {
                field.containerType  = ContainerType::Associative;
                field.containerTrait = &MapThunks<MemberType>::trait;
            }

            if (field.isPod) {
                field.getter = MemcpyGetter;
                field.setter = MemcpySetter;
                field.equals = [](const void *a, const void *b, size_t size) { return std::memcmp(a, b, size) == 0; };
                field.copy   = [](void *dst, const void *src, size_t size) { std::memcpy(dst, src, size); };
            } else {
                field.getter = GenericGetter<Class, MemberType, Ptr>;
                field.setter = GenericSetter<Class, MemberType, Ptr>;
                field.equals = nullptr;
                field.copy   = nullptr;
            }
            info.fields.push_back(std::move(field));
        }

        template <typename Class, auto MethodPtr, typename Ret, typename ArgsTuple, size_t NArgs, size_t NMeta>
        void RegisterMethodImpl(const DSL::MethodDescriptor<NArgs, NMeta> &desc) {
            std::apply([&](auto... args) { RegisterMethodThunkImpl<Class, MethodPtr, Ret, decltype(args)...>(desc); }, ArgsTuple{});
        }

        template <typename Class, auto MethodPtr, typename Ret, typename... Args, size_t NArgs, size_t NMeta>
        void RegisterMethodThunkImpl(const DSL::MethodDescriptor<NArgs, NMeta> &desc) {
            MethodInfo method;
            method.name          = desc.name;
            method.returnType    = desc.returnType;
            method.paramTypes    = {desc.paramTypes.begin(), desc.paramTypes.end()};
            method.signatureHash = desc.signatureHash;
            method.flags         = desc.flags;
            for (const auto &m : desc.metadata)
                method.metadata.push_back({m.key, m.value});
            std::sort(method.metadata.begin(), method.metadata.end(), [](auto &a, auto &b) { return a.first < b.first; });
            GenerateInvokeHelper<MethodPtr, Class, Ret, Args...>(method);
            info.methods.push_back(std::move(method));
        }

        template <auto Func, typename Class, typename Ret, typename... Args>
        void GenerateInvokeHelper(MethodInfo &outMethod) {
            outMethod.invoke = [](void *inst, void **args, void *ret) {
                InvokeThunkImpl<Class, Ret, Args...>(static_cast<Class *>(inst), Func, args, ret, std::make_index_sequence<sizeof...(Args)>{});
            };
            if constexpr (std::is_const_v<std::remove_pointer_t<decltype(Func)>>) {
                // Detect const member function... actually decltype(Func) is Ret(Class::*)(Args...) const
                // We can check if it matches the const signature
                // Or we can rely on overload resolution of InvokeThunkImpl
            }
            // Add Const flag if needed. But we need to detect it.
            // Helper:
            if constexpr (IsConstMember<decltype(Func)>::value) {
                outMethod.flags = outMethod.flags | FunctionFlags::Const;
            }
        }

        template <typename F>
        struct IsConstMember : std::false_type {};
        template <typename C, typename R, typename... A>
        struct IsConstMember<R (C::*)(A...) const> : std::true_type {};

        friend struct FieldInjector_Impl<T, void>; // Partial friend (simplified)
        template <typename U, typename D>
        friend struct FieldInjector_Impl;
        template <typename U, typename D>
        friend struct MethodInjector_Impl;
    };



    template <typename T, typename DSLType>
    struct FiledRegister {
        TypeBuilder<T> &builder;
        DSLType         dsl;
        bool            moved = false;

        constexpr FiledRegister(TypeBuilder<T> &b, DSLType d) : builder(b), dsl(d) {}

        constexpr FiledRegister(FiledRegister &&other) noexcept : builder(other.builder), dsl(other.dsl) { other.moved = true; }

        constexpr auto EditAnywhere() { return Chain(dsl.EditAnywhere()); }

    private:
        template <typename NewDSL>
        constexpr auto Chain(NewDSL newDSL) {
            moved = true;
            return FiledRegister<T, NewDSL>(builder, newDSL);
        }
    };

}

