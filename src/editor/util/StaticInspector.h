#pragma once
#include "EngineCore/reflection/Reflection.h"
#include "PropertyDrawer.h"
#include "imgui/imgui.h"
#include "string/shine_string.h"
#include <variant>

namespace shine::editor::util {

    // Helper type trait: detect vector-like containers
    template<typename T>
    struct IsVectorHelper : std::false_type {};

    template<typename T, typename A>
    struct IsVectorHelper<std::vector<T, A>> : std::true_type {};

    template<typename T>
    inline constexpr bool IsVector = IsVectorHelper<T>::value;

    // 编译期静态检查器构建器
    // 这个类在编译期被 REFLECT_STRUCT 实例化，直接生成绘制代码
    template<typename T>
    struct StaticInspectorBuilder {


        using ObjectType = T;
        T* instance;
        const char* currentCategory = nullptr; // Track current category

        StaticInspectorBuilder(T* inst) : instance(inst) {}

        template<typename U>
        static void Draw(U* instance) {
            if (!instance) return;
            
            if (ImGui::BeginTable("Inspector", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 150.0f);
                ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
                
                ImGui::PushID(instance);
                static StaticInspectorBuilder<T> builder(instance);
                _ReflectStaticBuild(builder, static_cast<T*>(nullptr));
                ImGui::PopID();
                
                ImGui::EndTable();
            }
        }

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
            template<typename V> auto Meta(shine::STextView key, V&& val) { return Chain(dsl.Meta(key, std::forward<V>(val))); }
            // Fix: Add constexpr Meta with string literal capture
            template<size_t N, typename V> 
            constexpr auto Meta(const char (&key)[N], V&& val) { 
                return Chain(dsl.Meta(key, std::forward<V>(val))); 
            }
            template<typename V> auto Range(V min, V max) { return Chain(dsl.Range(min, max)); }
            
            // Add FunctionSelect support
            auto FunctionSelect(bool onlyScriptCallable = true) { 
                return Chain(dsl.FunctionSelect(onlyScriptCallable)); 
            }

            template<size_t N> auto DisplayName(const char (&name)[N]) { return Meta("DisplayName", name); }
            auto DisplayName(shine::STextView name) { return Meta("DisplayName", name); }

            // Helper to chain with new DSL type
            template<typename NewDSL>
            auto Chain(NewDSL newDSL) {
                moved = true;
                return FieldProxy<NewDSL>(builder, newDSL);
            }
            
