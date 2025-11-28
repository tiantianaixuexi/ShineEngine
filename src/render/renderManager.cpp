#include "renderManager.h"




namespace shine::render
{

	void RenderManager::CreateRenderBackend()
	{
		RenderBackend = new backend::SRenderBackend();
	}


}
