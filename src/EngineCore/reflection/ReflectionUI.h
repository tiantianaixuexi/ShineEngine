#pragma once
#include <variant>

namespace shine::reflection {

    // UI Schema (Constexpr Friendly)
    namespace UI {

        struct None {};

        struct Slider {
            float min  = 0.0f;
            float max  = 0.0f;
            float step = 1.0f;
        };

        struct Color {
            bool hasAlpha = true;
        };

        struct Checkbox {};
        struct InputText {
            size_t maxLength = 256;
        };

        struct FunctionSelector {
            bool onlyScriptCallable = true;
        };

        using Schema = std::variant<None, Slider, Color, Checkbox, InputText, FunctionSelector>;
    }



} // namespace shine::reflection