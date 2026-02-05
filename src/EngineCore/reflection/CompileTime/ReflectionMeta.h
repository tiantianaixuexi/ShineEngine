#pragma once

#include <array>
#include <variant>
#include <string_view>

#include "../Core/ReflectionCore.h"
#include "ReflectionHash.h"
#include "EngineCore/reflection/ReflectionHash.h"

namespace shine::reflection {

    // =============================================================================
    // 编译期元数据系统
    // =============================================================================

    namespace meta {

        // =============================================================================
        // 元数据值类型
        // =============================================================================

        // 支持的元数据值类型
        using MetaValue = std::variant<
            bool,
            int8_t, int16_t, int32_t, int64_t,
            uint8_t, uint16_t, uint32_t, uint64_t,
            float, double,
            std::string_view,
            TypeId,
            HashValue
        >;

        // 元数据条目
        struct MetaEntry {
            MetadataKey key;
            MetaValue value;
            
            constexpr MetaEntry(MetadataKey k, MetaValue v) : key(k), value(v) {}
        };

        // =============================================================================
        // 编译期元数据存储
        // =============================================================================

        template<size_t MaxEntries>
        class CompileTimeMetaData {
        private:
            constexpr_::constexpr_vector<MetaEntry, MaxEntries> entries_;

        public:
            constexpr CompileTimeMetaData() = default;

            // 添加元数据
            template<typename T>
            consteval bool Add(MetadataKey key, T&& value) {
                if (entries_.full()) {
                    return false;
                }
                
                MetaValue meta_value{std::forward<T>(value)};
                entries_.push_back(MetaEntry{key, meta_value});
                return true;
            }

            // 获取元数据
            consteval const MetaValue* Get(MetadataKey key) const {
                for (const auto& entry : entries_) {
                    if (entry.key == key) {
                        return &entry.value;
                    }
                }
                return nullptr;
            }

            // 检查是否存在元数据
            consteval bool Contains(MetadataKey key) const {
                return Get(key) != nullptr;
            }

            // 获取特定类型的元数据
            template<typename T>
            consteval const T* GetAs(MetadataKey key) const {
                const MetaValue* value = Get(key);
                if (value && std::holds_alternative<T>(*value)) {
                    return &std::get<T>(*value);
                }
                return nullptr;
            }

            consteval size_t Size() const { return entries_.size(); }
            consteval bool Empty() const { return entries_.empty(); }
            consteval bool Full() const { return entries_.full(); }
        };

        // =============================================================================
        // 预定义的元数据键
        // =============================================================================

        namespace keys {
            
            // 显示相关
            inline constexpr MetadataKey DisplayName = h::precomputed::StringHash("DisplayName");
            inline constexpr MetadataKey Category = h::precomputed::StringHash("Category");
            inline constexpr MetadataKey Tooltip = h::precomputed::StringHash("Tooltip");
            inline constexpr MetadataKey Icon = h::precomputed::StringHash("Icon");
            
            // 编辑相关
            inline constexpr MetadataKey Editable = h::precomputed::StringHash("Editable");
            inline constexpr MetadataKey ReadOnly = h::precomputed::StringHash("ReadOnly");
            inline constexpr MetadataKey Visible = h::precomputed::StringHash("Visible");
            inline constexpr MetadataKey Hidden = h::precomputed::StringHash("Hidden");
            inline constexpr MetadataKey EditCondition = h::precomputed::StringHash("EditCondition");
            
            // 脚本相关
            inline constexpr MetadataKey ScriptReadable = h::precomputed::StringHash("ScriptReadable");
            inline constexpr MetadataKey ScriptWritable = h::precomputed::StringHash("ScriptWritable");
            inline constexpr MetadataKey ScriptName = h::precomputed::StringHash("ScriptName");
            
            // 序列化相关
            inline constexpr MetadataKey Serializable = h::precomputed::StringHash("Serializable");
            inline constexpr MetadataKey Transient = h::precomputed::StringHash("Transient");
            inline constexpr MetadataKey DefaultValue = h::precomputed::StringHash("DefaultValue");
            
