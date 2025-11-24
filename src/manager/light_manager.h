#pragma once




namespace shine::manager
{
    struct DirectionalLight
    {
        float dir[3] { -0.3f, -0.7f, -0.6f }; // 指向光源的方向（左手空间）
        float color[3] { 1.0f, 1.0f, 1.0f };  // 光照颜色
        float intensity { 1.0f };            // 强度
    };

    class LightManager
    {
    public:
        static LightManager& get()
        {
            static LightManager s;
            return s;
        }

        DirectionalLight& directional() { return m_DirLight; }
        const DirectionalLight& directional() const { return m_DirLight; }

        void setDirection(float x, float y, float z) { m_DirLight.dir[0]=x; m_DirLight.dir[1]=y; m_DirLight.dir[2]=z; }
        void setColor(float r, float g, float b) { m_DirLight.color[0]=r; m_DirLight.color[1]=g; m_DirLight.color[2]=b; }
        void setIntensity(float s) { m_DirLight.intensity = s; }

    private:
        DirectionalLight m_DirLight{};
    };
}



