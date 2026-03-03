#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "EngineCore/subsystem.h"
#include "EngineCore/engine_context.h"
#include "render/debug/debug_texture_sink.h"
#include "render/pipeline/render_pipeline.h"
#include "shine_define.h"

namespace shine::render
{
    struct DebugTextureEntry
    {
        u32 id = 0;
        int width = 0;
        int height = 0;
    };

    class PassTextureManager : public shine::Subsystem, public DebugTextureSink
    {
    public:
        static PassTextureManager& get() { return *shine::EngineContext::Get().GetSystem<PassTextureManager>(); }

        void RegisterTexture(const std::string& name, u32 textureId, int width, int height) override
        {
            m_Textures[name] = DebugTextureEntry{ textureId, width, height };
        }

        void ClearTextures()
        {
            m_Textures.clear();
        }

        void RefreshFromPipeline(const RenderPipeline* pipeline)
        {
            ClearTextures();
            if (!pipeline) return;
            for (const auto& pass : pipeline->GetPasses())
            {
                if (!pass) continue;
                pass->CollectDebugTextures(*this);
            }
        }

        u32 GetTextureId(std::string_view name) const
        {
            auto it = m_Textures.find(std::string(name));
            if (it == m_Textures.end()) return 0;
            return it->second.id;
        }

        DebugTextureEntry GetTextureEntry(std::string_view name) const
        {
            auto it = m_Textures.find(std::string(name));
            if (it == m_Textures.end()) return {};
            return it->second;
        }

        std::vector<std::string> GetNames() const
        {
            std::vector<std::string> names;
            names.reserve(m_Textures.size());
            for (const auto& [k, _] : m_Textures) names.push_back(k);
            return names;
        }

    private:
        std::unordered_map<std::string, DebugTextureEntry> m_Textures;
    };
}
