#pragma once

#include "../../EngineCore/subsystem.h"
#include "../../EngineCore/engine_context.h"
#include "backend/render_backend_export.h"

namespace shine::render
{
	class RenderManager : public shine::Subsystem
	{
	public:
		static constexpr size_t GetStaticID() { return shine::HashString("RenderManager"); }

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
