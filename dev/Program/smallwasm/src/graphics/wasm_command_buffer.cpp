#include "wasm_command_buffer.h"

namespace shine {
namespace graphics {

// Safe static instance in .bss
CommandBuffer g_cmd_buffer;

CommandBuffer& CommandBuffer::instance() {
    return g_cmd_buffer;
}

void CommandBuffer::reset() {
    m_count = 0;
    m_write = m_cmds;
    m_end = m_cmds + (MAX_CMDS * 8);
    m_drawCalls = 0;
    m_vertices = 0;
    m_instances = 0;
    m_overflow = 0;
    m_overflow_cmds.clear();
    m_submit_cmds.clear();
}

void CommandBuffer::push(int op, int a, int b, int c, int d, int e, int f, int g) {
    int* p = reserve8();
    if (!p) return;
    update_stats(op, c, d);
    p[0] = op;
    p[1] = a;
    p[2] = b;
    p[3] = c;
    p[4] = d;
    p[5] = e;
    p[6] = f;
    p[7] = g;
}

int* CommandBuffer::getData() {
    return m_cmds;
}

int CommandBuffer::getCount() const {
    return m_count;
}

const int* CommandBuffer::getSubmitData(int& out_count) {
    if (likely(m_overflow_cmds.empty())) {
        out_count = m_count;
        return m_cmds;
    }
    if (m_count == 0) {
        out_count = (int)(m_overflow_cmds.size() / 8u);
        return m_overflow_cmds.data();
    }
    const unsigned int total_cmds = (unsigned int)m_count + (unsigned int)(m_overflow_cmds.size() / 8u);
    const unsigned int total_ints = total_cmds * 8u;
    if (m_submit_cmds.capacity() < total_ints) {
        m_submit_cmds.reserve(total_ints);
    }
    m_submit_cmds.resize_uninitialized(total_ints);
    int* dst = m_submit_cmds.data();
    const unsigned int main_ints = (unsigned int)m_count * 8u;

    if (main_ints > 0) {
        raw_memcpy(dst, m_cmds, (shine::wasm::size_t)main_ints * sizeof(int));
    }
    const unsigned int overflow_ints = (unsigned int)m_overflow_cmds.size();
    raw_memcpy(dst + main_ints, m_overflow_cmds.data(), (shine::wasm::size_t)overflow_ints * sizeof(int));
    out_count = (int)total_cmds;
    return dst;
}

} // namespace graphics
} // namespace shine
