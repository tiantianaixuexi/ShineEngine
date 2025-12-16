#include "command_buffer.h"

namespace shine {
namespace graphics {

// Safe static instance in .bss
static CommandBuffer s_cmd_buffer;

CommandBuffer& CommandBuffer::instance() {
    return s_cmd_buffer;
}

void CommandBuffer::reset() {
    m_count = 0;
    m_drawCalls = 0;
    m_vertices = 0;
    m_instances = 0;
}

void CommandBuffer::push(int op, int a, int b, int c, int d, int e, int f, int g) {
    if (m_count >= MAX_CMDS) return;
    
    // Stats
    if (op == CMD_DRAW_ARRAYS) {
        m_drawCalls++;
        // a=mode, b=first, c=count
        m_vertices += c;
    } else if (op == CMD_DRAW_ARRAYS_INSTANCED) {
        m_drawCalls++;
        // a=mode, b=first, c=count, d=instCount
        m_vertices += c * d;
        m_instances += d;
    }

    int* p = m_cmds + (m_count * 8);
    p[0] = op;
    p[1] = a;
    p[2] = b;
    p[3] = c;
    p[4] = d;
    p[5] = e;
    p[6] = f;
    p[7] = g;
    m_count++;
}

int* CommandBuffer::getData() {
    return m_cmds;
}

int CommandBuffer::getCount() const {
    return m_count;
}

} // namespace graphics
} // namespace shine
