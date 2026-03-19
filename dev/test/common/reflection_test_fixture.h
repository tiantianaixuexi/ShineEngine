#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

#include "EngineCore/reflection/Reflection.h"

struct Vec3 {
    float x, y, z;

    Vec3() : x(0), y(0), z(0) {}
    Vec3(float inX, float inY, float inZ) : x(inX), y(inY), z(inZ) {}

    Vec3 operator+(const Vec3& other) const {
        return Vec3(x + other.x, y + other.y, z + other.z);
    }

    bool operator==(const Vec3& other) const {
        return x == other.x && y == other.y && z == other.z;
    }

    float LengthSquared() const {
        return x * x + y * y + z * z;
    }

    void SetX(float value) {
        x = value;
    }
};

struct Transform {
    Vec3 position;
    Vec3 rotation;
    Vec3 scale;
    std::string name;
    int id;
    bool enabled;
    std::vector<int> tags;
    std::map<std::string, int> properties;
    std::set<int> flags;

    Transform() : position(), rotation(), scale(1, 1, 1), name(""), id(0), enabled(true) {}

    void SetPosition(float x, float y, float z) {
        position.x = x;
        position.y = y;
        position.z = z;
    }

    Vec3 GetPosition() const {
        return position;
    }

    int AddTag(int tag) {
        tags.push_back(tag);
        return static_cast<int>(tags.size());
    }

    void SetProperty(std::string* key, int* value) {
        if (key && value) {
            properties[*key] = *value;
        }
    }

    int GetProperty(std::string* key) const {
        if (!key) {
            return -1;
        }
        const auto it = properties.find(*key);
        return it != properties.end() ? it->second : -1;
    }
};

enum class ETestEnum {
    None = 0,
    Value1 = 1,
    Value2 = 2,
    Value3 = 3
};

REFLECTION_STRUCT(Vec3) {
    REFLECT_FIELD(x);
    REFLECT_FIELD(y);
    REFLECT_FIELD(z);
    REFLECT_METHOD_FAST(LengthSquared);
    REFLECT_METHOD_FAST(SetX);
};

REFLECTION_STRUCT(Transform) {
    REFLECT_FIELD(position)
        .DisplayName(shine::STextView::from_literal("World Position"))
        .Meta(shine::reflection::MetaKeys::Category, shine::STextView::from_literal("Transform"))
        .Meta(shine::STextView::from_literal("Tooltip"), shine::STextView::from_literal("World transform origin"));
    REFLECT_FIELD(rotation)
        .Meta(shine::reflection::MetaKeys::Category, shine::STextView::from_literal("Transform"));
    REFLECT_FIELD(scale)
        .Meta(shine::reflection::MetaKeys::Category, shine::STextView::from_literal("Transform"));
    REFLECT_FIELD(name)
        .EditAnywhere()
        .ScriptReadWrite()
        .UI(shine::reflection::UI::TextInput{128, false})
        .DisplayName(shine::STextView::from_literal("Actor Name"))
        .Meta(shine::reflection::MetaKeys::Category, shine::STextView::from_literal("Identity"));
    REFLECT_FIELD(id)
        .DisplayName(shine::STextView::from_literal("Actor Id"))
        .Range(0.0f, 100.0f)
        .Meta(shine::reflection::MetaKeys::Category, shine::STextView::from_literal("Identity"))
        .Meta(shine::reflection::MetaKeys::EditCondition, shine::STextView::from_literal("enabled"));
    REFLECT_FIELD(enabled)
        .DisplayName(shine::STextView::from_literal("Enabled"))
        .Meta(shine::reflection::MetaKeys::Category, shine::STextView::from_literal("Identity"));
    REFLECT_FIELD(tags)
        .Meta(shine::reflection::MetaKeys::Category, shine::STextView::from_literal("Collections"));
    REFLECT_FIELD(properties)
        .Meta(shine::reflection::MetaKeys::Category, shine::STextView::from_literal("Collections"));
    REFLECT_FIELD(flags)
        .Meta(shine::reflection::MetaKeys::Category, shine::STextView::from_literal("Collections"));

    REFLECT_METHOD_FAST(SetPosition)
        .ScriptCallable();
    REFLECT_METHOD_FAST(GetPosition);
    REFLECT_METHOD_FAST(AddTag)
        .ScriptCallable()
        .Meta(shine::reflection::MetaKeys::BlueprintFunction, true);
    REFLECT_METHOD_FAST(SetProperty);
    REFLECT_METHOD_FAST(GetProperty);
};

REFLECT_ENUM(ETestEnum) {
    builder.Enums({
        {ETestEnum::None, "None"},
        {ETestEnum::Value1, "Value1"},
        {ETestEnum::Value2, "Value2"},
        {ETestEnum::Value3, "Value3"}
    });
}

namespace shine::reflection {

template <>
struct StaticTypeRegistrationCTPlanProvider<Transform> {
    static constexpr bool enabled = true;

