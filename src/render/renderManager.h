#pragma once

#include "singleton.h"
#include "backend/render_backend_export.h"


namespace shine::render
{


	class RenderManager : public util::Singleton<RenderManager>
	{

	private:
		backend::SRenderBackend* RenderBackend = nullptr;


	public:


		backend::SRenderBackend* CreateRenderBackend();


		backend::SRenderBackend* GetRenderBackend()  noexcept
		{
			return RenderBackend;
		}




	};
}
