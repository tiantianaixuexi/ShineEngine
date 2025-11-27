#pragma once


#include <memory>
#include <string>
#include <array>

#include <GL/glew.h>

#include "shader_manager.h"
#include "render/command/command_list.h"


namespace shine::render
{
    class Material
    {
    public:
        Material() = default;
        ~Material() = default;

        // 工厂方法：创建带有边缘光/分级着色的示例材质
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
                vec4 u_ViewPos; // xyz used
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
                vec4 u_ViewPos; // xyz used
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
                // Toon 分级
                float toon = floor(NdotL * 4.0) / 4.0;
                // Fresnel 边缘光
                float fres = pow(1.0 - max(dot(N, V), 0.0), 3.0);
                vec3 rimColor = vec3(0.2, 0.6, 1.0);
                // Blinn-Phong 高光
                float spec = pow(max(dot(N, H), 0.0), u_Shininess);
                vec3 diffuse = u_BaseColor * toon;
                vec3 specular = vec3(0.25) * spec;
                vec3 ambient = u_Ambient * u_BaseColor;
                vec3 rim = rimColor * fres * 0.7;
                color = vec4(ambient + diffuse + specular + rim, 1.0);
            }
            )";
            // 默认参数
            m->setBaseColor(0.9f, 0.5f, 0.3f);
            m->setAmbient(0.08f, 0.08f, 0.1f);
            m->setLightDir(-0.3f, -0.7f, -0.6f);
            m->setShininess(48.0f);
            return m;
        }

        // 单例：共享的 Fancy 材质（主要在 UI 中实时调节同一实例）
        static std::shared_ptr<Material> GetFancyRimToon()
        {
            static std::shared_ptr<Material> s_fancy;
            if (!s_fancy) s_fancy = CreateFancyRimToon();
            return s_fancy;
        }

        // 工厂方法：创建基于Cook-Torrance GGX的最简PBR材质（单方向光+无IBL）
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
            uniform vec3  u_Ambient; // ambient color factor

            const float PI = 3.14159265359;

            float DistributionGGX(vec3 N, vec3 H, float roughness)
            {
                float a      = roughness*roughness;
                float a2     = a*a;
                float NdotH  = max(dot(N, H), 0.0);
                float NdotH2 = NdotH*NdotH;
                float denom  = (NdotH2 * (a2 - 1.0) + 1.0);
                return a2 / (PI * denom * denom + 1e-5);
            }

            float GeometrySchlickGGX(float NdotV, float roughness)
            {
                float r = roughness + 1.0;
                float k = (r*r) / 8.0;
                return NdotV / (NdotV * (1.0 - k) + k + 1e-5);
            }

            float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
            {
                float NdotV = max(dot(N, V), 0.0);
                float NdotL = max(dot(N, L), 0.0);
                float ggx2  = GeometrySchlickGGX(NdotV, roughness);
                float ggx1  = GeometrySchlickGGX(NdotL, roughness);
                return ggx1 * ggx2;
            }

            vec3 FresnelSchlick(float cosTheta, vec3 F0)
            {
                return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
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

                // 基础反射率F0，金属使用albedo，非金属使用0.04
                vec3 F0 = mix(vec3(0.04), albedo, metallic);

                float NDF = DistributionGGX(N, H, roughness);
                float G   = GeometrySmith(N, V, L, roughness);
                vec3  F   = FresnelSchlick(max(dot(H, V), 0.0), F0);

                vec3 kS = F;
                vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

                float NdotL = max(dot(N, L), 0.0);
                float NdotV = max(dot(N, V), 0.0);
                vec3  numerator = NDF * G * F;
                float denom = max(4.0 * NdotV * NdotL, 1e-4);
                vec3  specular = numerator / denom;

                vec3 radiance = u_LColor.rgb * u_Inten.x;
                vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;

                vec3 ambient = u_Ambient * albedo * ao;
                vec3 outColor = ambient + Lo;
                color = vec4(outColor, 1.0);
            }
            )";

            // 初始化参数
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

        // 最小接口：绑定Program并设置材质参数（不包含相关模型变换）
        void bind(command::ICommandList& cmdList)
        {
#ifdef SHINE_OPENGL
            ensureCompiled();
            if (m_Program == 0) return;
            cmdList.useProgram(static_cast<std::uint64_t>(m_Program));
            // Uniform 设置通过命令列表记录，延迟到执行时
            if (m_LocationBaseColor >= 0) cmdList.setUniform3f(m_LocationBaseColor, m_BaseColor[0], m_BaseColor[1], m_BaseColor[2]);
            if (m_LocationAmbient   >= 0) cmdList.setUniform3f(m_LocationAmbient,   m_Ambient[0],   m_Ambient[1],   m_Ambient[2]);
            if (m_LocationShininess >= 0) cmdList.setUniform1f(m_LocationShininess, m_Shininess);
            if (m_LocationMetallic  >= 0) cmdList.setUniform1f(m_LocationMetallic,  m_Metallic);
            if (m_LocationRoughness >= 0) cmdList.setUniform1f(m_LocationRoughness, m_Roughness);
            if (m_LocationAo        >= 0) cmdList.setUniform1f(m_LocationAo,        m_Ao);
#endif
        }

        // 参数设置
        void setBaseColor(float r, float g, float b) { m_BaseColor = {r,g,b}; }
        void setAmbient(float r, float g, float b)   { m_Ambient = {r,g,b}; }
        void setLightDir(float x, float y, float z)  { m_LightDir = {x,y,z}; }
        void setShininess(float s)                   { m_Shininess = s; }
        void setMetallic(float m)                    { m_Metallic  = m; }
        void setRoughness(float r)                   { m_Roughness = r; }
        void setAo(float a)                          { m_Ao = a; }

        // 读取参数，用于ImGui实时调节
        std::array<float,3> getBaseColor() const { return m_BaseColor; }
        std::array<float,3> getAmbient()   const { return m_Ambient; }
        std::array<float,3> getLightDir()  const { return m_LightDir; }
        float getShininess() const { return m_Shininess; }
        float getMetallic()  const { return m_Metallic; }
        float getRoughness() const { return m_Roughness; }
        float getAo()        const { return m_Ao; }

        // 将内置Shader入队显示编译进度（可选调用）
        static void EnqueueBuiltinsForProgress()
        {
            // DefaultPhong 版本
            const char* vsPhong = R"(
            #version 330 core
            layout(location = 0) in vec3 aPos;
            layout(location = 1) in vec3 aNormal;
            out vec3 vNormal;
            out vec3 vWorldPos;
            uniform CameraUBO { mat4 u_VP; vec4 u_ViewPos; };
            void main(){ vNormal = aNormal; vWorldPos = aPos; gl_Position = u_VP * vec4(aPos, 1.0); }
            )";
            const char* fsPhong = R"(
            #version 330 core
            in vec3 vNormal; in vec3 vWorldPos; out vec4 color;
            uniform CameraUBO { mat4 u_VP; vec4 u_ViewPos; };
            layout(std140) uniform LightUBO { vec4 u_Dir; vec4 u_LColor; vec4 u_Inten; };
            uniform vec3 u_BaseColor; uniform vec3 u_Ambient; uniform float u_Shininess;
            void main(){ vec3 N=normalize(vNormal); vec3 L=normalize(-u_Dir.xyz); vec3 V=normalize(u_ViewPos.xyz-vWorldPos); vec3 H=normalize(L+V);
                float NdotL=max(dot(N,L),0.0)*u_Inten.x; float spec=pow(max(dot(N,H),0.0),u_Shininess);
                vec3 diffuse=(u_BaseColor*u_LColor.rgb)*NdotL; vec3 specular=(u_LColor.rgb)*(0.25*spec)*u_Inten.x; vec3 ambient=u_Ambient*u_BaseColor; color=vec4(ambient+diffuse+specular,1.0);} )";

            // Fancy 版本
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

            // PBR 版本
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

        // 获取一个默认的 Phong 材质（单例）
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
#ifdef SHINE_OPENGL
        void ensureCompiled()
        {
            if (m_Program != 0) return;
            const char* vs = nullptr;
            const char* fs = nullptr;
            // 若未指定着色器，则使用默认Phong
            if (m_VS.empty() || m_FS.empty()) {
                m_ShaderKey = "DefaultPhong";
                static const char* kVS = R"(
            #version 330 core
            layout(location = 0) in vec3 aPos;
            layout(location = 1) in vec3 aNormal;
            out vec3 vNormal;
            out vec3 vWorldPos;
            uniform CameraUBO {
                mat4 u_VP;
                vec4 u_ViewPos; // xyz used
            };
            void main(){
                vNormal = aNormal;
                vWorldPos = aPos;
                gl_Position = u_VP * vec4(aPos, 1.0);
            }
            )";
                static const char* kFS = R"(
            #version 330 core
            in vec3 vNormal;
            in vec3 vWorldPos;
            out vec4 color;
            uniform CameraUBO {
                mat4 u_VP;
                vec4 u_ViewPos; // xyz used
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
                float spec = pow(max(dot(N, H), 0.0), u_Shininess);
                vec3 diffuse = u_BaseColor * NdotL;
                vec3 specular = vec3(0.25) * spec;
                vec3 ambient = u_Ambient * u_BaseColor;
                color = vec4(ambient + diffuse + specular, 1.0);
            }
            )";
                vs = kVS; fs = kFS;
            } else {
                vs = m_VS.c_str(); fs = m_FS.c_str();
            }
            // 用ShaderManager统一编译/缓存Program
            m_Program = shine::render::ShaderManager::get().getOrCreateProgram(
                m_ShaderKey.empty() ? std::string("DefaultPhong") : m_ShaderKey,
                vs,
                fs
            );

            // uniform 位置
            m_LocationBaseColor = glGetUniformLocation(m_Program, "u_BaseColor");
            m_LocationAmbient   = glGetUniformLocation(m_Program, "u_Ambient");
            m_LocationShininess = glGetUniformLocation(m_Program, "u_Shininess");
            m_LocationMetallic  = glGetUniformLocation(m_Program, "u_Metallic");
            m_LocationRoughness = glGetUniformLocation(m_Program, "u_Roughness");
            m_LocationAo        = glGetUniformLocation(m_Program, "u_Ao");
        }
#endif

    private:
#ifdef SHINE_OPENGL
        GLuint m_Program { 0 };
        GLint  m_LocationBaseColor { -1 };
        GLint  m_LocationAmbient   { -1 };
        GLint  m_LocationShininess { -1 };
        GLint  m_LocationMetallic  { -1 };
        GLint  m_LocationRoughness { -1 };
        GLint  m_LocationAo        { -1 };
#endif
        std::string m_ShaderKey;
        std::string m_VS;
        std::string m_FS;
        std::array<float,3> m_BaseColor { 0.95f, 0.75f, 0.55f };
        std::array<float,3> m_Ambient   { 0.15f, 0.15f, 0.18f };
        std::array<float,3> m_LightDir  { -0.3f, -0.7f, -0.6f };
        float m_Shininess { 32.0f };
        float m_Metallic  { 0.0f };
        float m_Roughness { 0.5f };
        float m_Ao        { 1.0f };
    };
}



