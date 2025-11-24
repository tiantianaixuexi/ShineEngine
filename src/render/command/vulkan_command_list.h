export module shine.render.vulkan_command_list;

#include "shine.shine_define.h"
#include "render/command.h"


// Placeholder for Vulkan command list (record/submit via VkCommandBuffer).
namespace shine::render::vulkan
{
    using command::ICommandList;

    class VulkanCommandList final : public ICommandList
    {
    public:
        void begin() override {}
        void end() override {}
        void execute() override {}
        void reset() override {}

        void bindFramebuffer(u64) override {}
        void setViewport(s32, s32, s32, s32) override {}
        void clearColor(float, float, float, float) override {}
        void clear(bool, bool) override {}
        void enableDepthTest(bool) override {}
        void useProgram(u64) override {}
        void bindVertexArray(u64) override {}
        void drawTriangles(s32, s32) override {}
        void drawIndexedTriangles(s32, command::IndexType, u64) override {}
        void imguiRender(void*) override {}
        void swapBuffers(void*) override {}
    };
}

