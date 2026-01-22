#pragma once

#include "../Container/SArray.h"

namespace shine {
namespace graphics {

// Command constants (shared with JS)
static constexpr int CMD_NOP = 0;
static constexpr int CMD_VIEWPORT = 1;
static constexpr int CMD_CLEAR_COLOR = 2;
static constexpr int CMD_CLEAR = 3;
static constexpr int CMD_USE_PROGRAM = 4;
static constexpr int CMD_BIND_BUFFER = 5;
static constexpr int CMD_BUFFER_DATA_F32 = 6;
static constexpr int CMD_BUFFER_SUB_DATA_F32 = 7;
static constexpr int CMD_DRAW_ARRAYS = 8;
static constexpr int CMD_ACTIVE_TEXTURE = 9;
static constexpr int CMD_BIND_TEXTURE = 10;
static constexpr int CMD_UNIFORM1I = 11;
static constexpr int CMD_UNIFORM1F = 12;
static constexpr int CMD_UNIFORM2F = 13;
static constexpr int CMD_UNIFORM4F = 14;
static constexpr int CMD_DRAW_ARRAYS_INSTANCED = 15;
static constexpr int CMD_BIND_VAO = 16;
static constexpr int CMD_SETUP_VIEW_SAMPLER2D = 17;

class CommandBuffer {
public:
    static CommandBuffer& instance();

    void reset();
    void push(int op, int a, int b, int c, int d, int e, int f, int g);
    
    int* getData();
    int getCount() const;

    // Stats
    int getDrawCalls() const { return m_drawCalls; }
    int getVertices() const { return m_vertices; }
    int getInstances() const { return m_instances; }

private:
    friend class CommandBuffer; // Should not be needed for static instance but for completeness
    
    // Make constructor public or friend the static instance holder?
    // The simplest way for a singleton in C++ without complex friend logic is public ctor
    // or just rely on static local variable in instance().
    // But here we used a file-scope static in cpp.
    // Let's make it public for simplicity or friend the file scope.
public:
    CommandBuffer() = default;
private:
    
    static constexpr int MAX_CMDS = 1024;
    // 8 ints per command
    int m_cmds[MAX_CMDS * 8];
    int m_count = 0;

    // Stats counters
    int m_drawCalls = 0;
    int m_vertices = 0;
    int m_instances = 0;
};

// Global helper
static inline void cmd_push(int op, int a, int b, int c, int d, int e, int f, int g) {
    CommandBuffer::instance().push(op, a, b, c, d, e, f, g);
}

static inline void cmd_reset() {
    CommandBuffer::instance().reset();
}

} // namespace graphics
} // namespace shine