            // 验证相关
            inline constexpr MetadataKey RangeMin = h::precomputed::StringHash("RangeMin");
            inline constexpr MetadataKey RangeMax = h::precomputed::StringHash("RangeMax");
            inline constexpr MetadataKey Required = h::precomputed::StringHash("Required");
            inline constexpr MetadataKey RegexPattern = h::precomputed::StringHash("RegexPattern");
            
            // 性能相关
            inline constexpr MetadataKey Inline = h::precomputed::StringHash("Inline");
            inline constexpr MetadataKey Cacheable = h::precomputed::StringHash("Cacheable");
            inline constexpr MetadataKey ThreadSafe = h::precomputed::StringHash("ThreadSafe");
            
            // 版本相关
            inline constexpr MetadataKey SinceVersion = h::precomputed::StringHash("SinceVersion");
            inline constexpr MetadataKey Deprecated = h::precomputed::StringHash("Deprecated");
            inline constexpr MetadataKey Obsolete = h::precomputed::StringHash("Obsolete");

        } // namespace keys

        // =============================================================================
        // 元数据构建器
        // =============================================================================

        template<size_t MaxEntries = REFLECTION_MAX_METADATA_ENTRIES>
        class MetaDataBuilder {
        private:
            CompileTimeMetaData<MaxEntries> metadata_;

        public:
            constexpr MetaDataBuilder() = default;

            // 链式设置方法
            template<typename T>
            consteval MetaDataBuilder& Set(MetadataKey key, T&& value) {
                metadata_.Add(key, std::forward<T>(value));
                return *this;
            }

            // 便捷方法
            consteval MetaDataBuilder& DisplayName(std::string_view name) {
                return Set(keys::DisplayName, name);
            }

            consteval MetaDataBuilder& Category(std::string_view category) {
                return Set(keys::Category, category);
            }

            consteval MetaDataBuilder& Tooltip(std::string_view tooltip) {
                return Set(keys::Tooltip, tooltip);
            }

            consteval MetaDataBuilder& Editable(bool editable = true) {
                return Set(keys::Editable, editable);
            }

            consteval MetaDataBuilder& ReadOnly(bool readonly = true) {
                return Set(keys::ReadOnly, readonly);
            }

            consteval MetaDataBuilder& Visible(bool visible = true) {
                return Set(keys::Visible, visible);
            }

            consteval MetaDataBuilder& Hidden(bool hidden = true) {
                return Set(keys::Hidden, hidden);
            }

            consteval MetaDataBuilder& ScriptReadable(bool readable = true) {
                return Set(keys::ScriptReadable, readable);
            }

            consteval MetaDataBuilder& ScriptWritable(bool writable = true) {
                return Set(keys::ScriptWritable, writable);
            }

            consteval MetaDataBuilder& Range(double min, double max) {
                return Set(keys::RangeMin, min).Set(keys::RangeMax, max);
            }

            consteval MetaDataBuilder& Required(bool required = true) {
                return Set(keys::Required, required);
            }

            consteval MetaDataBuilder& Transient(bool transient = true) {
                return Set(keys::Transient, transient);
            }

            consteval MetaDataBuilder& Deprecated(std::string_view message = "") {
                return Set(keys::Deprecated, true).Set(keys::Tooltip, message);
            }

            // 构建最终的元数据对象
            consteval auto Build() const {
                return metadata_;
            }

            // 获取构建过程中的元数据（用于中间检查）
            consteval const CompileTimeMetaData<MaxEntries>& GetMetaData() const {
                return metadata_;
            }
        };

        // =============================================================================
        // 类型特定的元数据
        // =============================================================================

        // 数值类型元数据
        template<NumericType T>
        class NumericMetaDataBuilder : public MetaDataBuilder<> {
        public:
            consteval NumericMetaDataBuilder& Range(T min, T max) {
                return static_cast<NumericMetaDataBuilder&>(
                    MetaDataBuilder<>::Set(keys::RangeMin, static_cast<double>(min))
                                   .Set(keys::RangeMax, static_cast<double>(max)));
            }

            consteval NumericMetaDataBuilder& Slider(T min, T max) {
                return Range(min, max).Set(h::precomputed::StringHash("Slider"), true);
            }

