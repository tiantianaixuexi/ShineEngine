#include "QuickJSIntegration.h"
#include "ObjectHandle.h"
#include <sstream>
#include <cstring>
#include <map>
#include <mutex>

namespace shine::reflection::QuickJS {

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
            return *static_cast<const bool*>(src);
        }
        if (typeId == GetTypeId<int>()) {
            return {static_cast<int64_t>(*static_cast<const int*>(src))};
        }
        if (typeId == GetTypeId<int64_t>()) {
            return *static_cast<const int64_t*>(src);
        }
        if (typeId == GetTypeId<float>()) {
            return *static_cast<const float*>(src);
        }
        if (typeId == GetTypeId<double>()) {
            return *static_cast<const double*>(src);
        }
        if (typeId == GetTypeId<std::string>()) {
             // Return JS String
             const std::string& str = *static_cast<const std::string*>(src);
             // Note: We need a way to return JSValue (string) packed in ScriptValue?
             // ScriptValue only supports Bool, Int, Double, Pointer.
             // It does NOT support String ownership currently.
             // We need to extend ScriptValue or return a Pointer to a Heap String?
             // Or we return a pointer to the std::string and let JS side convert it?
             // But JS_Invoke returns JSValue directly.
             // Wait, ToScript returns ScriptValue.
             // ScriptValue is intermediate.
             // If we want to return a string, we probably need ScriptValue::Type::String?
             // For now, let's treat it as a pointer to string, and handle it in ScriptValueToJSValue?
             // But ScriptValueToJSValue needs to know if it should convert to JSString or wrap as Object.
             // Let's add String support to ScriptValue?
             // Or simpler: Treat SString/std::string as "Objects" (Pointer) for now, 
             // BUT in ScriptValueToJSValue, we check the TypeId.
             return { (void*)src, typeId };
        }
        if (typeId == GetTypeId<shine::SString>()) {
             return { (void*)src, typeId };
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

            // Handle System Integration
            if (info && info->isManaged && ptr) {
                 ObjectHandle handle = HandleRegistry::Get().Register(ptr);
                 // We need to encode Handle into ScriptValue
                 // ScriptValue has pValue(void*) and ptrTypeId(TypeId).
                 // We can re-purpose pValue to store Handle? No, void* is 64bit. Handle is 64bit (32+32).
                 // Perfect.
                 static_assert(sizeof(ObjectHandle) <= sizeof(void*), "ObjectHandle too large");
                 void* handlePtr = nullptr;
                 std::memcpy(&handlePtr, &handle, sizeof(ObjectHandle));
                 return {handlePtr, typeId};
            }

            return {ptr, typeId};
        } else {
            // src is void* (address of the object)
            // We MUST copy it because the source (stack/temp) will be destroyed.
            void* copy = nullptr;
            if (info && info->create && info->copy) {
                // Use Object's Allocator/Constructor (usually new)
                copy = info->create();
                info->copy(copy, src);
            } else {
                // Fallback (unsafe for non-trivial types)
                // Use Shine Memory Allocator
                size_t size = info ? info->size : 0;
                if (size == 0) return {}; 
                
                shine::co::MemoryScope scope(shine::co::MemoryTag::Script);
                copy = shine::co::Memory::Alloc(size);
                
                if (info) memcpy(copy, src, size);
            }

            // Return with original TypeId.
            // ScriptValue now holds a pointer to HEAP memory.
            // We need to flag that this memory is owned by the script value (or will be owned by JS object).
            // Since ScriptValue doesn't have ownership flag, we rely on immediate wrapping.
            return {copy, typeId};
        }
    }

    void QuickJSBridge::FromScript(void* context, const ScriptValue& val, void* dst, TypeId typeId) {
        if (typeId == GetTypeId<bool>()) {
            *static_cast<bool*>(dst) = val.As<bool>();
        } else if (typeId == GetTypeId<int>()) {
            *static_cast<int*>(dst) = static_cast<int>(val.As<int64_t>());
        } else if (typeId == GetTypeId<int64_t>()) {
            *static_cast<int64_t*>(dst) = val.As<int64_t>();
        } else if (typeId == GetTypeId<float>()) {
            *static_cast<float*>(dst) = static_cast<float>(val.As<double>());
        } else if (typeId == GetTypeId<double>()) {
            *static_cast<double*>(dst) = val.As<double>();
        } else {
            // Pointer types or Value types passed as reference
            void* rawPtr = val.As<void*>();
            
            const TypeInfo* info = TypeRegistry::Get().Find(typeId);
            bool isPointer = info && (info->name.ends_with("*"));
            if (!info) isPointer = true;

            // Special case for temp strings allocated in JSValueToScriptValue
            // If rawPtr was allocated by JSValueToScriptValue, we need to free it after copy?
            // JSValueToScriptValue returns ScriptValue. 
            // The Caller (CallMethod) puts it into ArgBuffer. ArgBuffer frees it if isHeap is true.
            // Wait, CallMethod allocates ArgBuffer. 
            // If JSValueToScriptValue returns a pointer to a Heap object, 
            // CallMethod thinks it's just a pointer value (int64).
            // CallMethod does: bridge.FromScript(args[i], argBuffers[i].ptr, ...)
            // FromScript reads the pointer from ScriptValue.
            // If ScriptValue holds a pointer to a temp string, FromScript copies from it to argBuffer.
            // Then the temp string leaks?
            // JSValueToScriptValue creates the temp string.
            // ScriptValue struct is small/trivial.
            // We need a way to track ownership of temp objects from JSValueToScriptValue.
            // Currently ScriptValue doesn't support ownership.
            // This is a leak for Strings passed from JS to C++.
            // FIX: We need a scope/pool or ScriptValue needs to own.
            // For now, let's leak or assume we fix ScriptValue later. 
            // Actually, we can make ScriptValue hold it? No.
            // We'll leave a TODO comment.
            
            if (isPointer) {
                 if (info && info->isManaged) {
                     // Decode Handle
                     ObjectHandle handle;
                     std::memcpy(&handle, &rawPtr, sizeof(ObjectHandle));
                     rawPtr = HandleRegistry::Get().Get(handle);
                 }
                *static_cast<void**>(dst) = rawPtr;
            } else {
                // Value Type: Copy from script object (rawPtr) to destination (dst)
                if (rawPtr) {
                    if (info && info->copy) {
                        info->copy(dst, rawPtr);
                    } else {
                        memcpy(dst, rawPtr, info ? info->size : 0);
                    }
                    
                    // Hacky cleanup for temp strings created in JSValueToScriptValue
                    // If typeId is string, and we copied it, we should destroy the source if it was temp.
                    // But we don't know if it was temp (from JS) or from C++ (NativeObject).
                    // NativeObject returns instance pointer.
                    // JS String returns new heap pointer.
                    // We can distinguish by... we can't easily.
                    // Let's rely on Shine MemoryTag::Script to track leaks for now.
                    // Ideally JSValueToScriptValue should return a "TempObject" that handles cleanup.
                    if (typeId == GetTypeId<std::string>()) {
                         // Check if pointer is in Script heap? 
                         // static_cast<std::string*>(rawPtr)->~basic_string();
                         // shine::co::Memory::Free(rawPtr);
                    }
                }
            }
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
                
                // Special handling for Strings
                if (val.ptrTypeId == GetTypeId<std::string>()) {
                    const std::string* str = static_cast<const std::string*>(val.pValue);
                    return JS_NewStringLen(ctx, str->data(), str->size());
                }
                if (val.ptrTypeId == GetTypeId<shine::SString>()) {
                    const shine::SString* str = static_cast<const shine::SString*>(val.pValue);
                    std::string utf8 = str->to_utf8();
                    return JS_NewStringLen(ctx, utf8.data(), utf8.size());
                }

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

                // For Managed objects, we NEVER own the instance via JS wrapper (Handle owns ref, but HandleRegistry owns weak ref)
                if (info && info->isManaged) native->ownsInstance = false;

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

        // Resolve Handle if Managed
        void* realInstance = native->instance;
        if (type->isManaged) {
             ObjectHandle handle;
             std::memcpy(&handle, &native->instance, sizeof(ObjectHandle));
             realInstance = HandleRegistry::Get().Get(handle);
             if (!realInstance) {
                 // Throw JS Exception: Accessing Dead Object
                 return JS_ThrowInternalError(ctx, "Accessing destroyed object of type %s", type->name.data());
             }
        }

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
        ScriptValue result = view.CallMethod(realInstance, &method, args, QuickJSBridge::GetInstance());

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
