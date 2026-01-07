#include "QuickJSIntegration.h"
#include <sstream>
#include <cstring>
#include <map>
#include <mutex>

namespace Shine::Reflection::QuickJS {

    // -------------------------------------------------------------------------
    // Registry for TypeId -> JSClassID
    // -------------------------------------------------------------------------
    static std::map<TypeId, JSClassID> g_typeToJSClass;
    static std::mutex g_registryMutex;

    void QuickJSBridge::RegisterJSClass(TypeId typeId, JSClassID classId) {
        std::lock_guard<std::mutex> lock(g_registryMutex);
        g_typeToJSClass[typeId] = classId;
    }

    JSClassID QuickJSBridge::GetJSClass(TypeId typeId) {
        std::lock_guard<std::mutex> lock(g_registryMutex);
        auto it = g_typeToJSClass.find(typeId);
        if (it != g_typeToJSClass.end()) return it->second;
        return 0;
    }

    // -------------------------------------------------------------------------
    // ScriptBridge Implementation
    // -------------------------------------------------------------------------

    ScriptValue QuickJSBridge::ToScript(void* context, const void* src, TypeId typeId) {
        if (typeId == GetTypeId<bool>()) {
            return ScriptValue(*static_cast<const bool*>(src));
        }
        if (typeId == GetTypeId<int>()) {
            return ScriptValue((int64_t)*static_cast<const int*>(src));
        }
        if (typeId == GetTypeId<int64_t>()) {
            return ScriptValue(*static_cast<const int64_t*>(src));
        }
        if (typeId == GetTypeId<float>()) {
            return ScriptValue((double)*static_cast<const float*>(src));
        }
        if (typeId == GetTypeId<double>()) {
            return ScriptValue(*static_cast<const double*>(src));
        }
        
        // Complex Types (Pointer or Value)
        const TypeInfo* info = TypeRegistry::Get().Find(typeId);
        
        // Heuristic: If name ends with *, it's a pointer. 
        // Note: This relies on TypeInfo being present. If not, we assume pointer (unsafe but common fallback).
        bool isPointer = info && (info->name.ends_with("*") || info->name.starts_with("struct ") && info->name.ends_with(" *")); 
        // Better: Check if size is pointer size? 
        if (!info) isPointer = true; // Fallback

        if (isPointer) {
            // src is void** (address of the pointer)
            void* ptr = *(void**)src;
            return ScriptValue(ptr, typeId);
        } else {
            // src is void* (address of the object)
            // We MUST copy it because the source (stack/temp) will be destroyed.
            size_t size = info ? info->size : 0;
            if (size == 0) return ScriptValue(); // Error

            void* copy = malloc(size);
            if (info && info->copy) {
                 info->copy(copy, src, size); // Use registered copy if available
            } else {
                 memcpy(copy, src, size); // Fallback to memcpy
            }
            // Return with original TypeId.
            // ScriptValue now holds a pointer to HEAP memory.
            // We need to flag that this memory is owned by the script value (or will be owned by JS object).
            // Since ScriptValue doesn't have ownership flag, we rely on immediate wrapping.
            return ScriptValue(copy, typeId);
        }
    }

    void QuickJSBridge::FromScript(void* context, const ScriptValue& val, void* dst, TypeId typeId) {
        if (typeId == GetTypeId<bool>()) {
            *static_cast<bool*>(dst) = val.As<bool>();
        } else if (typeId == GetTypeId<int>()) {
            *static_cast<int*>(dst) = (int)val.As<int64_t>();
        } else if (typeId == GetTypeId<int64_t>()) {
            *static_cast<int64_t*>(dst) = val.As<int64_t>();
        } else if (typeId == GetTypeId<float>()) {
            *static_cast<float*>(dst) = (float)val.As<double>();
        } else if (typeId == GetTypeId<double>()) {
            *static_cast<double*>(dst) = val.As<double>();
        } else {
            // Pointer types: dst is void**, val.As<void*>() is the pointer
            *static_cast<void**>(dst) = val.As<void*>();
        }
    }

