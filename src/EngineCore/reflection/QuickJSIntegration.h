#pragma once
#include "Reflection.h"
#include <vector>
#include <string>

// Assume standard QuickJS API
extern "C" {
#include "quickjs/quickjs.h"
}

namespace shine::reflection::QuickJS {

    // -------------------------------------------------------------------------
    // 4. this / Native Handle Specification
    // -------------------------------------------------------------------------
    struct JSNativeObject {
        void* instance;
        TypeId type;
        bool ownsInstance = false;
    };

    // -------------------------------------------------------------------------
    // 5. ScriptBridge x QuickJS Adaption
    // -------------------------------------------------------------------------
    class QuickJSBridge {
    public:
        // Singleton access to the bridge instance
        static const ScriptBridge& GetInstance();
        
        // Registry for TypeId -> JSClassID mapping
        static void RegisterJSClass(TypeId typeId, JSClassID classId);
        static JSClassID GetJSClass(TypeId typeId);
    
    private:
        // Bridge implementations
        static ScriptValue ToScript(void* context, const void* src, TypeId typeId);
        static void FromScript(void* context, const ScriptValue& val, void* dst, TypeId typeId);
    };

    // -------------------------------------------------------------------------
    // 2. ABI Frozen Entry Point
    // -------------------------------------------------------------------------
    JSValue JS_Invoke(
        JSContext* ctx, 
        JSValueConst this_val, 
        int argc, 
        JSValueConst* argv, 
        int magic /* MethodIndex */ 
    );

    // -------------------------------------------------------------------------
    // Helpers (Internal Use)
    // -------------------------------------------------------------------------
    ScriptValue JSValueToScriptValue(JSContext* ctx, JSValueConst val, TypeId targetType);
    JSValue ScriptValueToJSValue(JSContext* ctx, const ScriptValue& val);

    // -------------------------------------------------------------------------
    // 2. QuickJS glue codegen
    // -------------------------------------------------------------------------
    // Generates the C++ glue code for a given TypeInfo
    std::string GenerateGlueCode(const TypeInfo* typeInfo);
}
