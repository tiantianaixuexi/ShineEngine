#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <array>

#include "render/resources/shader_manager.h"
#include "render/pipeline/command_buffer.h"


namespace shine::render
{
    class Material
    {
    public:
        Material() = default;
        ~Material() = default;

        // ---- 工厂方法 ----

        static std::shared_ptr<Material> CreateFancyRimToon()
        {
            auto m = std::make_shared<Material>();
            m->m_ShaderKey = "FancyRimToon";
            m->m_VS = R"(
            #version 330 core
            layout(location = 0) in vec3 aPos;
            layout(location = 1) in vec3 aNormal;
            out vec3 vNormal;
            out vec3 vWorldPos;
            uniform CameraUBO {
                mat4 u_VP;
                vec4 u_ViewPos;
            };
            void main(){
                vNormal = aNormal;
                vWorldPos = aPos;
                gl_Position = u_VP * vec4(aPos, 1.0);
            }
            )";
            m->m_FS = R"(
            #version 330 core
            in vec3 vNormal;
            in vec3 vWorldPos;
            out vec4 color;
            uniform CameraUBO {
                mat4 u_VP;
                vec4 u_ViewPos;
            };
            uniform vec3 u_BaseColor;
            uniform vec3 u_Ambient;
            uniform vec3 u_LightDir;
            uniform float u_Shininess;
            void main(){
                vec3 N = normalize(vNormal);
                vec3 L = normalize(-u_LightDir);
                vec3 V = normalize(u_ViewPos.xyz - vWorldPos);
                vec3 H = normalize(L + V);
                float NdotL = max(dot(N, L), 0.0);
                float toon = floor(NdotL * 4.0) / 4.0;
                float fres = pow(1.0 - max(dot(N, V), 0.0), 3.0);
                vec3 rimColor = vec3(0.2, 0.6, 1.0);
                float spec = pow(max(dot(N, H), 0.0), u_Shininess);
                vec3 diffuse = u_BaseColor * toon;
                vec3 specular = vec3(0.25) * spec;
                vec3 ambient = u_Ambient * u_BaseColor;
                vec3 rim = rimColor * fres * 0.7;
                color = vec4(ambient + diffuse + specular + rim, 1.0);
            }
            )";
            m->setBaseColor(0.9f, 0.5f, 0.3f);
            m->setAmbient(0.08f, 0.08f, 0.1f);
            m->setLightDir(-0.3f, -0.7f, -0.6f);
            m->setShininess(48.0f);
            return m;
        }

        static std::shared_ptr<Material> GetFancyRimToon()
        {
            static std::shared_ptr<Material> s_fancy;
            if (!s_fancy) s_fancy = CreateFancyRimToon();
            return s_fancy;
        }

        static std::shared_ptr<Material> CreatePBR()
        {
            auto m = std::make_shared<Material>();
            m->m_ShaderKey = "PBR_GGX";
            m->m_VS = R"(
            #version 330 core
            layout(location = 0) in vec3 aPos;
            layout(location = 1) in vec3 aNormal;
            out vec3 vNormal;
            out vec3 vWorldPos;
            uniform CameraUBO { mat4 u_VP; vec4 u_ViewPos; };
            void main(){ vNormal = aNormal; vWorldPos = aPos; gl_Position = u_VP * vec4(aPos, 1.0); }
            )";
            m->m_FS = R"(
            #version 330 core
            in vec3 vNormal; in vec3 vWorldPos; out vec4 color;
            uniform CameraUBO { mat4 u_VP; vec4 u_ViewPos; };
            layout(std140) uniform LightUBO { vec4 u_Dir; vec4 u_LColor; vec4 u_Inten; };
            uniform vec3  u_BaseColor;
            uniform float u_Metallic;
            uniform float u_Roughness;
            uniform float u_Ao;
            uniform vec3  u_Ambient;

            const float PI = 3.14159265359;

            float DistributionGGX(vec3 N, vec3 H, float roughness)
            {
                float a = roughness*roughness; float a2 = a*a;
                float NdotH = max(dot(N,H),0.0); float NdotH2 = NdotH*NdotH;
                float denom = (NdotH2*(a2-1.0)+1.0);
                return a2/(PI*denom*denom+1e-5);
            }
            float GeometrySchlickGGX(float NdotV, float roughness)
            {
                float r = roughness+1.0; float k = (r*r)/8.0;
                return NdotV/(NdotV*(1.0-k)+k+1e-5);
            }
            float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
            {
                return GeometrySchlickGGX(max(dot(N,V),0.0),roughness)
                     * GeometrySchlickGGX(max(dot(N,L),0.0),roughness);
            }
            vec3 FresnelSchlick(float cosTheta, vec3 F0)
            {
                return F0+(1.0-F0)*pow(1.0-cosTheta,5.0);
            }

            void main()
            {
                vec3 N = normalize(vNormal);
                vec3 V = normalize(u_ViewPos.xyz - vWorldPos);
                vec3 L = normalize(-u_Dir.xyz);
                vec3 H = normalize(V + L);

                float metallic  = clamp(u_Metallic, 0.0, 1.0);
                float roughness = clamp(u_Roughness, 0.04, 1.0);
                float ao        = clamp(u_Ao, 0.0, 1.0);
                vec3  albedo    = clamp(u_BaseColor, 0.0, 1.0);

                vec3 F0 = mix(vec3(0.04), albedo, metallic);
                float NDF = DistributionGGX(N, H, roughness);
                float G   = GeometrySmith(N, V, L, roughness);
                vec3  F   = FresnelSchlick(max(dot(H,V),0.0), F0);

                vec3 kS = F;
                vec3 kD = (vec3(1.0)-kS)*(1.0-metallic);

                float NdotL = max(dot(N,L),0.0);
                float NdotV = max(dot(N,V),0.0);
                vec3  numerator = NDF*G*F;
                float denom = max(4.0*NdotV*NdotL, 1e-4);
                vec3  specular = numerator/denom;

                vec3 radiance = u_LColor.rgb * u_Inten.x;
                vec3 Lo = (kD*albedo/PI + specular)*radiance*NdotL;
                vec3 ambient = u_Ambient*albedo*ao;
                color = vec4(ambient + Lo, 1.0);
            }
            )";

            m->setBaseColor(0.95f, 0.3f, 0.3f);
            m->m_Metallic  = 0.0f;
            m->m_Roughness = 0.5f;
            m->m_Ao        = 1.0f;
            m->setLightDir(-0.3f, -0.7f, -0.6f);
            m->setAmbient(0.04f, 0.04f, 0.04f);
            return m;
        }

        static std::shared_ptr<Material> GetPBR()
        {
            static std::shared_ptr<Material> s_pbr;
            if (!s_pbr) s_pbr = CreatePBR();
            return s_pbr;
        }

        // ---- 绑定材质（记录到 CommandBuffer）----
        void bind(CommandBuffer& cmdBuffer)
        {
            ensureCompiled();
            if (m_Program == 0) return;

            cmdBuffer.UseProgram(static_cast<uint64_t>(m_Program));

            if (m_LocationBaseColor >= 0) cmdBuffer.SetUniform3f(m_LocationBaseColor, m_BaseColor[0], m_BaseColor[1], m_BaseColor[2]);
            if (m_LocationAmbient   >= 0) cmdBuffer.SetUniform3f(m_LocationAmbient,   m_Ambient[0],   m_Ambient[1],   m_Ambient[2]);
            if (m_LocationShininess >= 0) cmdBuffer.SetUniform1f(m_LocationShininess, m_Shininess);
            if (m_LocationMetallic  >= 0) cmdBuffer.SetUniform1f(m_LocationMetallic,  m_Metallic);
            if (m_LocationRoughness >= 0) cmdBuffer.SetUniform1f(m_LocationRoughness, m_Roughness);
            if (m_LocationAo        >= 0) cmdBuffer.SetUniform1f(m_LocationAo,        m_Ao);
            if (m_LocationLightDir  >= 0) cmdBuffer.SetUniform3f(m_LocationLightDir,  m_LightDir[0], m_LightDir[1], m_LightDir[2]);
        }

        // ---- 参数设置 ----
        void setBaseColor(float r, float g, float b) { m_BaseColor = {r,g,b}; }
        void setAmbient(float r, float g, float b)   { m_Ambient = {r,g,b}; }
        void setLightDir(float x, float y, float z)  { m_LightDir = {x,y,z}; }
        void setShininess(float s)                   { m_Shininess = s; }
        void setMetallic(float m)                    { m_Metallic  = m; }
        void setRoughness(float r)                   { m_Roughness = r; }
        void setAo(float a)                          { m_Ao = a; }

        // ---- 读取参数（用于 ImGui 实时调节）----
        [[nodiscard]] std::array<float,3> getBaseColor() const { return m_BaseColor; }
        [[nodiscard]] std::array<float,3> getAmbient()   const { return m_Ambient; }
        [[nodiscard]] std::array<float,3> getLightDir()   const { return m_LightDir; }
        [[nodiscard]] float getShininess() const { return m_Shininess; }
        [[nodiscard]] float getMetallic()  const { return m_Metallic; }
        [[nodiscard]] float getRoughness() const { return m_Roughness; }
        [[nodiscard]] float getAo()        const { return m_Ao; }

        // 将内置 Shader 入队显示编译进度
        static void EnqueueBuiltinsForProgress()
        {
            const char* vsPhong = R"(
            #version 330 core
            layout(location = 0) in vec3 aPos; layout(location = 1) in vec3 aNormal;
            out vec3 vNormal; out vec3 vWorldPos; uniform CameraUBO { mat4 u_VP; vec4 u_ViewPos; };
            void main(){ vNormal=aNormal; vWorldPos=aPos; gl_Position = u_VP * vec4(aPos,1.0);} )";
            const char* fsPhong = R"(
            #version 330 core
            in vec3 vNormal; in vec3 vWorldPos; out vec4 color;
            uniform CameraUBO { mat4 u_VP; vec4 u_ViewPos; };
            layout(std140) uniform LightUBO { vec4 u_Dir; vec4 u_LColor; vec4 u_Inten; };
            uniform vec3 u_BaseColor; uniform vec3 u_Ambient; uniform float u_Shininess;
            void main(){ vec3 N=normalize(vNormal); vec3 L=normalize(-u_Dir.xyz); vec3 V=normalize(u_ViewPos.xyz-vWorldPos); vec3 H=normalize(L+V);
                float NdotL=max(dot(N,L),0.0)*u_Inten.x; float spec=pow(max(dot(N,H),0.0),u_Shininess);
                vec3 diffuse=(u_BaseColor*u_LColor.rgb)*NdotL; vec3 specular=(u_LColor.rgb)*(0.25*spec)*u_Inten.x; vec3 ambient=u_Ambient*u_BaseColor; color=vec4(ambient+diffuse+specular,1.0);} )";

            const char* vsFancy = R"(
            #version 330 core
            layout(location = 0) in vec3 aPos; layout(location = 1) in vec3 aNormal;
            out vec3 vNormal; out vec3 vWorldPos; uniform CameraUBO { mat4 u_VP; vec4 u_ViewPos; };
            void main(){ vNormal=aNormal; vWorldPos=aPos; gl_Position = u_VP * vec4(aPos,1.0);} )";
            const char* fsFancy = R"(
            #version 330 core
            in vec3 vNormal; in vec3 vWorldPos; out vec4 color;
            uniform CameraUBO { mat4 u_VP; vec4 u_ViewPos; };
            layout(std140) uniform LightUBO { vec4 u_Dir; vec4 u_LColor; vec4 u_Inten; };
            uniform vec3 u_BaseColor; uniform vec3 u_Ambient; uniform float u_Shininess;
            void main(){ vec3 N=normalize(vNormal); vec3 L=normalize(-u_Dir.xyz); vec3 V=normalize(u_ViewPos.xyz - vWorldPos); vec3 H=normalize(L+V);
                float NdotL=max(dot(N,L),0.0); float toon=floor((NdotL*u_Inten.x)*4.0)/4.0; float fres=pow(1.0-max(dot(N,V),0.0),3.0);
                float spec=pow(max(dot(N,H),0.0),u_Shininess); vec3 diffuse=(u_BaseColor*u_LColor.rgb)*toon; vec3 specular=(u_LColor.rgb)*(0.25*spec)*u_Inten.x; vec3 ambient=u_Ambient*u_BaseColor; vec3 rim=vec3(0.2,0.6,1.0)*fres*0.7; color=vec4(ambient+diffuse+specular+rim,1.0);} )";

            shine::render::ShaderManager::get().enqueue("DefaultPhong", vsPhong, fsPhong);
            shine::render::ShaderManager::get().enqueue("FancyRimToon", vsFancy, fsFancy);

            const char* vsPBR = R"(
            #version 330 core
            layout(location = 0) in vec3 aPos; layout(location = 1) in vec3 aNormal;
            out vec3 vNormal; out vec3 vWorldPos; uniform CameraUBO { mat4 u_VP; vec4 u_ViewPos; };
            void main(){ vNormal=aNormal; vWorldPos=aPos; gl_Position = u_VP * vec4(aPos,1.0);} )";
            const char* fsPBR = R"(
            #version 330 core
            in vec3 vNormal; in vec3 vWorldPos; out vec4 color;
            uniform CameraUBO { mat4 u_VP; vec4 u_ViewPos; };
            layout(std140) uniform LightUBO { vec4 u_Dir; vec4 u_LColor; vec4 u_Inten; };
            uniform vec3 u_BaseColor; uniform float u_Metallic; uniform float u_Roughness; uniform float u_Ao; uniform vec3 u_Ambient;
            const float PI = 3.14159265359;
            float DistributionGGX(vec3 N, vec3 H, float r){ float a=r*r; float a2=a*a; float NdotH=max(dot(N,H),0.0); float NdotH2=NdotH*NdotH; float d=(NdotH2*(a2-1.0)+1.0); return a2/(PI*d*d+1e-5);}
            float GeometrySchlickGGX(float NdotV, float r){ float k=pow(r+1.0,2.0)/8.0; return NdotV/(NdotV*(1.0-k)+k+1e-5);}
            float GeometrySmith(vec3 N, vec3 V, vec3 L, float r){ return GeometrySchlickGGX(max(dot(N,V),0.0),r)*GeometrySchlickGGX(max(dot(N,L),0.0),r);}
            vec3 FresnelSchlick(float c, vec3 F0){ return F0 + (1.0-F0)*pow(1.0-c,5.0);}
            void main(){ vec3 N=normalize(vNormal); vec3 V=normalize(u_ViewPos.xyz-vWorldPos); vec3 L=normalize(-u_Dir.xyz); vec3 H=normalize(V+L);
                float m=clamp(u_Metallic,0.0,1.0); float r=clamp(u_Roughness,0.04,1.0); float ao=clamp(u_Ao,0.0,1.0); vec3 albedo=clamp(u_BaseColor,0.0,1.0);
                vec3 F0=mix(vec3(0.04), albedo, m);
                float NDF=DistributionGGX(N,H,r); float G=GeometrySmith(N,V,L,r); vec3 F=FresnelSchlick(max(dot(H,V),0.0),F0);
                vec3 kS=F; vec3 kD=(vec3(1.0)-kS)*(1.0-m); float NdotL=max(dot(N,L),0.0); float NdotV=max(dot(N,V),0.0);
                vec3 spec=(NDF*G*F)/max(4.0*NdotV*NdotL,1e-4);
                vec3 Lo=(kD*albedo/PI + spec) * (u_LColor.rgb*u_Inten.x) * NdotL; vec3 ambient=u_Ambient*albedo*ao; color=vec4(ambient+Lo,1.0);} )";
            shine::render::ShaderManager::get().enqueue("PBR_GGX", vsPBR, fsPBR);
        }

        static std::shared_ptr<Material> GetDefaultPhong()
        {
            static std::shared_ptr<Material> s_default;
            if (!s_default)
            {
                s_default = std::make_shared<Material>();
                s_default->setBaseColor(0.95f, 0.75f, 0.55f);
                s_default->setAmbient(0.15f, 0.15f, 0.18f);
                s_default->setLightDir(-0.3f, -0.7f, -0.6f);
                s_default->setShininess(32.0f);
            }
            return s_default;
        }

    private:
        // ---- 编译（通过 ShaderManager 抽象，不直接调用 GL）----
        void ensureCompiled()
        {
            if (m_Program != 0) return;

            std::string_view vs, fs;

            // 若未指定着色器，使用默认 Phong
            if (m_VS.empty() || m_FS.empty()) {
                m_ShaderKey = "DefaultPhong";
                static constexpr const char* kVS = R"(
            #version 330 core
            layout(location = 0) in vec3 aPos;
            layout(location = 1) in vec3 aNormal;
            out vec3 vNormal;
            out vec3 vWorldPos;
            uniform CameraUBO { mat4 u_VP; vec4 u_ViewPos; };
            uniform mat4 u_Model;
            void main(){
                vNormal = aNormal;
                vec4 worldPos = u_Model * vec4(aPos, 1.0);
                vWorldPos = worldPos.xyz;
                gl_Position = u_VP * worldPos;
            }
            )";
                static constexpr const char* kFS = R"(
            #version 330 core
            in vec3 vNormal;
            in vec3 vWorldPos;
            out vec4 color;
            uniform CameraUBO { mat4 u_VP; vec4 u_ViewPos; };
            uniform vec3 u_BaseColor;
            uniform vec3 u_Ambient;
            uniform vec3 u_LightDir;
            uniform float u_Shininess;
            void main(){
                vec3 N = normalize(vNormal);
                vec3 L = normalize(-u_LightDir);
                vec3 V = normalize(u_ViewPos.xyz - vWorldPos);
                vec3 H = normalize(L + V);
                float NdotL = max(dot(N, L), 0.0);
                float spec = pow(max(dot(N, H), 0.0), u_Shininess);
                vec3 diffuse = u_BaseColor * NdotL;
                vec3 specular = vec3(0.25) * spec;
                vec3 ambient = u_Ambient * u_BaseColor;
                color = vec4(ambient + diffuse + specular, 1.0);
            }
            )";
                vs = kVS;
                fs = kFS;
            } else {
                vs = m_VS;
                fs = m_FS;
            }

            auto& sm = shine::render::ShaderManager::get();
            const std::string key = m_ShaderKey.empty() ? std::string("DefaultPhong") : m_ShaderKey;
            m_Program = sm.getOrCreateProgram(key, vs, fs);

            // Uniform locations via backend abstraction (no direct GL calls)
            m_LocationBaseColor = sm.getUniformLocation(m_Program, "u_BaseColor");
            m_LocationAmbient   = sm.getUniformLocation(m_Program, "u_Ambient");
            m_LocationShininess = sm.getUniformLocation(m_Program, "u_Shininess");
            m_LocationMetallic  = sm.getUniformLocation(m_Program, "u_Metallic");
            m_LocationRoughness = sm.getUniformLocation(m_Program, "u_Roughness");
            m_LocationAo        = sm.getUniformLocation(m_Program, "u_Ao");
            m_LocationLightDir  = sm.getUniformLocation(m_Program, "u_LightDir");
            m_LocationModel     = sm.getUniformLocation(m_Program, "u_Model");
        }

    public:
        [[nodiscard]] int32_t getLocationModel() const { return m_LocationModel; }

    private:
        // API-agnostic types (no GLuint / GLint)
        uint32_t m_Program{ 0 };
        int32_t  m_LocationBaseColor{ -1 };
        int32_t  m_LocationAmbient  { -1 };
        int32_t  m_LocationShininess{ -1 };
        int32_t  m_LocationMetallic { -1 };
        int32_t  m_LocationRoughness{ -1 };
        int32_t  m_LocationAo       { -1 };
        int32_t  m_LocationLightDir { -1 };
        int32_t  m_LocationModel    { -1 };

        std::string m_ShaderKey;
        std::string m_VS;
        std::string m_FS;
        std::array<float,3> m_BaseColor{ 0.95f, 0.75f, 0.55f };
        std::array<float,3> m_Ambient  { 0.15f, 0.15f, 0.18f };
        std::array<float,3> m_LightDir { -0.3f, -0.7f, -0.6f };
        float m_Shininess{ 32.0f };
        float m_Metallic { 0.0f };
        float m_Roughness{ 0.5f };
        float m_Ao       { 1.0f };
    };
}
