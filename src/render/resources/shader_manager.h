#pragma once


#include <unordered_map>
#include <string>
#include <functional>
#include <algorithm>

#include <GL/glew.h>



namespace shine::render
{
    class ShaderManager
    {
    public:
        static ShaderManager& get()
        {
            static ShaderManager instance;
            return instance;
        }

        // 获取或创建一个Program。这里用key做缓存键，实际应用中可扩展为材质/文件路径等
        GLuint getOrCreateProgram(const std::string& key,
                                  const char* vsSource,
                                  const char* fsSource)
        {
#ifdef SHINE_OPENGL
            auto it = m_ProgramCache.find(key);
            if (it != m_ProgramCache.end()) return it->second;

            GLuint v = glCreateShader(GL_VERTEX_SHADER);
            glShaderSource(v, 1, &vsSource, nullptr);
            glCompileShader(v);
            GLint ok = 0; glGetShaderiv(v, GL_COMPILE_STATUS, &ok);
            if (!ok) {
                char log[1024]{}; glGetShaderInfoLog(v, 1024, nullptr, log);
            }

            GLuint f = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(f, 1, &fsSource, nullptr);
            glCompileShader(f);
            glGetShaderiv(f, GL_COMPILE_STATUS, &ok);
            if (!ok) {
                char log[1024]{}; glGetShaderInfoLog(f, 1024, nullptr, log);
            }

            GLuint prog = glCreateProgram();
            glAttachShader(prog, v);
            glAttachShader(prog, f);
            glLinkProgram(prog);
            glGetProgramiv(prog, GL_LINK_STATUS, &ok);
            if (!ok) {
                char log[1024]{}; glGetProgramInfoLog(prog, 1024, nullptr, log);
            }
            glDeleteShader(v);
            glDeleteShader(f);

            // 缁熶竴灏?CameraUBO 缁戝畾鍒?binding=0锛堣嫢瀛樺湪锛?
            GLuint blockIndex = glGetUniformBlockIndex(prog, "CameraUBO");
            if (blockIndex != GL_INVALID_INDEX) {
                glUniformBlockBinding(prog, blockIndex, 0);
            }

            m_ProgramCache.emplace(key, prog);
            return prog;
#else
            (void)key; (void)vsSource; (void)fsSource;
            return 0;
#endif
        }

        // =============== 批量编译 / 进度统计 ===============
        // 用于预编译着色器并显示进度（在主线程异步调用，不创建新的GL线程）
        void enqueue(const std::string& key, const std::string& vs, const std::string& fs)
        {
#ifdef SHINE_OPENGL
            if (m_ProgramCache.find(key) != m_ProgramCache.end()) return;
            // 已在队列中则跳过
            auto it = std::find_if(m_Queue.begin(), m_Queue.end(), [&](const CompileJob& j){ return j.key == key; });
            if (it != m_Queue.end()) return;
            m_Queue.push_back(CompileJob{ key, vs, fs, JobStatus::Pending, 0, {} });
#else
            (void)key; (void)vs; (void)fs;
#endif
        }

        // 每次编译一个，返回是否还有其他未编译任务
        bool compileNext()
        {
#ifdef SHINE_OPENGL
            for (auto& job : m_Queue)
            {
                if (job.status != JobStatus::Pending) continue;
                job.status = JobStatus::Compiling;
                GLuint prog = 0; std::string log;
                bool ok = compileProgram(job.vsSource.c_str(), job.fsSource.c_str(), prog, log);
                if (ok) {
                    m_ProgramCache.emplace(job.key, prog);
                    job.program = prog;
                    job.status = JobStatus::Completed;
                } else {
                    job.program = 0;
                    job.status = JobStatus::Failed;
                    job.log = std::move(log);
                }
                break; // 只处理一个
            }
            return std::any_of(m_Queue.begin(), m_Queue.end(), [](const CompileJob& j){ return j.status == JobStatus::Pending; });
#else
            return false;
#endif
        }