    static CTPlanView Get() {
        static const std::array<CTMetadataEntry, 1> kPositionRuntimeMetadata{{
            {Hash("Tooltip"), CTMetadataValue{shine::STextView::from_literal("World transform origin")}}
        }};
        static const std::array<CTMetadataEntry, 1> kAddTagRuntimeMetadata{{
            {MetaKeys::BlueprintFunction, CTMetadataValue{true}}
        }};
        static const std::array<TypeId, 3> kSetPositionParamTypes{{
            GetTypeId<float>(),
            GetTypeId<float>(),
            GetTypeId<float>()
        }};
        static const std::array<TypeId, 1> kAddTagParamTypes{{
            GetTypeId<int>()
        }};
        static const std::array<TypeId, 2> kSetPropertyParamTypes{{
            GetTypeId<std::string*>(),
            GetTypeId<int*>()
        }};
        static const std::array<TypeId, 1> kGetPropertyParamTypes{{
            GetTypeId<std::string*>()
        }};

        static const auto kFieldPlans = [] {
            std::array<CTFieldPlan, 9> plans{
                MakeCTFieldPlan<&Transform::position>(shine::STextView::from_literal("position")),
                MakeCTFieldPlan<&Transform::rotation>(shine::STextView::from_literal("rotation")),
                MakeCTFieldPlan<&Transform::scale>(shine::STextView::from_literal("scale")),
                MakeCTFieldPlan<&Transform::name>(shine::STextView::from_literal("name")),
                MakeCTFieldPlan<&Transform::id>(shine::STextView::from_literal("id")),
                MakeCTFieldPlan<&Transform::enabled>(shine::STextView::from_literal("enabled")),
                MakeCTFieldPlan<&Transform::tags>(shine::STextView::from_literal("tags")),
                MakeCTFieldPlan<&Transform::properties>(shine::STextView::from_literal("properties")),
                MakeCTFieldPlan<&Transform::flags>(shine::STextView::from_literal("flags"))
            };

            plans[0].builtinMetadata.displayName = shine::STextView::from_literal("World Position");
            plans[0].builtinMetadata.category = shine::STextView::from_literal("Transform");
            plans[0].runtimeMetadata = std::span<const CTMetadataEntry>{kPositionRuntimeMetadata};

            plans[1].builtinMetadata.category = shine::STextView::from_literal("Transform");
            plans[2].builtinMetadata.category = shine::STextView::from_literal("Transform");

            plans[3].flags = PropertyFlags::EditAnywhere | PropertyFlags::ScriptReadWrite;
            plans[3].builtinMetadata.hasUISchema = true;
            plans[3].builtinMetadata.uiSchema = UI::TextInput{128, false};
            plans[3].builtinMetadata.displayName = shine::STextView::from_literal("Actor Name");
            plans[3].builtinMetadata.category = shine::STextView::from_literal("Identity");

            plans[4].builtinMetadata.displayName = shine::STextView::from_literal("Actor Id");
            plans[4].builtinMetadata.category = shine::STextView::from_literal("Identity");
            plans[4].builtinMetadata.editCondition = shine::STextView::from_literal("enabled");
            plans[4].builtinMetadata.hasRange = true;
            plans[4].builtinMetadata.minValue = 0.0f;
            plans[4].builtinMetadata.maxValue = 100.0f;

            plans[5].builtinMetadata.displayName = shine::STextView::from_literal("Enabled");
            plans[5].builtinMetadata.category = shine::STextView::from_literal("Identity");

            plans[6].builtinMetadata.category = shine::STextView::from_literal("Collections");
            plans[7].builtinMetadata.category = shine::STextView::from_literal("Collections");
            plans[8].builtinMetadata.category = shine::STextView::from_literal("Collections");

            return plans;
        }();

        static const auto kMethodPlans = [] {
            std::array<CTMethodPlan, 5> plans{
                MakeCTMethodPlan<&Transform::SetPosition>(shine::STextView::from_literal("SetPosition"), std::span<const TypeId>{kSetPositionParamTypes}),
                MakeCTMethodPlan<&Transform::GetPosition>(shine::STextView::from_literal("GetPosition")),
                MakeCTMethodPlan<&Transform::AddTag>(shine::STextView::from_literal("AddTag"), std::span<const TypeId>{kAddTagParamTypes}),
                MakeCTMethodPlan<&Transform::SetProperty>(shine::STextView::from_literal("SetProperty"), std::span<const TypeId>{kSetPropertyParamTypes}),
                MakeCTMethodPlan<&Transform::GetProperty>(shine::STextView::from_literal("GetProperty"), std::span<const TypeId>{kGetPropertyParamTypes})
            };

            plans[0].flags = FunctionFlags::ScriptCallable;
            plans[2].flags = FunctionFlags::ScriptCallable;
            plans[2].runtimeMetadata = std::span<const CTMetadataEntry>{kAddTagRuntimeMetadata};
            return plans;
        }();

        static const CTPlanView plan{
            std::span<const CTFieldPlan>{kFieldPlans},
            std::span<const CTMethodPlan>{kMethodPlans},
            {}
        };
        return plan;
    }
};

template <>
struct StaticTypeRegistrationCTPlanProvider<ETestEnum> {
    static constexpr bool enabled = true;

    static CTPlanView Get() {
        static const std::array<CTEnumPlan, 4> kEnumPlans{{
            {static_cast<int64_t>(ETestEnum::None), shine::STextView::from_literal("None")},
            {static_cast<int64_t>(ETestEnum::Value1), shine::STextView::from_literal("Value1")},
            {static_cast<int64_t>(ETestEnum::Value2), shine::STextView::from_literal("Value2")},
            {static_cast<int64_t>(ETestEnum::Value3), shine::STextView::from_literal("Value3")}
        }};
        static const CTPlanView plan{
            {},
            {},
            std::span<const CTEnumPlan>{kEnumPlans}
        };
        return plan;
    }
};

} // namespace shine::reflection