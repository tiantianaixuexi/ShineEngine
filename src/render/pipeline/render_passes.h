#pragma once

#include "render_pass.h"
#include "command_buffer.h"
#include "render_data.h"
#include "gameplay/object.h"
#include "gameplay/component/component.h"
#include "render/material.h"
#include "render/renderer_service.h"
#include "render/resources/TextureManager.h"
#include "render/debug/debug_texture_sink.h"
#include "EngineCore/engine_context.h"
#include "scriptable_render_context.h"
#include "util/timer/FunctionTimer.h"
#include <algorithm>
#include <vector>
#include <string>
#include <array>

namespace shine::render
{
    class OpaquePass : public RenderPass
    {
    public:
        OpaquePass() : RenderPass("OpaquePass", RenderPassEvent::AfterShadowMap) 
        {
            AddOutput("SceneColor");
            AddOutput("Emissive");
        }

        s32 m_MRTHandle = 0;
        TextureHandle m_ColorTex;
        TextureHandle m_EmissiveTex;
        int m_Width = 0;
        int m_Height = 0;

        void Configure(RenderPipeline* pipeline, RenderingData& data) override
        {
            if (data.viewport.width <= 0 || data.viewport.height <= 0) return;

            if (m_Width != data.viewport.width || m_Height != data.viewport.height || m_MRTHandle == 0)
            {
                m_Width = data.viewport.width;
                m_Height = data.viewport.height;

                auto* service = shine::EngineContext::Get().GetSystem<RendererService>();
                auto* backend = service->GetBackend();
                
                if (m_MRTHandle) backend->DeleteCustomFramebuffer(m_MRTHandle);
                if (m_ColorTex.isValid()) TextureManager::get().ReleaseTexture(m_ColorTex);
                if (m_EmissiveTex.isValid()) TextureManager::get().ReleaseTexture(m_EmissiveTex);

                TextureCreateInfo info;
                info.width = m_Width;
                info.height = m_Height;
                info.generateMipmaps = false;
                info.linearFilter = true;
                
                m_ColorTex = TextureManager::get().CreateTexture(info);
                m_EmissiveTex = TextureManager::get().CreateTexture(info);
                
                // Note: Passing 0 for depth attachment. Depth Test depends on backend behavior or existing depth buffer reuse.
                std::vector<uint32_t> attachments = {
                    TextureManager::get().GetTextureId(m_ColorTex),
                    TextureManager::get().GetTextureId(m_EmissiveTex)
                };
                m_MRTHandle = backend->CreateCustomFramebuffer(m_Width, m_Height, attachments, 0);
            }
        }

        void Execute(ScriptableRenderContext& context, RenderingData& data) override
        {
            CommandBuffer cmd;
            if (m_MRTHandle)
            {
                auto* service = shine::EngineContext::Get().GetSystem<RendererService>();
                auto* backend = service->GetBackend();
                u32 fbo = backend->GetViewportFBO(m_MRTHandle);
                if (fbo == 0) return;
                cmd.BindFramebuffer(static_cast<uint64_t>(fbo));
                cmd.SetViewport(0, 0, m_Width, m_Height);
                cmd.SetClearColor(0.02f, 0.02f, 0.02f, 1.0f);
                cmd.ClearRenderTarget(true, true); 
                cmd.EnableDepthTest(true);
                
                for (auto* obj : data.sceneObjects)
                {
                    if (!obj) continue;
                    for (auto& compPtr : obj->getComponents())
                    {
                        if (compPtr) compPtr->onRender(cmd);
                    }
                }
            }
            context.Submit(std::move(cmd));
        }

        void CollectDebugTextures(DebugTextureSink& sink) override
        {
            if (m_ColorTex.isValid())
            {
                sink.RegisterTexture("SceneColor", TextureManager::get().GetTextureId(m_ColorTex), m_Width, m_Height);
            }
            if (m_EmissiveTex.isValid())
            {
                sink.RegisterTexture("Emissive", TextureManager::get().GetTextureId(m_EmissiveTex), m_Width, m_Height);
            }
        }
    };

