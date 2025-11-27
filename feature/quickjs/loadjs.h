//JSRuntime* runtime = JS_NewRuntime();
	//JSContext* ctx = JS_NewContext(runtime);

	//JSValue global = JS_GetGlobalObject(ctx);
	//JS_SetPropertyStr(ctx, global, "MoveActor", JS_NewCFunction(ctx, js_MoveActor, "MoveActor", 3));
	//JS_SetPropertyStr(ctx, global, "Log",JS_NewCFunction(ctx, js_Log, "Log", 1));
	//JS_FreeValue(ctx, global);

	//JSValue updateFunc{};
	//auto script_path = shine::util::get_script_path("game.js");
	//if (!script_path.has_value()) {
	//	fmt::print("无法获取脚本文件路径\n");
	//	return 1;
	//}
	//auto result = shine::util::read_file_text(script_path.value());
	//if (result.has_value())
	//{
	//	std::string scriptCode = result.value();
	//	JS_Eval(ctx, scriptCode.c_str(), scriptCode.size(), "game.js", JS_EVAL_TYPE_GLOBAL);

	//	// 获取 update 函数
	//	updateFunc = JS_GetPropertyStr(ctx,global, "update");
	//	if (!JS_IsFunction(ctx, updateFunc)) {
	//		fmt::print("update function not found in script");
	//		return 1;
	//	}
	//}
	//else
	//{
	//	fmt::print("无法读取脚本文件: {}", result.error());
	//}
	//


//while(true){

	//if (JS_IsExtensible(ctx, updateFunc)) {
		//	JSValue arg = JS_NewFloat64(ctx, dt_d); // 转换为秒
		//	JSValue ret = JS_Call(ctx, updateFunc, JS_UNDEFINED, 1, &arg);
		//	JS_FreeValue(ctx, arg);

		//	if (JS_IsException(ret)) {
		//		JSValue error = JS_GetException(ctx);

		//		// 转成字符串
		//		const char* error_str = JS_ToCString(ctx, error);
		//		fmt::print("脚本运行时错误: {}\n", error_str);
		//		JS_FreeCString(ctx, error_str);

		//		// 处理 error.stack (traceback)
		//		JSValue stack = JS_GetPropertyStr(ctx, error, "stack");
		//		if (!JS_IsUndefined(stack)) {
		//			const char* stack_str = JS_ToCString(ctx, stack);
		//			fmt::print("脚本调用栈:\n{}\n", stack_str);
		//			JS_FreeCString(ctx, stack_str);
		//		}

		//		JS_FreeValue(ctx, stack);
		//		JS_FreeValue(ctx, error);
		//	}
		//	JS_FreeValue(ctx, ret);
		//}
/}