    const ScriptBridge& QuickJSBridge::GetInstance() {
        static ScriptBridge bridge = {
            nullptr, // Context not used for primitive conversions
            &ToScript,
            &FromScript
        };
        return bridge;
    }

    // -------------------------------------------------------------------------
    // JS_Invoke Implementation
    // -------------------------------------------------------------------------

    static JSNativeObject* GetNative(JSContext* ctx, JSValueConst val) {
        // Safe check using a known ClassID would be better, but we are generic.
        // We assume the object is a JSNativeObject.
        return (JSNativeObject*)JS_GetOpaque2(ctx, val, 0);
    }

    ScriptValue JSValueToScriptValue(JSContext* ctx, JSValueConst val, TypeId targetType) {
        if (JS_IsBool(val)) {
            return ScriptValue((bool)JS_ToBool(ctx, val));
        }
        if (JS_IsNumber(val)) {
            if (targetType == GetTypeId<int>() || targetType == GetTypeId<int64_t>()) {
                int64_t v;
                JS_ToInt64(ctx, &v, val);
                return ScriptValue(v);
            }
            double d;
            JS_ToFloat64(ctx, &d, val);
            return ScriptValue(d);
        }
        if (JS_IsObject(val)) {
            JSNativeObject* native = GetNative(ctx, val);
            if (native) {
                return ScriptValue(native->instance, native->type);
            }
            return ScriptValue(nullptr);
        }
        if (JS_IsNull(val) || JS_IsUndefined(val)) {
            return ScriptValue();
        }
        return ScriptValue();
    }

    JSValue ScriptValueToJSValue(JSContext* ctx, const ScriptValue& val) {
        switch (val.type) {
            case ScriptValue::Type::Bool: return JS_NewBool(ctx, val.bValue);
            case ScriptValue::Type::Int64: return JS_NewInt64(ctx, val.iValue);
            case ScriptValue::Type::Double: return JS_NewFloat64(ctx, val.dValue);
            case ScriptValue::Type::Pointer: {
                if (!val.pValue) return JS_NULL;
                
                // Find the JSClassID for this TypeId
                JSClassID classId = QuickJSBridge::GetJSClass(val.ptrTypeId);
                if (classId == 0) {
                    // Fallback: Return as generic object or null?
                    // If we can't find class ID, we can't wrap it typed.
                    return JS_NULL; 
                }

                JSValue obj = JS_NewObjectClass(ctx, classId);
                if (JS_IsException(obj)) return obj;

                JSNativeObject* native = new JSNativeObject();
                native->instance = val.pValue;
                native->type = val.ptrTypeId;
                
                // Heuristic for ownership:
                // If it was copied in ToScript (Value type), we own it.
                // We check TypeRegistry.
                const TypeInfo* info = TypeRegistry::Get().Find(val.ptrTypeId);
                bool isPointer = info && (info->name.ends_with("*"));
                if (!info) isPointer = true;

                native->ownsInstance = !isPointer; // If it's not a pointer type, we own the copy.

                JS_SetOpaque(obj, native);
                return obj;
            }
            default: return JS_UNDEFINED;
        }
    }

    JSValue JS_Invoke(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic) {
        // 1. Get Native Instance
        JSNativeObject* native = GetNative(ctx, this_val);
        if (!native || !native->instance) return JS_EXCEPTION;

        // 2. Look up Type and Method
        const TypeInfo* type = TypeRegistry::Get().Find(native->type);
        if (!type) return JS_EXCEPTION;

        // magic is the index in type->methods
        if (magic < 0 || static_cast<size_t>(magic) >= type->methods.size()) return JS_EXCEPTION;
        const MethodInfo& method = type->methods[magic];

        // 3. Convert Arguments
        std::vector<ScriptValue> args;
        args.reserve(argc);
        for (int i = 0; i < argc; ++i) {
            // Map JS args to expected param types
            TypeId paramType = (static_cast<size_t>(i) < method.paramTypes.size()) ? method.paramTypes[i] : 0;
            args.push_back(JSValueToScriptValue(ctx, argv[i], paramType));
        }

        // 4. Call via ScriptView
        ScriptView view;
        view.typeInfo = type;
        ScriptValue result = view.CallMethod(native->instance, &method, args, QuickJSBridge::GetInstance());

        // 5. Convert Return Value
        return ScriptValueToJSValue(ctx, result);
    }

