#pragma once
#include <string_view>
#include <variant>

namespace shine::reflection {
namespace UI {

// UI模式基类
struct Schema {};

// 无UI显示
struct None : Schema {};

// 文本输入框
struct TextInput : Schema {
    size_t max_length = 0;
    bool multiline = false;
};

// 数字输入框
struct NumberInput : Schema {
    double min_value = 0.0;
    double max_value = 100.0;
    double step = 1.0;
};

// 滑动条
struct Slider : Schema {
    double min_value = 0.0;
    double max_value = 100.0;
    double step = 1.0;
};

// 下拉选择框
struct Dropdown : Schema {
    // 选项将在运行时提供
};

// 复选框
struct Checkbox : Schema {};

// 颜色选择器
struct ColorPicker : Schema {};

// 文件选择器
struct FilePicker : Schema {
    std::string_view filter;
    bool allow_multiple = false;
};

// 函数选择器
struct FunctionSelector : Schema {
    bool only_script_callable = true;
};

// 向量编辑器
struct VectorEditor : Schema {
    size_t dimensions = 3;
    double min_value = -1000.0;
    double max_value = 1000.0;
};

// 矩阵编辑器
struct MatrixEditor : Schema {
    size_t rows = 4;
    size_t cols = 4;
};

} // namespace UI
} // namespace shine::reflection