            consteval NumericMetaDataBuilder& Spinner(T step = T{1}) {
                return Set(h::precomputed::StringHash("Spinner"), true)
                          .Set(h::precomputed::StringHash("Step"), static_cast<double>(step));
            }
        };

        // 字符串类型元数据
        template<StringType T>
        class StringMetaDataBuilder : public MetaDataBuilder<> {
        public:
            consteval StringMetaDataBuilder& Multiline(size_t lines = 3) {
                return Set(h::precomputed::StringHash("Multiline"), true)
                          .Set(h::precomputed::StringHash("LineCount"), static_cast<uint32_t>(lines));
            }

            consteval StringMetaDataBuilder& Password(bool password = true) {
                return Set(h::precomputed::StringHash("Password"), password);
            }

            consteval StringMetaDataBuilder& FilePath(bool file_path = true) {
                return Set(h::precomputed::StringHash("FilePath"), file_path);
            }

            consteval StringMetaDataBuilder& DirectoryPath(bool dir_path = true) {
                return Set(h::precomputed::StringHash("DirectoryPath"), dir_path);
            }

            consteval StringMetaDataBuilder& Regex(std::string_view pattern) {
                return Set(keys::RegexPattern, pattern);
            }
        };

        // 容器类型元数据
        template<ContainerReflectable T>
        class ContainerMetaDataBuilder : public MetaDataBuilder<> {
        public:
            consteval ContainerMetaDataBuilder& FixedSize(size_t size) {
                return Set(h::precomputed::StringHash("FixedSize"), static_cast<uint32_t>(size));
            }

            consteval ContainerMetaDataBuilder& Resizable(bool resizable = true) {
                return Set(h::precomputed::StringHash("Resizable"), resizable);
            }

            consteval ContainerMetaDataBuilder& Sorted(bool sorted = true) {
                return Set(h::precomputed::StringHash("Sorted"), sorted);
            }
        };

        // =============================================================================
        // 元数据查询和验证
        // =============================================================================

        class MetaDataValidator {
        public:
            // 验证元数据的一致性
            template<size_t N>
            consteval static bool Validate(const CompileTimeMetaData<N>& metadata) {
                // 检查冲突的标志组合
                bool editable = metadata.Contains(keys::Editable) && 
                               *metadata.GetAs<bool>(keys::Editable);
                bool readonly = metadata.Contains(keys::ReadOnly) && 
                               *metadata.GetAs<bool>(keys::ReadOnly);
                bool visible = metadata.Contains(keys::Visible) && 
                              *metadata.GetAs<bool>(keys::Visible);
                bool hidden = metadata.Contains(keys::Hidden) && 
                             *metadata.GetAs<bool>(keys::Hidden);

                // 编辑性冲突检查
                if (editable && readonly) return false;
                
                // 可见性冲突检查
                if (visible && hidden) return false;

                // 范围有效性检查
                const auto* min_val = metadata.GetAs<double>(keys::RangeMin);
                const auto* max_val = metadata.GetAs<double>(keys::RangeMax);
                if (min_val && max_val && *min_val > *max_val) return false;

                return true;
            }

            // 获取元数据摘要
            template<size_t N>
            consteval static auto GetSummary(const CompileTimeMetaData<N>& metadata) {
                struct Summary {
                    size_t entry_count = 0;
                    bool has_display_info = false;
                    bool has_edit_info = false;
                    bool has_validation = false;
                    bool is_valid = false;
                };

                Summary summary;
                summary.entry_count = metadata.Size();
                summary.has_display_info = metadata.Contains(keys::DisplayName) || 
                                          metadata.Contains(keys::Category);
                summary.has_edit_info = metadata.Contains(keys::Editable) || 
                                       metadata.Contains(keys::ReadOnly);
                summary.has_validation = metadata.Contains(keys::RangeMin) || 
                                        metadata.Contains(keys::Required) ||
                                        metadata.Contains(keys::RegexPattern);
                summary.is_valid = Validate(metadata);

                return summary;
            }
        };

    } // namespace meta

    // 便利的命名空间别名
    namespace m = meta;

} // namespace shine::reflection