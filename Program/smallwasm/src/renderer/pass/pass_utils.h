#pragma once

#include "../render_pipeline.h"

class DemoGame;

namespace shine { namespace renderer {

void RenderSceneAndFlush(DemoGame& game, const RenderPass& pass, float t);

} } // namespace shine::renderer
