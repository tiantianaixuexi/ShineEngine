namespace shine::manager
{

    // bool PluginsManager::loadPlugin(const std::string& plugin_path)
    // {
    //     // 妫€鏌ユ枃浠舵槸鍚﹀瓨鍦?
    //     if (!std::filesystem::exists(plugin_path))
    //     {
    //         return false;
    //     }

    //     // 鍔犺浇DLL
    //     HMODULE plugin_handle = LoadLibraryA(plugin_path.c_str());
    //     if (!plugin_handle)
    //     {
    //         return false;
    //     }

    //     // // 鑾峰彇鎻掍欢鍒涘缓鍑芥暟
    //     // using CreatePluginFunc = PluginBase* (*)();

    //     // auto create_plugin_func = reinterpret_cast<CreatePluginFunc>(
    //     //     GetProcAddress(plugin_handle, "createPlugin"));

    //     // if (!create_plugin_func)
    //     // {
    //     //     FreeLibrary(plugin_handle);
    //     //     return false;
    //     // }

    //     // // 鍒涘缓鎻掍欢瀹炰緥
    //     // PluginBase* plugin = create_plugin_func();
    //     // if (!plugin)
    //     // {
    //     //     FreeLibrary(plugin_handle);
    //     //     return false;
    //     // }

    //     // 鑾峰彇鎻掍欢鍚嶇О
    //     std::string plugin_name = plugin->getMeta().name;

    //     // // 鍒濆鍖栨彃浠?
    //     // if (!plugin->initialize())
    //     // {
    //     //     delete plugin;
    //     //     FreeLibrary(plugin_handle);
    //     //     return false;
    //     // }

    //     // 淇濆瓨鎻掍欢鍙ユ焺
    //     //m_plugin_handles[plugin_name] = plugin_handle;

    //     return true;
    // }

}


