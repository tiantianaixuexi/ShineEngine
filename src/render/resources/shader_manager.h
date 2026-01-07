#pragma once

#include <unordered_map>
#include <string>
#include <functional>
#include <algorithm>
#include <vector>

#include "render/backend/render_backend.h"
#include "fmt/format.h"

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

        void Initialize(backend::IRenderBackend* backend)
        {
            m_Backend = backend;
        }

        // 获取或创建一个Program。这里用key做缓存键
        uint32_t getOrCreateProgram(const std::string& key,
                                  const char* vsSource,
                                  const char* fsSource)
        {
            if (!m_Backend) return 0;

            auto it = m_ProgramCache.find(key);
            if (it != m_ProgramCache.end()) return it->second;

            std::string log;
            uint32_t prog = m_Backend->CreateShaderProgram(vsSource, fsSource, log);
            
            if (prog == 0) {
                fmt::println("Shader compilation failed for {}: {}", key, log);
                return 0;
            }

            m_ProgramCache.emplace(key, prog);
            return prog;
        }

        // =============== 批量编译 / 进度统计 ===============
        // 用于预编译着色器并显示进度（在主线程异步调用，不创建新的GL线程）
        void enqueue(const std::string& key, const std::string& vs, const std::string& fs)
        {
            if (m_ProgramCache.find(key) != m_ProgramCache.end()) return;
            // 已在队列中则跳过
            auto it = std::find_if(m_Queue.begin(), m_Queue.end(), [&](const CompileJob& j){ return j.key == key; });
            if (it != m_Queue.end()) return;
            m_Queue.push_back(CompileJob{ key, vs, fs, JobStatus::Pending, 0, {} });
        }

        // 每次编译一个，返回是否还有其他未编译任务
        bool compileNext()
        {
            if (!m_Backend) return false;

            for (auto& job : m_Queue)
            {
                if (job.status != JobStatus::Pending) continue;
                job.status = JobStatus::Compiling;
                
                std::string log;
                uint32_t prog = m_Backend->CreateShaderProgram(job.vsSource.c_str(), job.fsSource.c_str(), log);
                
                if (prog != 0) {
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
        }

        // 编译全部（阻塞），可在加载场景时调用；可选提供回调显示编译百分比
        void compileAllBlocking(const std::function<void(float, const std::string&)>& onProgress = {})
        {
            if (!m_Backend) return;

            size_t total = m_Queue.size();
            size_t done = 0;
            for (auto& job : m_Queue)
            {
                if (job.status != JobStatus::Pending) { ++done; continue; }
                job.status = JobStatus::Compiling;
                
                std::string log;
                uint32_t prog = m_Backend->CreateShaderProgram(job.vsSource.c_str(), job.fsSource.c_str(), log);
                
                if (prog != 0) {
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
        }

        struct CompileStats { size_t total{0}; size_t completed{0}; size_t failed{0}; size_t pending{0}; };

        CompileStats getStats() const
        {
            CompileStats s{};
            s.total = m_Queue.size();
            for (auto& j : m_Queue) {
                if (j.status == JobStatus::Completed) ++s.completed;
                else if (j.status == JobStatus::Failed) ++s.failed;
                else if (j.status == JobStatus::Pending) ++s.pending;
            }
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
            if (m_Backend) {
                for (const auto& [k, prog] : m_ProgramCache) {
                    if (prog) m_Backend->ReleaseShaderProgram(prog);
                }
            }
            m_ProgramCache.clear();
        }

    private:
        ShaderManager() = default;
        ~ShaderManager() = default;
        ShaderManager(const ShaderManager&) = delete;
        ShaderManager& operator=(const ShaderManager&) = delete;

    private:
        backend::IRenderBackend* m_Backend{ nullptr };
        std::unordered_map<std::string, uint32_t> m_ProgramCache;

        enum class JobStatus { Pending, Compiling, Completed, Failed };
        struct CompileJob {
            std::string key;
            std::string vsSource;
            std::string fsSource;
            JobStatus status;
            uint32_t program;
            std::string log;
        };
        std::vector<CompileJob> m_Queue;
    };
}



