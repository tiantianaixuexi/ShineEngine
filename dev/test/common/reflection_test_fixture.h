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