
    //// 创建一个三维立方体网格并加载到对象上，注册到渲染服务
    //{
    //    auto mesh = std::make_shared<shine::gameplay::StaticMesh>();
    //    mesh->initCubeWithNormals();
    //    // 使用全局共享的PBR材质，可在UI调节点参数
    //    mesh->setMaterial(shine::render::Material::GetPBR());
    //    auto* smc = g_TestActor.addComponent<shine::gameplay::component::StaticMeshComponent>();
    //    smc->setMesh(mesh);
    //    shine::render::RendererService::get().registerObject(&g_TestActor);
    //}
	//
	//// 加载纹理示例（类似 UE5 的 LoadObject，一步到位）
	//{
	//	shine::util::FunctionTimer __function_timer__("加载纹理", shine::util::TimerPrecision::Nanoseconds);
	//	
	//	auto texture = shine::manager::AssetManager::Get().LoadTexture("test_texture.png");
	//	if (texture && mainEditor->imageViewerView)
	//	{
	//		mainEditor->imageViewerView->SetTexture(texture);
	//	}
	//}

	
	// ========================================================================
	// 资源统计信息
	// ========================================================================
	//size_t textureCount, totalMemory;
	//shine::render::TextureManager::get().GetTextureStats(textureCount, totalMemory);
	//fmt::println("纹理统计: GPU纹理数量={}, 估算内存={} KB", textureCount, totalMemory / 1024);
	
	// 注意：AssetManager 的资源统计需要额外实现，这里只是示例
	//fmt::println("提示: AssetManager 管理资源加载，TextureManager 管理 GPU 纹理");
	//fmt::println("     同一资源可以被多个纹理复用，提高内存效率");