    class BloomPass : public RenderPass
    {
    public:
        BloomPass() : RenderPass("BloomPass", RenderPassEvent::AfterOpaque) 
        {
            AddInput("Emissive");
            AddOutput("SceneColor");
        }

        OpaquePass* m_OpaquePass = nullptr;
        void SetOpaquePass(OpaquePass* pass) { m_OpaquePass = pass; }
        bool IsPostProcessPass() const override { return true; }

        float m_Threshold = 0.5f;
        float m_Intensity = 1.2f;
        int m_BlurRadius = 3; 
        int m_ActiveLevels = 5;
        float m_BloomRadius = 0.15f;
        float m_SoftKnee = 0.5f;

        static constexpr int kBloomLevels = 5;
        std::array<s32, kBloomLevels> m_PingHandle{};
        std::array<s32, kBloomLevels> m_PongHandle{};
        std::array<TextureHandle, kBloomLevels> m_PingTex{};
        std::array<TextureHandle, kBloomLevels> m_PongTex{};
        std::array<int, kBloomLevels> m_Width{};
        std::array<int, kBloomLevels> m_Height{};
        int m_BaseWidth = 0;
        int m_BaseHeight = 0;
        
        uint32_t m_ProgBlurX = 0;
        uint32_t m_ProgBlurY = 0;
        uint32_t m_ProgCombine = 0;
        uint32_t m_ProgFXAA = 0;
        uint32_t m_ProgHighPass = 0;
        uint32_t m_DummyVAO = 0;
        s32 m_LocHighPassImage = -1;
        s32 m_LocHighPassThreshold = -1;
        s32 m_LocHighPassSoftKnee = -1;
        s32 m_LocBlurXImage = -1;
        s32 m_LocBlurYImage = -1;
        s32 m_LocCombineScene = -1;
        s32 m_LocCombineBloom0 = -1;
        s32 m_LocCombineBloom1 = -1;
        s32 m_LocCombineBloom2 = -1;
        s32 m_LocCombineBloom3 = -1;
        s32 m_LocCombineBloom4 = -1;
        s32 m_LocCombineWeights = -1;
        s32 m_LocCombineWeight4 = -1;
        s32 m_LocCombineIntensity = -1;
        s32 m_LocCombineExposure = -1;
        s32 m_LocCombineRadius = -1;
        s32 m_LocFxaaScene = -1;
        s32 m_LocFxaaInvTexSize = -1;
        float m_Exposure = 1.0f;
        std::array<float, kBloomLevels> m_BloomWeights{ 1.0f, 0.8f, 0.6f, 0.4f, 0.3f };
        bool m_EnableFXAA = true;
        s32 m_FinalHandle = 0;
        TextureHandle m_FinalTex;
        int m_FinalWidth = 0;
        int m_FinalHeight = 0;
        s32 m_BrightHandle = 0;
        TextureHandle m_BrightTex;
        int m_BrightWidth = 0;
        int m_BrightHeight = 0;

