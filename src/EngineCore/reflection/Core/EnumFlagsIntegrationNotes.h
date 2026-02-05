#pragma once

// =============================================================================
// EnumFlags 集成改进说明
// =============================================================================

/*
为什么要集成 EnumFlags.h？

1. 代码复用性
   - 避免重复实现枚举位运算操作符
   - 统一整个项目的枚举标志处理方式
   - 减少维护成本

2. 功能增强
   - 获得成熟的 EnumFlags 框架支持
   - 编译期安全检查
   - 更丰富的标志操作函数

3. 一致性提升
   - 与项目其他部分使用相同的枚举处理方式
   - 统一的编码风格和接口
   - 降低学习成本

4. 性能优化
   - 利用经过优化的位运算实现
   - 编译期计算优势
   - 更好的类型安全

使用方式对比：

// 原始方式（手动实现）
enum class PropertyFlags {
    None = 0,
    Editable = 1 << 0,
    ReadOnly = 1 << 1
};

// 手动实现所有操作符...

// 改进方式（使用 EnumFlags）
enum class PropertyFlags {
    None = 0,
    Editable = 1 << 0,
    ReadOnly = 1 << 1
};
ENABLE_ENUM_FLAGS(PropertyFlags)  // 一行启用所有功能

// 直接获得：
// - operator|, operator&, operator^, operator~
// - operator|=, operator&=, operator^=
// - HasFlag, HasAnyFlag 等实用函数

这样做的好处：
1. 代码量减少 80% 以上
2. 功能更强大且安全
3. 与项目其他部分保持一致
4. 易于维护和扩展
*/

namespace shine::reflection::enum_flags_integration {

    // 这个命名空间展示了集成的最佳实践
    
    // 1. 在枚举定义后立即启用 EnumFlags 支持
    // 2. 使用统一的命名约定
    // 3. 提供便捷的类型别名
    
    // 示例用法：
    /*
    using namespace shine::reflection;
    
    // 定义属性标志
    PropertyFlags field_flags = PropertyFlags::Editable | PropertyFlags::ScriptReadable;
    
    // 检查标志
    if (HasFlag(field_flags, PropertyFlags::Editable)) {
        // 处理可编辑字段
    }
    
    // 组合标志
    MethodFlags method_flags = MethodFlags::Const | MethodFlags::ScriptCallable;
    
    // 编译期验证也完全支持
    static_assert(HasFlag(PropertyFlags::Editable | PropertyFlags::ReadOnly, 
                         PropertyFlags::Editable));
    */

} // namespace shine::reflection::enum_flags_integration