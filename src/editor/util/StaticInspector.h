#pragma once
#include "EngineCore/reflection/Reflection.h"
#include "PropertyDrawer.h"
#include "imgui/imgui.h"
#include <variant>

namespace shine::editor::util {

    // 编译期静态检查器构建器
    // 这个类在编译期被 REFLECT_STRUCT 实例化，直接生成绘制代码
    template<typename T>
    struct StaticInspectorBuilder {
        using ObjectType = T;
        T* instance;
        const char* currentCategory = nullptr; // Track current category

        StaticInspectorBuilder(T* inst) : instance(inst) {}

        // Mock DSL consumers that just chain
        template<typename DSLType>
        struct FieldProxy {
            StaticInspectorBuilder& builder;
            DSLType dsl;
            bool moved = false;
            
            // Constructor
            FieldProxy(StaticInspectorBuilder& b, DSLType d) : builder(b), dsl(d) {}
            // Move Constructor
            FieldProxy(FieldProxy&& other) : builder(other.builder), dsl(other.dsl) { other.moved = true; }

            // Chaining methods: Return a NEW proxy with NEW DSL type and mark current as moved
            auto EditAnywhere() { return Chain(dsl.EditAnywhere()); }
            auto ReadOnly() { return Chain(dsl.ReadOnly()); }
            auto ScriptReadWrite() { return Chain(dsl.ScriptReadWrite()); }
            template<typename U> auto UI(U&& schema) { return Chain(dsl.UI(std::forward<U>(schema))); }
            template<typename V> auto Meta(std::string_view key, V&& val) { return Chain(dsl.Meta(key, std::forward<V>(val))); }
            // Fix: Add constexpr Meta with string literal capture
            template<size_t N, typename V> 
            constexpr auto Meta(const char (&key)[N], V&& val) { 
                return Chain(dsl.Meta(key, std::forward<V>(val))); 
            }
            template<typename V> auto Range(V min, V max) { return Chain(dsl.Range(min, max)); }

            // Helper to chain with new DSL type
            template<typename NewDSL>
            auto Chain(NewDSL newDSL) {
                moved = true;
                return FieldProxy<NewDSL>(builder, newDSL);
            }

            // Destructor: This is where the magic happens!
            // Only draw if we haven't been moved from
            ~FieldProxy() {
                if (!moved) {
                    DrawField(dsl.desc);
                }
            }
        private:
            // Compile-time dispatch for drawing
            template<size_t N>
            void DrawField(const Shine::Reflection::DSL::FieldDescriptor<N>& desc) {
                using namespace Shine::Reflection;

                // 1. Get Value
                using MemberType = typename DSLType::MemberType;
                // Fix C2327/C2065: FieldProxy is a nested struct but not a member of StaticInspectorBuilder.
                // It holds a reference `builder` to the parent builder.
                // We must access `builder.instance` instead of `instance`.
                auto& value = builder.instance->*DSLType::MemberPtr;

                // 2. Handle Category
                const char* category = nullptr;
                for(const auto& m : desc.metadata) {
                    if (m.key == Hash("Category")) {
                        if (std::holds_alternative<std::string_view>(m.value)) {
                            // Note: std::string_view might point to temporary if not careful, 
                            // but in our DSL it points to string literal from .Meta("Category", "Audio")
                             category = std::get<std::string_view>(m.value).data();
                        }
                    }
                }

                if (category) {
                    bool changed = false;
                    if (builder.currentCategory == nullptr) changed = true;
                    else if (strcmp(builder.currentCategory, category) != 0) changed = true;

                    if (changed) {
                        builder.currentCategory = category;
                        ImGui::Separator();
                        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "[%s]", category);
                    }
                }

                // 3. Draw UI based on Type and Schema
                // Since we know MemberType at compile time, we don't need std::visit or typeId checks!
                
                // Helper to extract Range from metadata
                float min = 0.0f, max = 0.0f;
                for(const auto& m : desc.metadata) {
                    if (m.key == Hash("Min")) {
                        if (std::holds_alternative<float>(m.value)) min = std::get<float>(m.value);
                        else if (std::holds_alternative<int>(m.value)) min = (float)std::get<int>(m.value);
                    }
                    if (m.key == Hash("Max")) {
                        if (std::holds_alternative<float>(m.value)) max = std::get<float>(m.value);
                        else if (std::holds_alternative<int>(m.value)) max = (float)std::get<int>(m.value);
                    }
                }

