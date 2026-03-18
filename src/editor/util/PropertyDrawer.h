#pragma once
#include "EngineCore/reflection/Reflection.h"
#include "memory/memory.ixx"
#include "string/shine_string.h"

#include <algorithm>
#include <cstring>

namespace shine::editor::util {

    namespace detail {

        struct ScopedInputTextBuffer {
            static constexpr std::size_t kStackCapacity = 256;

            char stack[kStackCapacity]{};
            char* text = stack;
            std::size_t capacity = kStackCapacity;

            explicit ScopedInputTextBuffer(shine::STextView initialText) {
                const std::size_t requiredCapacity = (std::max)(kStackCapacity, initialText.size() + 1);
                if (requiredCapacity > kStackCapacity) {
                    shine::co::MemoryScope scope(shine::co::MemoryTag::EditorInspectorTemp);
                    text = static_cast<char*>(shine::co::Memory::Alloc(requiredCapacity, alignof(char)));
                    capacity = text ? requiredCapacity : kStackCapacity;
                    if (!text) {
                        text = stack;
                    }
                }

                const std::size_t copyLength = (std::min)(initialText.size(), capacity - 1);
                if (copyLength != 0) {
                    std::memcpy(text, initialText.data(), copyLength);
                }
                text[copyLength] = '\0';
            }

            ~ScopedInputTextBuffer() {
                if (text != stack) {
                    shine::co::Memory::Free(text);
                }
            }

            ScopedInputTextBuffer(const ScopedInputTextBuffer&) = delete;
            ScopedInputTextBuffer& operator=(const ScopedInputTextBuffer&) = delete;

            char* data() noexcept {
                return text;
            }

            std::size_t size() const noexcept {
                return capacity;
            }
        };

    }

    class PropertyDrawer {
    public:
        // Main entry point to draw a field based on its UI schema and metadata
        static void DrawField(void* instance, const reflection::FieldInfo& field, const reflection::TypeInfo* ownerType = nullptr);

        // Immediate mode helpers (Static access without FieldInfo)
        static bool DrawFloat(const char* label, float& value, float min = 0.0f, float max = 0.0f);
        static bool DrawInt(const char* label, int& value, int min = 0, int max = 0);
        static bool DrawBool(const char* label, bool& value);
        static bool DrawString(const char* label, shine::SString& value);
    };

}
