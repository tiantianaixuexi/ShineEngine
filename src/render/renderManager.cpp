#include "renderManager.h"




namespace shine::render
{

	backend::SRenderBackend* RenderManager::CreateRenderBackend()
	{
		return RenderBackend = new backend::SRenderBackend();
	}

	
}
