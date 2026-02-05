#pragma once

namespace shine::reflection {

// 简化的数组特质
template<typename T>
struct ArrayTrait {
    using ElementType = typename T::value_type;
    
    static constexpr size_t GetSize(const T* container) {
        return container ? container->size() : 0;
    }
    
    static constexpr ElementType* GetElement(T* container, size_t index) {
        return container && index < container->size() ? &(*container)[index] : nullptr;
    }
    
    static constexpr const ElementType* GetElementConst(const T* container, size_t index) {
        return container && index < container->size() ? &(*container)[index] : nullptr;
    }
    
    static constexpr void Resize(T* container, size_t newSize) {
        if (container) container->resize(newSize);
    }
};

// 简化的映射特质
template<typename T>
struct MapTrait {
    using KeyType = typename T::key_type;
    using ValueType = typename T::mapped_type;
    
    static constexpr size_t GetSize(const T* container) {
        return container ? container->size() : 0;
    }
    
    static constexpr void Clear(T* container) {
        if (container) container->clear();
    }
    
    static constexpr void Insert(T* container, const KeyType& key, const ValueType& value) {
        if (container) container->insert_or_assign(key, value);
    }
    
    // 迭代器相关操作
    struct IteratorWrapper {
        typename T::const_iterator iter;
        const T* container;
        
        constexpr bool IsValid() const {
            return container && iter != container->end();
        }
        
        constexpr const KeyType& Key() const {
            return iter->first;
        }
        
        constexpr ValueType& Value() const {
            return iter->second;
        }
        
        constexpr void Next() {
            if (IsValid()) ++iter;
        }
    };
    
    static constexpr IteratorWrapper Begin(const T* container) {
        return container ? IteratorWrapper{container->begin(), container} : IteratorWrapper{typename T::const_iterator{}, nullptr};
    }
};

// 概念约束
template<typename T>
concept HasArrayInterface = requires(T t, size_t i) {
    t.size();
    t[i];
    t.resize(size_t{});
};

template<typename T>
concept HasMapInterface = requires(T t, typename T::key_type k, typename T::mapped_type v) {
    t.size();
    t.clear();
    t.insert_or_assign(k, v);
    typename T::const_iterator;
};

// 特质工厂
template<typename T>
struct TraitFactory {
    using Array = ArrayTrait<T>;
    using Map = MapTrait<T>;
};

} // namespace shine::reflection