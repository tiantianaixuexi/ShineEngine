#pragma once

#include "../Container/SArray.h"
#include "../Container/SVector.h"

namespace shine {
namespace graphics {

#if defined(__clang__)
#define SHINE_FORCE_INLINE inline __attribute__((always_inline))
#else
#define SHINE_FORCE_INLINE inline
#endif

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
static constexpr int CMD_BIND_BUFFER_BASE = 18;
static constexpr int CMD_UNIFORM_RR_BLOCK = 19;

class CommandBuffer {
public:
    struct Pass {
        CommandBuffer* owner = nullptr;
        int* write = nullptr;
        int* end = nullptr;
        int count = 0;

        SHINE_FORCE_INLINE Pass(CommandBuffer* o, int* w, int* e) : owner(o), write(w), end(e), count(0) {}
        Pass(const Pass&) = delete;
        Pass& operator=(const Pass&) = delete;
        SHINE_FORCE_INLINE Pass(Pass&& other) noexcept
            : owner(other.owner), write(other.write), end(other.end), count(other.count) {
            other.owner = nullptr;
            other.write = nullptr;
            other.end = nullptr;
            other.count = 0;
        }
        Pass& operator=(Pass&&) = delete;
        ~Pass() { commit(); }

        SHINE_FORCE_INLINE void push(int op, int a, int b, int c, int d, int e, int f, int g) {
            if (write + 8 > end) {
                if (owner) owner->note_overflow();
                return;
            }
            owner->update_stats(op, c, d);
            int* p = write;
            p[0] = op;
            p[1] = a;
            p[2] = b;
            p[3] = c;
            p[4] = d;
            p[5] = e;
            p[6] = f;
            p[7] = g;
            write = p + 8;
            ++count;
        }

        SHINE_FORCE_INLINE void commit() {
            if (!owner) return;
            owner->commit_pass(write, count);
            owner = nullptr;
        }
    };

    static CommandBuffer& instance();

    void reset();
    void push(int op, int a, int b, int c, int d, int e, int f, int g);
    SHINE_FORCE_INLINE int* reserve8();
    SHINE_FORCE_INLINE void update_stats(int op, int c, int d);
    SHINE_FORCE_INLINE void note_overflow();
    SHINE_FORCE_INLINE Pass begin_pass();
    SHINE_FORCE_INLINE void commit_pass(int* write, int count);
    
    int* getData();
    int getCount() const;
    const int* getSubmitData(int& out_count);
    int getOverflowCount() const { return m_overflow; }
    int* getOverflowData() { return m_overflow_cmds.data(); }
    int getOverflowCmdCount() const { return (int)(m_overflow_cmds.size() / 8u); }

    // Stats
    int getDrawCalls() const { return m_drawCalls; }
    int getVertices() const { return m_vertices; }
    int getInstances() const { return m_instances; }

private:
    friend class CommandBuffer; // Should not be needed for static instance but for completeness
    

public:
    CommandBuffer() = default;
private:
    
    static constexpr int MAX_CMDS = 1024;
    // 8 ints per command
    int m_cmds[MAX_CMDS * 8];
    int* m_write = m_cmds;
    int* m_end = m_cmds + (MAX_CMDS * 8);
    int m_count = 0;
    int m_overflow = 0;
    shine::wasm::SVector<int> m_overflow_cmds;
    shine::wasm::SVector<int> m_submit_cmds;

    // Stats counters
    int m_drawCalls = 0;
    int m_vertices = 0;
    int m_instances = 0;
};

SHINE_FORCE_INLINE int* CommandBuffer::reserve8() {
    if (likely(m_write + 8 <= m_end)) {
        int* p = m_write;
        m_write = p + 8;
        m_count++;
        return p;
    }
    ++m_overflow;
    if (m_overflow_cmds.capacity() < 2048u) {
        m_overflow_cmds.reserve(2048u);
    }
    const unsigned int old = m_overflow_cmds.size();
    m_overflow_cmds.resize_uninitialized(old + 8u);
    return m_overflow_cmds.data() + old;
}

SHINE_FORCE_INLINE void CommandBuffer::note_overflow() {
    ++m_overflow;
}

SHINE_FORCE_INLINE void CommandBuffer::update_stats(int op, int c, int d) {
#if defined(DEBUG) && DEBUG
    if (op == CMD_DRAW_ARRAYS) {
        m_drawCalls++;
        m_vertices += c;
    } else if (op == CMD_DRAW_ARRAYS_INSTANCED) {
        m_drawCalls++;
        m_vertices += c * d;
        m_instances += d;
    }
#else
    (void)op;
    (void)c;
    (void)d;
#endif
}

SHINE_FORCE_INLINE CommandBuffer::Pass CommandBuffer::begin_pass() {
    return Pass(this, m_write, m_end);
}

SHINE_FORCE_INLINE void CommandBuffer::commit_pass(int* write, int count) {
    m_write = write;
    m_count += count;
}

// Global instance to avoid extra indirection in hot paths.
extern CommandBuffer g_cmd_buffer;

// Global helper
static inline void cmd_push(int op, int a, int b, int c, int d, int e, int f, int g) {
    int* p = g_cmd_buffer.reserve8();
    if (!p) return;
    g_cmd_buffer.update_stats(op, c, d);
    p[0] = op;
    p[1] = a;
    p[2] = b;
    p[3] = c;
    p[4] = d;
    p[5] = e;
    p[6] = f;
    p[7] = g;
}

static inline void cmd_reset() {
    g_cmd_buffer.reset();
}

#undef SHINE_FORCE_INLINE

} // namespace graphics
} // namespace shine
