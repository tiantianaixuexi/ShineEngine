#pragma once
#include "ContainerTraits.h"
#include "../Field/FieldInfo.h"
#include "../../../constexpr/constexpr_vector.h"
#include "../../../constexpr/constexpr_map.h"
#include "../CompileTime/ReflectionModernHash.h"

namespace shine::reflection::container {

// 容器反射信息
template<typename Container>
struct ContainerReflectionInfo {
    static constexpr std::string_view name = TypeIdentity<Container>::name;
    static constexpr uint32_t type_id = TypeIdentity<Container>::id;
    
    // 容器元素类型信息
    using element_type = container_element_type_t<Container>;
    static constexpr std::string_view element_name = TypeIdentity<element_type>::name;
    static constexpr uint32_t element_id = TypeIdentity<element_type>::id;
    
    // 容器特质
    using traits = std::conditional_t<
        VectorLike<Container>, VectorTraits<Container>,
        std::conditional_t<
            MapLike<Container>, MapTraits<Container>,
            ContainerInterface<Container>
        >
    >;
    
    // 编译期容器信息
    static constexpr bool is_compile_time_container = is_compile_time_container_v<Container>;
    
    // 容器类型分类
    static constexpr ContainerType container_type = 
        is_compile_time_container ? ContainerType::Array :
        VectorLike<Container> ? ContainerType::Sequence :
        MapLike<Container> ? ContainerType::Associative :
        ContainerType::None;
};

// 容器字段访问器
template<typename Container>
struct ContainerFieldAccessor {
    static void Get(const void* instance, void* out_value, size_t offset, size_t size) {
        const auto* container = static_cast<const Container*>(static_cast<const char*>(instance) + offset);
        *static_cast<Container*>(out_value) = *container;
    }
    
    static void Set(void* instance, const void* in_value, size_t offset, size_t size) {
        auto* container = static_cast<Container*>(static_cast<char*>(instance) + offset);
        *container = *static_cast<const Container*>(in_value);
    }
    
    static bool Equals(const void* a, const void* b, size_t size) {
        const auto* container_a = static_cast<const Container*>(a);
        const auto* container_b = static_cast<const Container*>(b);
        return *container_a == *container_b;
    }
    
    static void Copy(void* dst, const void* src, size_t size) {
        auto* container_dst = static_cast<Container*>(dst);
        const auto* container_src = static_cast<const Container*>(src);
        *container_dst = *container_src;
    }
    
    // 编译期容器专用访问
    template<typename CTContainer = Container>
    requires (ContainerReflectionInfo<CTContainer>::is_compile_time_container)
    static constexpr auto GetCompileTimeView(const CTContainer& container) {
        return container.view(); // 返回编译期视图
    }
    
    // 容器大小获取
    static size_t GetSize(const Container& container) {
        return ContainerReflectionInfo<Container>::traits::size(container);
    }
    
    // 容器是否为空
    static bool IsEmpty(const Container& container) {
        return ContainerReflectionInfo<Container>::traits::empty(container);
    }
};

// 容器元数据构建器
struct ContainerMetadataBuilder {
    template<VectorLike Container>
    static consteval auto BuildVectorMetadata() {
        using VectorInfo = ContainerReflectionInfo<Container>;
        constexpr auto map = shine::constexpr_map<std::string_view, std::string_view, 8>{
            std::pair{"container_type", "vector"},
            std::pair{"element_type", VectorInfo::element_name},
            std::pair{"max_size", "128"}  // VectorTraits中的max_compile_time_size
        };
        return map;
    }
    
    template<MapLike Container>
    static consteval auto BuildMapMetadata() {
        using MapInfo = ContainerReflectionInfo<Container>;
        constexpr auto map = shine::constexpr_map<std::string_view, std::string_view, 8>{
            std::pair{"container_type", "map"},
            std::pair{"key_type", TypeIdentity<typename Container::key_type>::name},
            std::pair{"value_type", TypeIdentity<typename Container::mapped_type>::name},
            std::pair{"max_pairs", "64"}  // MapTraits中的max_compile_time_pairs
        };
        return map;
    }
    
    template<typename Container>
    static consteval auto BuildCompileTimeContainerMetadata() {
        using CTInfo = ContainerReflectionInfo<Container>;
        if constexpr (VectorLike<Container>) {
            constexpr auto map = shine::constexpr_map<std::string_view, std::string_view, 8>{
                std::pair{"container_type", "compile_time_vector"},
                std::pair{"element_type", CTInfo::element_name},
                std::pair{"capacity", std::to_string(Container::capacity)}
            };
            return map;
        } else if constexpr (MapLike<Container>) {
            constexpr auto map = shine::constexpr_map<std::string_view, std::string_view, 8>{
                std::pair{"container_type", "compile_time_map"},
                std::pair{"key_type", TypeIdentity<typename Container::key_type>::name},
                std::pair{"value_type", TypeIdentity<typename Container::mapped_type>::name},
                std::pair{"capacity", std::to_string(Container::capacity)}
            };
            return map;
        } else {
            constexpr auto map = shine::constexpr_map<std::string_view, std::string_view, 4>{
                std::pair{"container_type", "unknown_compile_time"}
            };
            return map;
        }
    }
};

// 容器字段信息构建器
template<typename Container>
struct ContainerFieldInfoBuilder {
    static consteval FieldInfo BuildFieldInfo(
        std::string_view name,
        size_t offset,
        compile_time::PropertyFlags flags = compile_time::PropertyFlags::None) {
        
        FieldInfo field{};
        field.typeId = TypeIdentity<Container>::id;
        field.containerType = ContainerReflectionInfo<Container>::container_type;
        field.offset = offset;
        field.size = sizeof(Container);
        field.alignment = alignof(Container);
        field.isPod = TypeIdentity<Container>::is_pod;
        field.name = name;
        field.flags = static_cast<PropertyFlags>(static_cast<uint32_t>(flags));
        
        // 设置优化的函数指针
        field.getterFn = ContainerFieldAccessor<Container>::Get;
        field.setterFn = ContainerFieldAccessor<Container>::Set;
        field.equalsFn = ContainerFieldAccessor<Container>::Equals;
        field.copyFn = ContainerFieldAccessor<Container>::Copy;
        
        return field;
    }
};

} // namespace shine::reflection::container