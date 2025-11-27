
JavaScript 继承 SObject 的完整解决方案：
#pragma once

#include "quickjs/quickjs.h"
#include "gameplay/object.h"
#include <unordered_map>
#include <memory>

namespace shine::script {

// QuickJS 类 ID
static JSClassID js_sobject_class_id = 0;

// 用于存储 JS 继承类的方法引用
struct JSObjectMethodRefs {
	JSValue onTick = JS_UNDEFINED;
	JSValue onBeginPlay = JS_UNDEFINED;
	JSValue onInit = JS_UNDEFINED;
};

// 全局方法映射表（JavaScript 实例 -> C++ 方法引用）
static std::unordered_map<uintptr_t, JSObjectMethodRefs> g_jsMethodRefs;

// ================== SObject 构造函数 ==================
static JSValue js_sobject_constructor(JSContext* ctx, JSValueConst new_target, 
                                     int argc, JSValueConst* argv)
{
	auto* obj = new shine::gameplay::SObject();
	
	if (argc > 0) {
		const char* name = JS_ToCString(ctx, argv[0]);
		if (name) {
			obj->setName(name);
			JS_FreeCString(ctx, name);
		}
	}
	
	JSValue obj_val = JS_NewObjectClass(ctx, js_sobject_class_id);
	if (JS_IsException(obj_val)) {
		delete obj;
		return obj_val;
	}
	
	JS_SetOpaque(obj_val, obj);
	
	// 保存 JS 对象引用，用于回调
	uintptr_t obj_ptr = reinterpret_cast<uintptr_t>(obj);
	JSObjectMethodRefs refs;
	g_jsMethodRefs[obj_ptr] = refs;
	
	return obj_val;
}

// ================== SObject 析构函数 ==================
static void js_sobject_finalizer(JSRuntime* rt, JSValueConst val)
{
	auto* obj = static_cast<shine::gameplay::SObject*>(JS_GetOpaque(val, js_sobject_class_id));
	if (obj) {
		uintptr_t obj_ptr = reinterpret_cast<uintptr_t>(obj);
		auto it = g_jsMethodRefs.find(obj_ptr);
		if (it != g_jsMethodRefs.end()) {
			JSFreeValue(rt, it->second.onTick);
			JS_FreeValueRT(rt, it->second.onBeginPlay);
			JS_FreeValueRT(rt, it->second.onInit);
			g_jsMethodRefs.erase(it);
		}
		delete obj;
	}
}

// ================== 基础方法实现 ==================
static JSValue js_sobject_setName(JSContext* ctx, JSValueConst this_val, 
                                 int argc, JSValueConst* argv)
{
	auto* obj = static_cast<shine::gameplay::SObject*>(JS_GetOpaque2(ctx, this_val, js_sobject_class_id));
	if (!obj) return JS_EXCEPTION;
	
	if (argc < 1) return JS_ThrowTypeError(ctx, "setName requires 1 argument");
	
	const char* name = JS_ToCString(ctx, argv[0]);
	if (name) {
		obj->setName(name);
		JS_FreeCString(ctx, name);
	}
	return JS_UNDEFINED;
}

static JSValue js_sobject_getName(JSContext* ctx, JSValueConst this_val, 
                                 int argc, JSValueConst* argv)
{
	auto* obj = static_cast<shine::gameplay::SObject*>(JS_GetOpaque2(ctx, this_val, js_sobject_class_id));
	if (!obj) return JS_EXCEPTION;
	return JS_NewString(ctx, obj->getName().c_str());
}

static JSValue js_sobject_setActive(JSContext* ctx, JSValueConst this_val, 
                                   int argc, JSValueConst* argv)
{
	auto* obj = static_cast<shine::gameplay::SObject*>(JS_GetOpaque2(ctx, this_val, js_sobject_class_id));
	if (!obj) return JS_EXCEPTION;
	if (argc < 1) return JS_ThrowTypeError(ctx, "setActive requires 1 argument");
	
	obj->setActive(JS_ToBool(ctx, argv[0]));
	return JS_UNDEFINED;
}

static JSValue js_sobject_isActive(JSContext* ctx, JSValueConst this_val, 
                                  int argc, JSValueConst* argv)
{
	auto* obj = static_cast<shine::gameplay::SObject*>(JS_GetOpaque2(ctx, this_val, js_sobject_class_id));
	if (!obj) return JS_EXCEPTION;
	return JS_NewBool(ctx, obj->isActive());
}

// ================== 虚方法处理 ==================
// 当 C++ 代码调用这些虚方法时，如果存在 JS 继承类，则调用 JS 实现

static JSValue js_sobject_onTick(JSContext* ctx, JSValueConst this_val, 
                                int argc, JSValueConst* argv)
{
	auto* obj = static_cast<shine::gameplay::SObject*>(JS_GetOpaque2(ctx, this_val, js_sobject_class_id));
	if (!obj) return JS_EXCEPTION;
	if (argc < 1) return JS_ThrowTypeError(ctx, "onTick requires 1 argument");
	
	double dt = 0.0;
	if (JS_ToFloat64(ctx, &dt, argv[0]) < 0) return JS_EXCEPTION;
	
	// 检查是否有 JS 继承的 onTick 方法
	uintptr_t obj_ptr = reinterpret_cast<uintptr_t>(obj);
	auto it = g_jsMethodRefs.find(obj_ptr);
	if (it != g_jsMethodRefs.end() && !JS_IsUndefined(it->second.onTick)) {
		// 调用 JS 中的 onTick
		JSValue args[1] = { JS_NewFloat64(ctx, dt) };
		JSValue ret = JS_Call(ctx, it->second.onTick, this_val, 1, args);
		JS_FreeValue(ctx, args[0]);
		if (JS_IsException(ret)) {
			JS_FreeValue(ctx, ret);
			return JS_EXCEPTION;
		}
		JS_FreeValue(ctx, ret);
	} else {
		// 调用 C++ 的基础实现
		obj->onTick(static_cast<float>(dt));
	}
	
	return JS_UNDEFINED;
}

static JSValue js_sobject_onBeginPlay(JSContext* ctx, JSValueConst this_val, 
                                     int argc, JSValueConst* argv)
{
	auto* obj = static_cast<shine::gameplay::SObject*>(JS_GetOpaque2(ctx, this_val, js_sobject_class_id));
	if (!obj) return JS_EXCEPTION;
	
	uintptr_t obj_ptr = reinterpret_cast<uintptr_t>(obj);
	auto it = g_jsMethodRefs.find(obj_ptr);
	if (it != g_jsMethodRefs.end() && !JS_IsUndefined(it->second.onBeginPlay)) {
		JSValue ret = JS_Call(ctx, it->second.onBeginPlay, this_val, 0, nullptr);
		if (JS_IsException(ret)) {
			JS_FreeValue(ctx, ret);
			return JS_EXCEPTION;
		}
		JS_FreeValue(ctx, ret);
	} else {
		obj->onBeginPlay();
	}
	
	return JS_UNDEFINED;
}

static JSValue js_sobject_onInit(JSContext* ctx, JSValueConst this_val, 
                                int argc, JSValueConst* argv)
{
	auto* obj = static_cast<shine::gameplay::SObject*>(JS_GetOpaque2(ctx, this_val, js_sobject_class_id));
	if (!obj) return JS_EXCEPTION;
	
	uintptr_t obj_ptr = reinterpret_cast<uintptr_t>(obj);
	auto it = g_jsMethodRefs.find(obj_ptr);
	if (it != g_jsMethodRefs.end() && !JS_IsUndefined(it->second.onInit)) {
		JSValue ret = JS_Call(ctx, it->second.onInit, this_val, 0, nullptr);
		if (JS_IsException(ret)) {
			JS_FreeValue(ctx, ret);
			return JS_EXCEPTION;
		}
		JS_FreeValue(ctx, ret);
	} else {
		obj->OnInit();
	}
	
	return JS_UNDEFINED;
}

// ================== 设置 JS 继承方法 ==================
// 此函数由 JS 在定义子类时调用，用于注册要覆盖的方法

static JSValue js_sobject_setOverrideMethod(JSContext* ctx, JSValueConst this_val, 
                                           int argc, JSValueConst* argv)
{
	auto* obj = static_cast<shine::gameplay::SObject*>(JS_GetOpaque2(ctx, this_val, js_sobject_class_id));
	if (!obj) return JS_EXCEPTION;
	if (argc < 2) return JS_ThrowTypeError(ctx, "setOverrideMethod requires 2 arguments");
	
	const char* method_name = JS_ToCString(ctx, argv[0]);
	if (!method_name) return JS_EXCEPTION;
	
	if (!JS_IsFunction(ctx, argv[1])) {
		JS_FreeCString(ctx, method_name);
		return JS_ThrowTypeError(ctx, "second argument must be a function");
	}
	
	uintptr_t obj_ptr = reinterpret_cast<uintptr_t>(obj);
	auto it = g_jsMethodRefs.find(obj_ptr);
	if (it == g_jsMethodRefs.end()) {
		JS_FreeCString(ctx, method_name);
		return JS_EXCEPTION;
	}
	
	JSValue func_copy = JS_DupValue(ctx, argv[1]);
	
	if (strcmp(method_name, "onTick") == 0) {
		if (!JS_IsUndefined(it->second.onTick)) {
			JS_FreeValue(ctx, it->second.onTick);
		}
		it->second.onTick = func_copy;
	} else if (strcmp(method_name, "onBeginPlay") == 0) {
		if (!JS_IsUndefined(it->second.onBeginPlay)) {
			JS_FreeValue(ctx, it->second.onBeginPlay);
		}
		it->second.onBeginPlay = func_copy;
	} else if (strcmp(method_name, "onInit") == 0) {
		if (!JS_IsUndefined(it->second.onInit)) {
			JS_FreeValue(ctx, it->second.onInit);
		}
		it->second.onInit = func_copy;
	} else {
		JS_FreeValue(ctx, func_copy);
		JS_FreeCString(ctx, method_name);
		return JS_ThrowTypeError(ctx, "unknown method name");
	}
	
	JS_FreeCString(ctx, method_name);
	return JS_UNDEFINED;
}

// ================== 初始化 SObject 类 ==================
static void js_init_sobject_class(JSContext* ctx)
{
	JSRuntime* rt = JS_GetRuntime(ctx);
	JS_NewClassID(rt, &js_sobject_class_id);
	
	JSCFunctionListEntry sobject_methods[] = {
		JS_CFUNC_DEF("setName", 1, js_sobject_setName),
		JS_CFUNC_DEF("getName", 0, js_sobject_getName),
		JS_CFUNC_DEF("setActive", 1, js_sobject_setActive),
		JS_CFUNC_DEF("isActive", 0, js_sobject_isActive),
		JS_CFUNC_DEF("onTick", 1, js_sobject_onTick),
		JS_CFUNC_DEF("onBeginPlay", 0, js_sobject_onBeginPlay),
		JS_CFUNC_DEF("onInit", 0, js_sobject_onInit),
		JS_CFUNC_DEF("_setOverrideMethod", 2, js_sobject_setOverrideMethod),
	};
	
	JSValue proto = JS_NewObject(ctx);
	JS_SetPropertyFunctionList(ctx, proto, sobject_methods, 
							  countof(sobject_methods));
	
	JSClassDef sobject_class = {
		"SObject",
		.finalizer = js_sobject_finalizer,
	};
	JS_NewClass(rt, js_sobject_class_id, &sobject_class);
	JS_SetClassProto(ctx, js_sobject_class_id, proto);
	
	JSValue constructor = JS_NewCFunction2(ctx, js_sobject_constructor, 
										 "SObject", 1, JS_CFUNC_constructor, 0);
	JS_SetConstructorBit(ctx, constructor, true);
	
	JSValue global = JS_GetGlobalObject(ctx);
	JS_SetPropertyStr(ctx, global, "SObject", constructor);
	JS_FreeValue(ctx, global);
	JS_FreeValue(ctx, proto);
}

} // namespace shine::script