        // 编译全部（阻塞），可在加载场景时调用；可选提供回调显示编译百分比
        void compileAllBlocking(const std::function<void(float, const std::string&)>& onProgress = {})
        {
#ifdef SHINE_OPENGL
            size_t total = m_Queue.size();
            size_t done = 0;
            for (auto& job : m_Queue)
            {
                if (job.status != JobStatus::Pending) { ++done; continue; }
                job.status = JobStatus::Compiling;
                GLuint prog = 0; std::string log;
                bool ok = compileProgram(job.vsSource.c_str(), job.fsSource.c_str(), prog, log);
                if (ok) {
                    m_ProgramCache.emplace(job.key, prog);
                    job.program = prog;
                    job.status = JobStatus::Completed;
                } else {
                    job.program = 0;
                    job.status = JobStatus::Failed;
                    job.log = std::move(log);
                }
                ++done;
                if (onProgress) onProgress(total == 0 ? 1.0f : (float)done / (float)total, job.key);
            }
#else
            (void)onProgress;
#endif
        }

        struct CompileStats { size_t total{0}; size_t completed{0}; size_t failed{0}; size_t pending{0}; };

        CompileStats getStats() const
        {
            CompileStats s{};
#ifdef SHINE_OPENGL
            s.total = m_Queue.size();
            for (auto& j : m_Queue) {
                if (j.status == JobStatus::Completed) ++s.completed;
                else if (j.status == JobStatus::Failed) ++s.failed;
                else if (j.status == JobStatus::Pending) ++s.pending;
            }
#endif
            return s;
        }

        float getProgress() const
        {
            CompileStats s = getStats();
            const size_t denom = std::max<size_t>(1, s.total);
            return static_cast<float>(s.completed) / static_cast<float>(denom);
        }

        // 清理：删除所有程序
        void clear()
        {
#ifdef SHINE_OPENGL
            for (const auto& [k, prog] : m_ProgramCache) {
                if (prog) glDeleteProgram(prog);
            }
            m_ProgramCache.clear();
#endif
        }

    private:
        ShaderManager() = default;
        ~ShaderManager() = default;
        ShaderManager(const ShaderManager&) = delete;
        ShaderManager& operator=(const ShaderManager&) = delete;

    private:
        std::unordered_map<std::string, GLuint> m_ProgramCache;

#ifdef SHINE_OPENGL
        enum class JobStatus { Pending, Compiling, Completed, Failed };
        struct CompileJob {
            std::string key;
            std::string vsSource;
            std::string fsSource;
            JobStatus status;
            GLuint program;
            std::string log;
        };
        std::vector<CompileJob> m_Queue;

        static bool compileProgram(const char* vs, const char* fs, GLuint& outProg, std::string& outLog)
        {
            GLint ok = 0; outProg = 0; outLog.clear();
            GLuint v = glCreateShader(GL_VERTEX_SHADER);
            glShaderSource(v, 1, &vs, nullptr);
            glCompileShader(v);
            glGetShaderiv(v, GL_COMPILE_STATUS, &ok);
            if (!ok) {
                char log[2048]{}; glGetShaderInfoLog(v, 2048, nullptr, log); outLog += "VS:"; outLog += log; glDeleteShader(v); return false;
            }

            GLuint f = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(f, 1, &fs, nullptr);
            glCompileShader(f);
            glGetShaderiv(f, GL_COMPILE_STATUS, &ok);
            if (!ok) {
                char log[2048]{}; glGetShaderInfoLog(f, 2048, nullptr, log); outLog += "FS:"; outLog += log; glDeleteShader(v); glDeleteShader(f); return false;
            }

            GLuint prog = glCreateProgram();
            glAttachShader(prog, v);
            glAttachShader(prog, f);
            glLinkProgram(prog);
            glGetProgramiv(prog, GL_LINK_STATUS, &ok);
            if (!ok) {
                char log[2048]{}; glGetProgramInfoLog(prog, 2048, nullptr, log); outLog += "LK:"; outLog += log; glDeleteShader(v); glDeleteShader(f); glDeleteProgram(prog); return false;
            }
            glDeleteShader(v);
            glDeleteShader(f);

            // 绑定 CameraUBO
            GLuint blockIndex = glGetUniformBlockIndex(prog, "CameraUBO");
            if (blockIndex != GL_INVALID_INDEX) {
                glUniformBlockBinding(prog, blockIndex, 0);
            }
            outProg = prog;
            return true;
        }
#endif
    };
}



