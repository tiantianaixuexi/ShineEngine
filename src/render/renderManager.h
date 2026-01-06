#pragma once

#include "EngineCore/subsystem.h"
#include "backend/render_backend_export.h"

namespace shine::render
{
	class RenderManager : public shine::Subsystem
	{
	public:

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