        void Configure(RenderPipeline* pipeline, RenderingData& data) override
        {
             int w = data.viewport.width / 2;
             int h = data.viewport.height / 2;
             if(w <= 0 || h <= 0) return;
             
             auto* service = shine::EngineContext::Get().GetSystem<RendererService>();
             auto* backend = service->GetBackend();

             if (m_DummyVAO == 0)
             {
                 m_DummyVAO = backend->CreateVertexArray();
             }

             if(m_BaseWidth != w || m_BaseHeight != h || m_PingHandle[0] == 0)
             {
                 m_BaseWidth = w;
                 m_BaseHeight = h;
                 
                 for (int i = 0; i < kBloomLevels; ++i)
                 {
                     if (m_PingHandle[i]) backend->DeleteCustomFramebuffer(m_PingHandle[i]);
                     if (m_PongHandle[i]) backend->DeleteCustomFramebuffer(m_PongHandle[i]);
                     if (m_PingTex[i].isValid()) TextureManager::get().ReleaseTexture(m_PingTex[i]);
                     if (m_PongTex[i].isValid()) TextureManager::get().ReleaseTexture(m_PongTex[i]);
                     m_PingHandle[i] = 0;
                     m_PongHandle[i] = 0;
                 }

                 for (int i = 0; i < kBloomLevels; ++i)
                 {
                     int levelW = m_BaseWidth >> i;
                     int levelH = m_BaseHeight >> i;
                     if (levelW <= 0) levelW = 1;
                     if (levelH <= 0) levelH = 1;
                     m_Width[i] = levelW;
                     m_Height[i] = levelH;

                     TextureCreateInfo info;
                     info.width = levelW;
                     info.height = levelH;
                     info.linearFilter = true;
                     info.clampToEdge = true;

                     m_PingTex[i] = TextureManager::get().CreateTexture(info);
                     m_PongTex[i] = TextureManager::get().CreateTexture(info);

                     m_PingHandle[i] = backend->CreateCustomFramebuffer(levelW, levelH, {TextureManager::get().GetTextureId(m_PingTex[i])}, 0);
                     m_PongHandle[i] = backend->CreateCustomFramebuffer(levelW, levelH, {TextureManager::get().GetTextureId(m_PongTex[i])}, 0);
                 }
             }

             if (m_FinalWidth != data.viewport.width || m_FinalHeight != data.viewport.height || m_FinalHandle == 0)
             {
                 m_FinalWidth = data.viewport.width;
                 m_FinalHeight = data.viewport.height;
                 if (m_FinalHandle) backend->DeleteCustomFramebuffer(m_FinalHandle);
                 if (m_FinalTex.isValid()) TextureManager::get().ReleaseTexture(m_FinalTex);

                 TextureCreateInfo info;
                 info.width = m_FinalWidth;
                 info.height = m_FinalHeight;
                 info.linearFilter = true;
                 info.clampToEdge = true;
                 m_FinalTex = TextureManager::get().CreateTexture(info);
                 m_FinalHandle = backend->CreateCustomFramebuffer(m_FinalWidth, m_FinalHeight, {TextureManager::get().GetTextureId(m_FinalTex)}, 0);
             }

            if (m_BrightWidth != m_BaseWidth || m_BrightHeight != m_BaseHeight || m_BrightHandle == 0)
            {
                m_BrightWidth = m_BaseWidth;
                m_BrightHeight = m_BaseHeight;
                if (m_BrightHandle) backend->DeleteCustomFramebuffer(m_BrightHandle);
                if (m_BrightTex.isValid()) TextureManager::get().ReleaseTexture(m_BrightTex);

                TextureCreateInfo brightInfo;
                brightInfo.width = m_BrightWidth;
                brightInfo.height = m_BrightHeight;
                brightInfo.linearFilter = true;
                brightInfo.clampToEdge = true;
                m_BrightTex = TextureManager::get().CreateTexture(brightInfo);
                m_BrightHandle = backend->CreateCustomFramebuffer(m_BrightWidth, m_BrightHeight, {TextureManager::get().GetTextureId(m_BrightTex)}, 0);
            }

            if(m_ProgBlurX == 0) CompileShaders(backend);
        }
        
        void CompileShaders(backend::IRenderBackend* backend)
        {
            const char* vs = R"(
                #version 330 core
                out vec2 TexCoords;
                void main() {
                    vec2 pos[3] = vec2[](vec2(-1,-1), vec2(3,-1), vec2(-1,3));
                    gl_Position = vec4(pos[gl_VertexID], 0.0, 1.0);
                    TexCoords = pos[gl_VertexID] * 0.5 + 0.5;
                }
            )";

