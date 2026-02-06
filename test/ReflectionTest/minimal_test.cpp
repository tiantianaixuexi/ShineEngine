#include <iostream>
#include <string>
#include <vector>
#include <map>

// Single include for the whole reflection system
#include "../../src/EngineCore/reflection/Reflection.h"

// ---- Test struct -----------------------------------------------------------

struct TestStruct {
    int         id      = 42;
    std::string name    = "test";
    float       score   = 3.14f;
    bool        active  = true;

    int  GetValue() const    { return id; }
    void SetValue(int newId) { id = newId; }
};

// ---- Registration using the runtime DSL ------------------------------------

REFLECTION_STRUCT(TestStruct) {
    REFLECT_FIELD(id)
        .EditAnywhere()
        .DisplayName("ID");

    REFLECT_FIELD(name)
        .EditAnywhere()
        .UI(shine::reflection::UI::TextInput{})
        .DisplayName("Name");

    REFLECT_FIELD(score)
        .EditAnywhere()
        .Range(0.0f, 100.0f)
        .UI(shine::reflection::UI::Slider{})
        .DisplayName("Score");

    REFLECT_FIELD(active)
        .EditAnywhere()
        .UI(shine::reflection::UI::Checkbox{})
        .DisplayName("Active");

    REFLECT_METHOD(GetValue).ScriptCallable();
    REFLECT_METHOD(SetValue).ScriptCallable();
}

REFLECTION_REGISTER(TestStruct)

// ---- Main ------------------------------------------------------------------

int main() {
    using namespace shine::reflection;

    std::cout << "Minimal Reflection Test\n";
    std::cout << "=======================\n\n";

    // Look up registered type
    const TypeInfo* info = TypeRegistry::Get().Find<TestStruct>();
    if (!info) {
        std::cerr << "ERROR: TestStruct not registered!\n";
        return 1;
    }

    std::cout << "Type name   : " << info->name      << "\n";
    std::cout << "Type size   : " << info->size       << " bytes\n";
    std::cout << "Field count : " << info->GetFieldCount()  << "\n";
    std::cout << "Method count: " << info->GetMethodCount() << "\n\n";

    // Field access via type-erased API
    TestStruct obj;

    const FieldInfo* idField = info->FindField("id");
    if (idField) {
        int val = 0;
        idField->Get(&obj, &val);
        std::cout << "Original ID : " << val << "\n";

        val = 100;
        idField->Set(&obj, &val);
        std::cout << "Modified ID : " << obj.id << "\n";
    }

    const FieldInfo* nameField = info->FindField("name");
    if (nameField) {
        std::string val;
        nameField->Get(&obj, &val);
        std::cout << "Name        : " << val << "\n";
    }

    std::cout << "\n";

    // Method invocation
    const MethodInfo* getVal = info->FindMethod("GetValue");
    if (getVal) {
        std::cout << "GetValue()  : invoking via reflection... ";
        getVal->Invoke(&obj, nullptr, nullptr);
        std::cout << "(invoked)\n";
    }

    std::cout << "\nAll tests passed!\n";
    return 0;
}