// 创建基础 SObject
const baseActor = new SObject("BaseActor");

// ========== 方式1：在实例上覆盖方法 ==========
class Player extends SObject {
    speed: number = 5.0;
    position: number = 0;

    constructor(name: string) {
        super(name);
        
        // 覆盖 onTick 方法
        this._setOverrideMethod("onTick", (deltaTime: number) => {
            this.position += this.speed * deltaTime;
            console.log(`${this.getName()} moved to ${this.position}`);
        });
        
        // 覆盖 onBeginPlay 方法
        this._setOverrideMethod("onBeginPlay", () => {
            console.log(`${this.getName()} 游戏开始了！`);
        });
    }
}

// ========== 方式2：创建实例并在外部设置覆盖 ==========
const enemy = new SObject("Enemy");

enemy._setOverrideMethod("onTick", function(deltaTime: number) {
    console.log(`敌人正在执行 AI，deltaTime: ${deltaTime}`);
});

// ========== 方式3：组合多个虚方法 ==========
class NPC extends SObject {
    constructor(name: string) {
        super(name);
        this.setupBehavior();
    }

    private setupBehavior() {
        this._setOverrideMethod("onInit", () => {
            console.log("NPC 初始化完成");
            this.setActive(true);
        });

        this._setOverrideMethod("onBeginPlay", () => {
            console.log(`${this.getName()} 开始活动`);
        });

        this._setOverrideMethod("onTick", (deltaTime: number) => {
            if (this.isActive()) {
                console.log(`NPC 每帧更新: ${deltaTime}s`);
            }
        });
    }
}

// ========== 在游戏循环中使用 ==========
function update(deltaTime: number) {
    // 创建并更新玩家
    const player = new Player("MainPlayer");
    player.onBeginPlay();  // 调用 JS 实现的 onBeginPlay
    player.onTick(deltaTime);  // 调用 JS 实现的 onTick

    // 创建并更新 NPC
    const npc = new NPC("Merchant");
    npc.onInit();
    npc.onTick(deltaTime);

    // 创建并更新敌人
    enemy.onTick(deltaTime);
}