            const char* fs_blurX = R"(
                #version 330 core
                in vec2 TexCoords;
                out vec4 FragColor;
                uniform sampler2D image;
                uniform float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);
                void main() {
                    vec2 tex_offset = 1.0 / textureSize(image, 0); 
                    vec3 result = texture(image, TexCoords).rgb * weight[0]; 
                    for(int i = 1; i < 5; ++i) {
                        result += texture(image, TexCoords + vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
                        result += texture(image, TexCoords - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
                    }
                    FragColor = vec4(result, 1.0);
                }
            )";

            const char* fs_blurY = R"(
                #version 330 core
                in vec2 TexCoords;
                out vec4 FragColor;
                uniform sampler2D image;
                uniform float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);
                void main() {
                    vec2 tex_offset = 1.0 / textureSize(image, 0); 
                    vec3 result = texture(image, TexCoords).rgb * weight[0]; 
                    for(int i = 1; i < 5; ++i) {
                        result += texture(image, TexCoords + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
                        result += texture(image, TexCoords - vec2(0.0, tex_offset.y * i)).rgb * weight[i];
                    }
                    FragColor = vec4(result, 1.0);
                }
            )";

            const char* fs_highpass = R"(
                #version 330 core
                in vec2 TexCoords;
                out vec4 FragColor;
                uniform sampler2D image;
                uniform float threshold;
                uniform float softKnee;
                void main() {
                    vec3 c = texture(image, TexCoords).rgb;
                    float brightness = max(max(c.r, c.g), c.b);
                    float knee = threshold * softKnee + 1e-4;
                    float soft = clamp((brightness - threshold + knee) / (2.0 * knee), 0.0, 1.0);
                    float contrib = max(brightness - threshold, 0.0) + soft * soft * knee;
                    vec3 result = c * (contrib / max(brightness, 1e-4));
                    FragColor = vec4(result, 1.0);
                }
            )";

            const char* fs_combine = R"(
                #version 330 core
                in vec2 TexCoords;
                out vec4 FragColor;
                uniform sampler2D scene;
                uniform sampler2D bloom0;
                uniform sampler2D bloom1;
                uniform sampler2D bloom2;
                uniform sampler2D bloom3;
                uniform sampler2D bloom4;
                uniform vec4 bloomWeights;
                uniform float bloomWeight4;
                uniform float intensity;
                uniform float exposure;
                uniform float bloomRadius;
                vec3 ACESFilm(vec3 x)
                {
                    float a = 2.51;
                    float b = 0.03;
                    float c = 2.43;
                    float d = 0.59;
                    float e = 0.14;
                    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
                }
                float lerpBloomFactor(float factor)
                {
                    float mirrorFactor = 1.2 - factor;
                    return mix(factor, mirrorFactor, bloomRadius);
                }
                void main() {
                    vec3 hdrColor = texture(scene, TexCoords).rgb;
                    vec3 bloomColor = texture(bloom0, TexCoords).rgb * lerpBloomFactor(bloomWeights.x);
                    bloomColor += texture(bloom1, TexCoords).rgb * lerpBloomFactor(bloomWeights.y);
                    bloomColor += texture(bloom2, TexCoords).rgb * lerpBloomFactor(bloomWeights.z);
                    bloomColor += texture(bloom3, TexCoords).rgb * lerpBloomFactor(bloomWeights.w);
                    bloomColor += texture(bloom4, TexCoords).rgb * lerpBloomFactor(bloomWeight4);
                    bloomColor *= intensity;
                    vec3 color = hdrColor + bloomColor;
                    color *= exposure;
                    color = ACESFilm(color);
                    color = pow(color, vec3(1.0/2.2));
                    FragColor = vec4(color, 1.0);
                }
            )";

            const char* fs_fxaa = R"(
                #version 330 core
                in vec2 TexCoords;
                out vec4 FragColor;
                uniform sampler2D scene;
                uniform vec2 invTexSize;
                void main()
                {
                    vec3 rgbNW = texture(scene, TexCoords + vec2(-1.0, -1.0) * invTexSize).rgb;
                    vec3 rgbNE = texture(scene, TexCoords + vec2(1.0, -1.0) * invTexSize).rgb;
                    vec3 rgbSW = texture(scene, TexCoords + vec2(-1.0, 1.0) * invTexSize).rgb;
                    vec3 rgbSE = texture(scene, TexCoords + vec2(1.0, 1.0) * invTexSize).rgb;
                    vec3 rgbM  = texture(scene, TexCoords).rgb;
                    vec3 luma = vec3(0.299, 0.587, 0.114);
                    float lumaNW = dot(rgbNW, luma);
                    float lumaNE = dot(rgbNE, luma);
                    float lumaSW = dot(rgbSW, luma);
                    float lumaSE = dot(rgbSE, luma);
                    float lumaM  = dot(rgbM,  luma);
                    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
                    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
                    vec2 dir;
                    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
                    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));
                    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * 0.25 * 0.5, 1.0/128.0);
                    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
                    dir = clamp(dir * rcpDirMin, vec2(-8.0), vec2(8.0)) * invTexSize;
                    vec3 rgbA = 0.5 * (
                        texture(scene, TexCoords + dir * (1.0/3.0 - 0.5)).rgb +
                        texture(scene, TexCoords + dir * (2.0/3.0 - 0.5)).rgb);
                    vec3 rgbB = rgbA * 0.5 + 0.25 * (
                        texture(scene, TexCoords + dir * -0.5).rgb +
                        texture(scene, TexCoords + dir * 0.5).rgb);
                    float lumaB = dot(rgbB, luma);
                    if (lumaB < lumaMin || lumaB > lumaMax)
                        FragColor = vec4(rgbA, 1.0);
                    else
                        FragColor = vec4(rgbB, 1.0);
                }
            )";

            auto compile = [&](const char* f) { return backend->CreateShaderProgram(vs, f).value_or(0); };
            m_ProgBlurX = compile(fs_blurX);
            m_ProgBlurY = compile(fs_blurY);
            m_ProgHighPass = compile(fs_highpass);
            m_ProgCombine = compile(fs_combine);
            m_ProgFXAA = compile(fs_fxaa);
            CacheUniformLocations(backend);
        }

        void CacheUniformLocations(backend::IRenderBackend* backend)
        {
            m_LocHighPassImage = backend->GetUniformLocation(m_ProgHighPass, "image");
            m_LocHighPassThreshold = backend->GetUniformLocation(m_ProgHighPass, "threshold");
            m_LocHighPassSoftKnee = backend->GetUniformLocation(m_ProgHighPass, "softKnee");

            m_LocBlurXImage = backend->GetUniformLocation(m_ProgBlurX, "image");
            m_LocBlurYImage = backend->GetUniformLocation(m_ProgBlurY, "image");

            m_LocCombineScene = backend->GetUniformLocation(m_ProgCombine, "scene");
            m_LocCombineBloom0 = backend->GetUniformLocation(m_ProgCombine, "bloom0");
            m_LocCombineBloom1 = backend->GetUniformLocation(m_ProgCombine, "bloom1");
            m_LocCombineBloom2 = backend->GetUniformLocation(m_ProgCombine, "bloom2");
            m_LocCombineBloom3 = backend->GetUniformLocation(m_ProgCombine, "bloom3");
            m_LocCombineBloom4 = backend->GetUniformLocation(m_ProgCombine, "bloom4");
            m_LocCombineWeights = backend->GetUniformLocation(m_ProgCombine, "bloomWeights");
            m_LocCombineWeight4 = backend->GetUniformLocation(m_ProgCombine, "bloomWeight4");
            m_LocCombineIntensity = backend->GetUniformLocation(m_ProgCombine, "intensity");
            m_LocCombineExposure = backend->GetUniformLocation(m_ProgCombine, "exposure");
            m_LocCombineRadius = backend->GetUniformLocation(m_ProgCombine, "bloomRadius");

            m_LocFxaaScene = backend->GetUniformLocation(m_ProgFXAA, "scene");
            m_LocFxaaInvTexSize = backend->GetUniformLocation(m_ProgFXAA, "invTexSize");
        }

        void Execute(ScriptableRenderContext& context, RenderingData& data) override
        {
            if(!m_OpaquePass || !m_OpaquePass->m_EmissiveTex.isValid()) return;

            //shine::util::FunctionTimer totalTimer("BloomPass::Execute");
            auto* service = shine::EngineContext::Get().GetSystem<RendererService>();
            auto* backend = service->GetBackend();

            CommandBuffer cmd;
            cmd.EnableDepthTest(false);
            cmd.BindVertexArray(static_cast<uint64_t>(m_DummyVAO));

            int levels = m_ActiveLevels;
            if (levels < 1) levels = 1;
            if (levels > kBloomLevels) levels = kBloomLevels;
            std::array<float, kBloomLevels> weights = m_BloomWeights;
            for (int i = levels; i < kBloomLevels; ++i) weights[i] = 0.0f;
            std::array<u32, kBloomLevels> pingFBOs{};
            std::array<u32, kBloomLevels> pongFBOs{};
            for (int i = 0; i < levels; ++i)
            {
                pingFBOs[i] = backend->GetViewportFBO(m_PingHandle[i]);
                pongFBOs[i] = backend->GetViewportFBO(m_PongHandle[i]);
                if (pingFBOs[i] == 0 || pongFBOs[i] == 0) return;
            }

            {
                u32 brightFBO = backend->GetViewportFBO(m_BrightHandle);
                if (brightFBO == 0) return;
                cmd.BindFramebuffer(static_cast<uint64_t>(brightFBO));
                cmd.SetViewport(0, 0, m_BrightWidth, m_BrightHeight);
                cmd.UseProgram(static_cast<uint64_t>(m_ProgHighPass));
                cmd.SetUniform1i(m_LocHighPassImage, 0);
                cmd.SetUniform1f(m_LocHighPassThreshold, m_Threshold);
                cmd.SetUniform1f(m_LocHighPassSoftKnee, m_SoftKnee);
                cmd.BindTexture(0, TextureManager::get().GetTextureId(m_OpaquePass->m_EmissiveTex));
                cmd.DrawTriangles(0, 3);
            }

            {
                const int iterations = (std::max)(1, m_BlurRadius);
                u32 sourceTexId = TextureManager::get().GetTextureId(m_BrightTex);

                for (int iter = 0; iter < iterations; ++iter)
                {
                    cmd.BindFramebuffer(static_cast<uint64_t>(pingFBOs[0]));
                    cmd.SetViewport(0, 0, m_Width[0], m_Height[0]);
                    cmd.UseProgram(static_cast<uint64_t>(m_ProgBlurX));
                    cmd.SetUniform1i(m_LocBlurXImage, 0);
                    cmd.BindTexture(0, sourceTexId);
                    cmd.DrawTriangles(0, 3);

                    cmd.BindFramebuffer(static_cast<uint64_t>(pongFBOs[0]));
                    cmd.UseProgram(static_cast<uint64_t>(m_ProgBlurY));
                    cmd.SetUniform1i(m_LocBlurYImage, 0);
                    cmd.BindTexture(0, TextureManager::get().GetTextureId(m_PingTex[0]));
                    cmd.DrawTriangles(0, 3);
                    sourceTexId = TextureManager::get().GetTextureId(m_PongTex[0]);
                }

                for (int i = 1; i < levels; ++i)
                {
                    sourceTexId = TextureManager::get().GetTextureId(m_PongTex[i - 1]);
                    for (int iter = 0; iter < iterations; ++iter)
                    {
                        cmd.BindFramebuffer(static_cast<uint64_t>(pingFBOs[i]));
                        cmd.SetViewport(0, 0, m_Width[i], m_Height[i]);
                        cmd.UseProgram(static_cast<uint64_t>(m_ProgBlurX));
                        cmd.SetUniform1i(m_LocBlurXImage, 0);
                        cmd.BindTexture(0, sourceTexId);
                        cmd.DrawTriangles(0, 3);

                        cmd.BindFramebuffer(static_cast<uint64_t>(pongFBOs[i]));
                        cmd.UseProgram(static_cast<uint64_t>(m_ProgBlurY));
                        cmd.SetUniform1i(m_LocBlurYImage, 0);
                        cmd.BindTexture(0, TextureManager::get().GetTextureId(m_PingTex[i]));
                        cmd.DrawTriangles(0, 3);
                        sourceTexId = TextureManager::get().GetTextureId(m_PongTex[i]);
                    }
                }
            }

            {
               // timer("BloomPass::Combine");
                u32 combineFBO = m_EnableFXAA ? backend->GetViewportFBO(m_FinalHandle) : backend->GetViewportFBO(data.viewport.handle);
                if (combineFBO == 0) return;
                cmd.BindFramebuffer(static_cast<uint64_t>(combineFBO));
                cmd.SetViewport(0, 0, data.viewport.width, data.viewport.height);
                cmd.UseProgram(static_cast<uint64_t>(m_ProgCombine));
                cmd.SetUniform1i(m_LocCombineScene, 0);
                cmd.SetUniform1i(m_LocCombineBloom0, 1);
                cmd.SetUniform1i(m_LocCombineBloom1, 2);
                cmd.SetUniform1i(m_LocCombineBloom2, 3);
                cmd.SetUniform1i(m_LocCombineBloom3, 4);
                cmd.SetUniform1i(m_LocCombineBloom4, 5);
                cmd.SetUniform4f(m_LocCombineWeights, weights[0], weights[1], weights[2], weights[3]);
                cmd.SetUniform1f(m_LocCombineWeight4, weights[4]);
                cmd.SetUniform1f(m_LocCombineIntensity, m_Intensity);
                cmd.SetUniform1f(m_LocCombineExposure, m_Exposure);
                cmd.SetUniform1f(m_LocCombineRadius, m_BloomRadius);
                cmd.BindTexture(0, TextureManager::get().GetTextureId(m_OpaquePass->m_ColorTex));
                cmd.BindTexture(1, TextureManager::get().GetTextureId(m_PongTex[0]));
                cmd.BindTexture(2, TextureManager::get().GetTextureId(m_PongTex[1]));
                cmd.BindTexture(3, TextureManager::get().GetTextureId(m_PongTex[2]));
                cmd.BindTexture(4, TextureManager::get().GetTextureId(m_PongTex[3]));
                cmd.BindTexture(5, TextureManager::get().GetTextureId(m_PongTex[4]));
                cmd.DrawTriangles(0, 3);
            }

            if (m_EnableFXAA)
            {
                //shine::util::FunctionTimer timer("BloomPass::FXAA");
                u32 targetFBO = backend->GetViewportFBO(data.viewport.handle);
                if (targetFBO == 0) return;
                cmd.BindFramebuffer(static_cast<uint64_t>(targetFBO));
                cmd.SetViewport(0, 0, data.viewport.width, data.viewport.height);
                cmd.UseProgram(static_cast<uint64_t>(m_ProgFXAA));
                cmd.SetUniform1i(m_LocFxaaScene, 0);
                cmd.SetUniform2f(m_LocFxaaInvTexSize, 1.0f / static_cast<float>(m_FinalWidth), 1.0f / static_cast<float>(m_FinalHeight));
                cmd.BindTexture(0, TextureManager::get().GetTextureId(m_FinalTex));
                cmd.DrawTriangles(0, 3);
            }

            context.Submit(std::move(cmd));
        }

        void CollectDebugTextures(DebugTextureSink& sink) override
        {
            int levels = m_ActiveLevels;
            if (levels < 1) levels = 1;
            if (levels > kBloomLevels) levels = kBloomLevels;
            if (m_BrightTex.isValid())
            {
                sink.RegisterTexture("BloomBright", TextureManager::get().GetTextureId(m_BrightTex), m_BrightWidth, m_BrightHeight);
            }
            for (int i = 0; i < levels; ++i)
            {
                if (m_PongTex[i].isValid())
                {
                    sink.RegisterTexture("BloomL" + std::to_string(i), TextureManager::get().GetTextureId(m_PongTex[i]), m_Width[i], m_Height[i]);
                }
            }
            if (m_EnableFXAA && m_FinalTex.isValid())
            {
                sink.RegisterTexture("BloomFinal", TextureManager::get().GetTextureId(m_FinalTex), m_FinalWidth, m_FinalHeight);
            }
        }
    };
}
