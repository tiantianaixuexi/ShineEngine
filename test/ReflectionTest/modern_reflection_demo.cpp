// 现代C++反射系统 - 最小演示版本
// 展示核心概念和设计理念

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <type_traits>
#include <concepts>

namespace shine::reflection::demo {

// 1. 编译期哈希系统
consteval uint32_t HashString(std::string_view str) {
    uint32_t hash = 2166136261u;
    for (char c : str) {
        hash ^= static_cast<uint8_t>(c);
        hash *= 16777619u;
    }
    return hash;
}

// 2. 现代化类型系统
template<typename T>
struct TypeIdentity {
    static constexpr std::string_view name = "unknown";
    static constexpr uint32_t id = HashString("unknown");
    static constexpr bool is_pod = std::is_trivially_copyable_v<T>;
    static constexpr size_t size = sizeof(T);
    static constexpr size_t alignment = alignof(T);
};

// 3. 简化的字段信息
template<typename Class, typename MemberType>
struct FieldInfo {
    std::string_view name;
    MemberType Class::* ptr;
    size_t offset;
    uint32_t type_id;
    
    constexpr FieldInfo(std::string_view n, MemberType Class::* p, size_t off)
        : name(n), ptr(p), offset(off), type_id(TypeIdentity<MemberType>::id) {}
    
    // 字段访问方法
    constexpr MemberType Get(const Class& instance) const {
        return instance.*ptr;
    }
    
    constexpr void Set(Class& instance, const MemberType& value) const {
        instance.*ptr = value;
    }
};

// 4. 容器特质系统
template<typename T>
concept Container = requires(T t) {
    t.size();
    t.begin();
    t.end();
};

template<typename T>
concept AssociativeContainer = Container<T> && requires(T t, typename T::key_type k) {
    t.find(k);
    typename T::mapped_type{};
};

// 5. 注册宏系统
#define REFLECT_FIELD(Type, Field) \
    ::shine::reflection::demo::FieldInfo<Type, decltype(Type::Field)>{#Field, &Type::Field, offsetof(Type, Field)}

#define BEGIN_REFLECTION(Type) \
    struct Type##Reflection { \
        static constexpr auto GetFields() { \
            return std::array{

#define FIELD(Field) \
                REFLECT_FIELD(Type, Field),

#define END_REFLECTION() \
            }; \
        } \
    };

// 6. 实际使用示例
struct Person {
    std::string name;
    int age;
    std::vector<std::string> hobbies;
    std::map<std::string, int> scores;
};

BEGIN_REFLECTION(Person)
    FIELD(name)
    FIELD(age)
    FIELD(hobbies)
    FIELD(scores)
END_REFLECTION()

// 7. 编译期验证和使用
template<typename T>
consteval void ValidateReflection() {
    constexpr auto fields = T##Reflection::GetFields();
    std::cout << "Type " << #T << " has " << fields.size() << " fields:\n";
    for (const auto& field : fields) {
        std::cout << "  - " << field.name << " (offset: " << field.offset 
                  << ", type_id: " << field.type_id << ")\n";
    }
}

} // namespace shine::reflection::demo

// 测试函数
int main() {
    using namespace shine::reflection::demo;
    
    std::cout << "=== 现代C++反射系统演示 ===\n\n";
    
    // 验证Person类型的反射信息
    std::cout << "编译期反射信息:\n";
    // ValidateReflection<Person>(); // 编译期验证
    
    // 运行时使用示例
    Person person{"Alice", 25, {"reading", "coding"}, {{"math", 95}, {"english", 88}}};
    
    constexpr auto fields = PersonReflection::GetFields();
    std::cout << "Person实例字段值:\n";
    for (const auto& field : fields) {
        if (field.name == "name") {
            std::cout << "  " << field.name << ": " << field.Get(person) << "\n";
        } else if (field.name == "age") {
            std::cout << "  " << field.name << ": " << field.Get(person) << "\n";
        }
    }
    
    // 修改字段值
    std::cout << "\n修改字段值后:\n";
    for (auto& field : fields) {
        if (field.name == "age") {
            field.Set(person, 26);
            std::cout << "  " << field.name << ": " << field.Get(person) << "\n";
        }
    }
    
    std::cout << "\n=== 演示完成 ===\n";
    return 0;
}