    // -------------------------------------------------------------------------
    // Glue Codegen
    // -------------------------------------------------------------------------
    std::string GenerateGlueCode(const TypeInfo* type) {
        std::stringstream ss;
        std::string typeName(type->name);
        
        ss << "// Generated Glue for " << typeName << "\n";
        ss << "static JSClassID js_" << typeName << "_class_id;\n\n";

        // Generate Method List
        ss << "static const JSCFunctionListEntry js_" << typeName << "_funcs[] = {\n";
        for (size_t i = 0; i < type->methods.size(); ++i) {
            const auto& m = type->methods[i];
            ss << "    JS_CFUNC_MAGIC_DEF(\"" << m.name << "\", " 
               << m.paramTypes.size() << ", "
               << "Shine::Reflection::QuickJS::JS_Invoke, " 
               << i << "), // magic = " << i << "\n";
        }
        ss << "};\n\n";

        // Class Def
        ss << "static JSClassDef js_" << typeName << "_class = {\n";
        ss << "    \"" << typeName << "\",\n";
        ss << "    .finalizer = [](JSRuntime* rt, JSValue val) {\n";
        ss << "        JSNativeObject* native = (JSNativeObject*)JS_GetOpaque(val, js_" << typeName << "_class_id);\n";
        ss << "        if (native) {\n";
        ss << "            if (native->ownsInstance) {\n";
        ss << "                // We need to cast to correct type to delete, or use virtual destructor?\n";
        ss << "                // Since we don't have polymorphic delete in TypeInfo easily (only destroy void*),\n";
        ss << "                // we use TypeInfo::destroy if available.\n";
        ss << "                // But we need to look it up.\n";
        ss << "                const auto* info = Shine::Reflection::TypeRegistry::Get().Find(native->type);\n";
        ss << "                if (info && info->destroy) info->destroy(native->instance);\n";
        ss << "                else free(native->instance); // Fallback for malloc\n";
        ss << "            }\n";
        ss << "            delete native;\n";
        ss << "        }\n";
        ss << "    }\n";
        ss << "};\n\n";

        // Registration Helper
        ss << "void Register_" << typeName << "(JSContext* ctx) {\n";
        ss << "    JS_NewClassID(&js_" << typeName << "_class_id);\n";
        ss << "    JS_NewClass(JS_GetRuntime(ctx), js_" << typeName << "_class_id, &js_" << typeName << "_class);\n";
        ss << "    JSValue proto = JS_NewObject(ctx);\n";
        ss << "    JS_SetPropertyFunctionList(ctx, proto, js_" << typeName << "_funcs, countof(js_" << typeName << "_funcs));\n";
        ss << "    JS_SetClassProto(ctx, js_" << typeName << "_class_id, proto);\n";
        ss << "    // Register TypeId -> ClassID mapping\n";
        // We register both T and T* to the same ClassID, so both return types work.
        ss << "    Shine::Reflection::QuickJS::QuickJSBridge::RegisterJSClass(Shine::Reflection::GetTypeId<" << typeName << ">(), js_" << typeName << "_class_id);\n";
        ss << "    Shine::Reflection::QuickJS::QuickJSBridge::RegisterJSClass(Shine::Reflection::GetTypeId<" << typeName << "*>(), js_" << typeName << "_class_id);\n";
        ss << "}\n";

        return ss.str();
    }

}