            // Add OnChange support (template version for compile-time capture)
            template<auto CallbackPtr>
            auto OnChange() {
                return Chain(dsl.template OnChange<CallbackPtr>());
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
            void DrawField(const shine::reflection::DSL::FieldDescriptorBase& desc) {
                using namespace shine::reflection;

                // 1. Get Value
                using MemberType = typename DSLType::MemberType;
                // Fix C2327/C2065: FieldProxy is a nested struct but not a member of StaticInspectorBuilder.
                // It holds a reference `builder` to the parent builder.
                // We must access `builder.instance` instead of `instance`.
                auto& value = builder.instance->*DSLType::MemberPtrValue;

                // 2. Handle Category
                const char* category = nullptr;
                for(const auto& m : desc.metadata) {
                    if (m.first == MetaKeys::Category) {
                        if (std::holds_alternative<shine::STextView>(m.second)) {
                             category = std::get<shine::STextView>(m.second).data();
                        }
                    }
                }

                if (category) {
                    bool changed = false;
                    if (builder.currentCategory == nullptr) changed = true;
                    else if (strcmp(builder.currentCategory, category) != 0) changed = true;

                    if (changed) {
                        builder.currentCategory = category;
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "[%s]", category);
                    }
                }

                // 3. Draw UI based on Type and Schema
                // Since we know MemberType at compile time, we don't need std::visit or typeId checks!
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();

                // Special case for Vectors and Structs (TreeNodes)
                if constexpr (IsVector<MemberType>) {
                     ImGui::TableNextRow();
                     ImGui::TableSetColumnIndex(0);
                     bool open = ImGui::TreeNodeEx(desc.name.data(), ImGuiTreeNodeFlags_SpanFullWidth);
                     ImGui::TableSetColumnIndex(1);
                     ImGui::Text("Size: %zu", value.size());
                     
                     if (open) {
                         // Vector logic
                         ImGui::SameLine();
                         if (ImGui::Button("+")) value.resize(value.size() + 1);
                         ImGui::SameLine();
                         if (ImGui::Button("-") && value.size() > 0) value.resize(value.size() - 1);
                         
                         // Elements
                         for (size_t i = 0; i < value.size(); ++i) {
                             ImGui::PushID(static_cast<int>(i));
                             
                             ImGui::TableNextRow();
                             ImGui::TableSetColumnIndex(0);
                             ImGui::TreeNodeEx((void*)(intptr_t)i, ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen, "Element %zu", i);
                             
                             ImGui::TableSetColumnIndex(1);
                             ImGui::PushItemWidth(-1);
                             
                             using ElementType = typename MemberType::value_type;
                             if constexpr (std::is_same_v<ElementType, float>) PropertyDrawer::DrawFloat("##v", value[i]);
                             else if constexpr (std::is_same_v<ElementType, int>) PropertyDrawer::DrawInt("##v", value[i]);
                             else if constexpr (std::is_same_v<ElementType, bool>) PropertyDrawer::DrawBool("##v", value[i]);
                                else if constexpr (std::is_same_v<ElementType, shine::SString>) PropertyDrawer::DrawString("##v", value[i]);
                             else {
                                  // Recursive struct in vector
                                  StaticInspectorBuilder::Draw(&value[i]);
                             }
                             
                             ImGui::PopItemWidth();
                             ImGui::PopID();
                         }
                         ImGui::TreePop();
                     }
                     return;
                }
                else if constexpr (!std::is_same_v<MemberType, float> && !std::is_same_v<MemberType, int> && !std::is_same_v<MemberType, bool> && !std::is_same_v<MemberType, shine::SString>) {
                     if constexpr (std::is_enum_v<MemberType>) {
                         ImGui::Text("%s", desc.name.data());
                         ImGui::TableSetColumnIndex(1);
                         ImGui::PushItemWidth(-1);

                         using namespace shine::reflection;
                         // We need to look up TypeInfo via TypeId, not compile-time template if possible
                         // But StaticInspectorBuilder knows MemberType at compile time.
                         const TypeInfo* typeInfo = TypeRegistry::Get().FindFast(GetTypeId<MemberType>());
                         
                         // Fallback: If not found, try to register it on the fly?
                         // We can force a temporary TypeBuilder if we know the type name, but we only have TypeId.
                         // HOWEVER, if the type has a static Refection function, we can try to call it? No, we don't have that interface.
                         
                         // Hack: For GameDifficulty specifically (or any enum), ensure it's registered.
                         // But we are in a template.
                         
                         if (typeInfo && typeInfo->isEnum) {
                             int64_t currentVal = (int64_t)value;
                             const char* currentName = "Unknown";
                             for(const auto& e : typeInfo->enumEntries) {
                                 if (e.value == currentVal) { currentName = e.name.data(); break; }
                             }

                             shine::SString label = "##";
                             label += desc.name;
                             if (ImGui::BeginCombo(label.c_str(), currentName)) {
                                 for(const auto& e : typeInfo->enumEntries) {
                                     bool isSelected = (currentVal == e.value);
                                     if (ImGui::Selectable(e.name.data(), isSelected)) {
                                         value = (MemberType)e.value;
                                         if (desc.onChange) desc.onChange(builder.instance, &currentVal);
                                     }
                                     if (isSelected) ImGui::SetItemDefaultFocus();
                                 }
                                 ImGui::EndCombo();
                             }
                         } else {

                             int         val    = static_cast<int>(value);
                             int         oldVal = val;
                             shine::SString label  = "##";
                             label += desc.name;
                             if (ImGui::InputInt(label.c_str(), &val)) {
                                 value = static_cast<MemberType>(val);
                                 if (desc.onChange) desc.onChange(builder.instance, &oldVal);
                             }
                             ImGui::SameLine();
                             ImGui::TextDisabled("(Enum Info Missing)");
                         }
                         ImGui::PopItemWidth();
                     }
                     else {
                         // Struct (Recursive)
                         ImGui::TableNextRow();
                         ImGui::TableSetColumnIndex(0);
                         bool open = ImGui::TreeNodeEx(desc.name.data(), ImGuiTreeNodeFlags_SpanFullWidth);
                         ImGui::TableSetColumnIndex(1);
                         
                         if (open) {
                             StaticInspectorBuilder::Draw(&value);
                             ImGui::TreePop();
                         }
                     }
                     return;
                }

                // Normal Field
                const char* displayName = desc.name.data();
                for(const auto& m : desc.metadata) {
                    if (m.first == MetaKeys::DisplayName) {
                        if (std::holds_alternative<shine::STextView>(m.second)) {
                            displayName = std::get<shine::STextView>(m.second).data();
                        }
                    }
                }
                ImGui::Text("%s", displayName);
                
                ImGui::TableSetColumnIndex(1);
                ImGui::PushItemWidth(-1);

                shine::SString label = "##";
                label += desc.name;
                const char* labelC = label.c_str();

                // Helper to extract Range from metadata
                float min = 0.0f, max = 0.0f;
                for(const auto& m : desc.metadata) {
                    if (m.first == MetaKeys::Min) {
                        if (std::holds_alternative<float>(m.second)) min = std::get<float>(m.second);
                        else if (std::holds_alternative<int>(m.second)) min = static_cast<float>(std::get<int>(m.second));
                    }
                    if (m.first == MetaKeys::Max) {
                        if (std::holds_alternative<float>(m.second)) max = std::get<float>(m.second);
                        else if (std::holds_alternative<int>(m.second)) max = static_cast<float>(std::get<int>(m.second));
                    }
                }

                // Compile-time if/else
                bool changed = false;
                MemberType oldValue = value;

                if constexpr (std::is_same_v<MemberType, float>) {
                    if (min != 0.0f || max != 0.0f) {
                        if (ImGui::SliderFloat(labelC, &value, min, max)) changed = true;
                    } else {
                        if (ImGui::DragFloat(labelC, &value)) changed = true;
                    }
                }
                else if constexpr (std::is_same_v<MemberType, int>) {
                    if (min != 0.0f || max != 0.0f) {
                        if (ImGui::SliderInt(labelC, &value, static_cast<int>(min), static_cast<int>(max))) changed = true;
                    } else {
                        if (ImGui::DragInt(labelC, &value)) changed = true;
                    }
                }
                else if constexpr (std::is_same_v<MemberType, bool>) {
                    if (ImGui::Checkbox(labelC, &value)) changed = true;
                }
                else if constexpr (std::is_same_v<MemberType, shine::SString>) {
                    // Check for FunctionSelector schema
                    bool isFunctionSelector = false;
                    bool onlyScriptCallable = true;
                    if (std::holds_alternative<shine::reflection::UI::FunctionSelector>(desc.uiSchema)) {
                        isFunctionSelector = true;
                        onlyScriptCallable = std::get<shine::reflection::UI::FunctionSelector>(desc.uiSchema).onlyScriptCallable;
                    }

                    if (isFunctionSelector) {
                        using namespace shine::reflection;
                        const TypeInfo* typeInfo = TypeRegistry::Get().FindFast(GetTypeId<ObjectType>());
                        
                        if (typeInfo) {
                            if (ImGui::BeginCombo(labelC, value.c_str())) {
                                for (const auto& method : typeInfo->methods) {
                                    bool show = !onlyScriptCallable;
                                    if (onlyScriptCallable) {
                                        if (reflection::HasFlag(method.flags, FunctionFlags::ScriptCallable)) show = true;
                                        if (method.GetMeta(MetaKeys::BlueprintFunction)) show = true;
                                    }

                                    if (show) {
                                        const auto methodName = method.GetNameView();
                                        bool isSelected = (value == methodName);
                                        if (ImGui::Selectable(methodName.data(), isSelected)) {
                                            value = shine::SString(methodName);
                                            changed = true;
                                        }
                                        if (isSelected) ImGui::SetItemDefaultFocus();
                                    }
                                }
                                ImGui::EndCombo();
                            }
                        } else {
                            ImGui::TextDisabled("%s (TypeInfo Not Found)", desc.name.data());
                        }
                    } else {
                        // Standard InputText
                        char buffer[256];
                        strncpy_s(buffer, sizeof(buffer), value.c_str(), sizeof(buffer) - 1);
                        if (ImGui::InputText(labelC, buffer, sizeof(buffer))) {
                            value = shine::SString(buffer);
                            changed = true;
                        }
                    }
                }
                
                ImGui::PopItemWidth();
                
                if (changed && desc.onChange) {
                    // Pass oldValue to callback
                    desc.onChange(builder.instance, &oldValue);
                }
            }
        };