                // Compile-time if/else
                if constexpr (std::is_same_v<MemberType, float>) {
                    if (min != 0.0f || max != 0.0f) {
                        ImGui::SliderFloat(desc.name.data(), &value, min, max);
                    } else {
                        ImGui::DragFloat(desc.name.data(), &value);
                    }
                }
                else if constexpr (std::is_same_v<MemberType, int>) {
                    if (min != 0.0f || max != 0.0f) {
                        ImGui::SliderInt(desc.name.data(), &value, (int)min, (int)max);
                    } else {
                        ImGui::DragInt(desc.name.data(), &value);
                    }
                }
                else if constexpr (std::is_same_v<MemberType, bool>) {
                    ImGui::Checkbox(desc.name.data(), &value);
                }
                else if constexpr (std::is_same_v<MemberType, std::string>) {
                    // PropertyDrawer::DrawString logic inline
                    char buffer[256];
                    strncpy_s(buffer, sizeof(buffer), value.c_str(), sizeof(buffer) - 1);
                    if (ImGui::InputText(desc.name.data(), buffer, sizeof(buffer))) {
                        value = buffer;
                    }
                }
                else if constexpr (Shine::Reflection::IsVector<MemberType>::value) {
                    // Vector Drawing
                    if (ImGui::TreeNode(desc.name.data())) {
                        using VecType = MemberType;
                        using ElementType = typename VecType::value_type;
                        
                        // Size & Resize UI
                        size_t size = value.size();
                        ImGui::Text("Size: %zu", size);
                        if (ImGui::Button("+")) {
                            value.resize(size + 1);
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("-") && size > 0) {
                            value.resize(size - 1);
                        }

                        // Iterate Elements
                        for (size_t i = 0; i < value.size(); ++i) {
                            ImGui::PushID((int)i);
                            // Recursive Draw for Element
                            // We need a helper function because we can't easily generate DSL for dynamic elements
                            // But we can call StaticInspector::Draw if ElementType is reflectable, 
                            // OR use basic ImGui calls if it's a basic type.
                            
                            // Simple workaround: Use PropertyDrawer's static helpers for basic types,
                            // or recursively call StaticInspector::Draw for structs.
                            
                            if constexpr (std::is_same_v<ElementType, float>) {
                                PropertyDrawer::DrawFloat("##v", value[i]);
                            } else if constexpr (std::is_same_v<ElementType, int>) {
                                PropertyDrawer::DrawInt("##v", value[i]);
                            } else if constexpr (std::is_same_v<ElementType, bool>) {
                                PropertyDrawer::DrawBool("##v", value[i]);
                            } else if constexpr (std::is_same_v<ElementType, std::string>) {
                                PropertyDrawer::DrawString("##v", value[i]);
                            } else {
                                // Recursive struct
                                if (ImGui::TreeNode((void*)(intptr_t)i, "Element %zu", i)) {
                                    StaticInspector::Draw(&value[i]);
                                    ImGui::TreePop();
                                }
                            }
                            
                            ImGui::PopID();
                        }
                        ImGui::TreePop();
                    }
                }
                else {
                    // Fallback or Recursive Struct
                    // Try to detect if MemberType has RegisterReflection
                    // We can use SFINAE or Concept (C++20)
                    // For now, let's just try to call Draw and let the compiler optimize it out if empty?
                    // No, StaticInspector::Draw requires T::RegisterReflection to exist.
                    // We need a concept "IsReflectable".
                    
                    // Simple check: does it have RegisterReflection static method?
                    // We assume yes if it's a struct we want to draw.
                    // But if it's int/float we already handled it.
                    // So here it's likely a struct.
                    
                    if (ImGui::TreeNode(desc.name.data())) {
                         StaticInspector::Draw(&value);
                         ImGui::TreePop();
                    }
                }
            }
        };

        template<typename DSLType>
        FieldProxy<DSLType> RegisterFieldFromDSL(const DSLType& dsl) {
            return FieldProxy<DSLType>{*this, dsl};
        }

        // Methods are ignored in inspector for now
        template<typename DSLType>
        void RegisterMethodFromDSL(const DSLType& dsl) {}
    };

    struct StaticInspector {
        template<typename T>
        static void Draw(T* instance) {
            if (!instance) return;
            ImGui::PushID(instance);
            StaticInspectorBuilder<T> builder(instance);
            T::RegisterReflection(builder);
            ImGui::PopID();
        }
    };
}