        template<typename DSLType>
        FieldProxy<DSLType> RegisterFieldFromDSL(const DSLType& dsl) {
            return FieldProxy<DSLType>{*this, dsl};
        }

        // Methods are ignored in inspector for now, but we must support chaining to avoid compilation errors
        template<typename DSLType>
        struct MethodProxy {
            StaticInspectorBuilder& builder;
            DSLType dsl; // Store DSL to access name/metadata
            bool moved = false;
            
            MethodProxy(StaticInspectorBuilder& b, DSLType d) : builder(b), dsl(d) {}
            MethodProxy(MethodProxy&& other) noexcept : builder(other.builder), dsl(other.dsl) { other.moved = true; }

            // Chaining methods - Must move *this to avoid copy (which is deleted)
            // And to ensure "moved" flag propagates if we were creating new objects
            // But here we return the same object (moved)
            auto ScriptCallable() { return std::move(*this); }
            auto EditorCallable() { return std::move(*this); }
            template<typename V> auto Meta(shine::STextView key, V&& val) { return std::move(*this); }
            template<size_t N, typename V> constexpr auto Meta(const char (&key)[N], V&& val) { return std::move(*this); }

            ~MethodProxy() {
                if (!moved) {
                    DrawMethod();
                }
            }

        private:
            void DrawMethod() {
                // Draw a button for the method
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%s", dsl.desc.name.data());
                
                ImGui::TableSetColumnIndex(1);
                if (ImGui::Button(dsl.desc.name.data())) {
                    (builder.instance->*DSLType::MethodPtrValue)();
                }
            }
        };

        template<typename DSLType>
        MethodProxy<DSLType> RegisterMethodFromDSL(const DSLType& dsl) {
            return MethodProxy<DSLType>{*this, dsl};
        }
    };


}
