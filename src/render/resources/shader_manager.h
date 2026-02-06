#pragma once

#include <algorithm>
#include <expected>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
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

        [[nodiscard]]
        backend::IRenderBackend* GetBackend() const noexcept { return m_Backend; }

        // 获取或创建一个 Program，key 做缓存键
        uint32_t getOrCreateProgram(const std::string& key,
                                    std::string_view vsSource,
                                    std::string_view fsSource)
        {
            if (!m_Backend) return 0;

            if (auto it = m_ProgramCache.find(key); it != m_ProgramCache.end())
                return it->second;

            auto result = m_Backend->CreateShaderProgram(vsSource, fsSource);
            if (!result) {
                fmt::println("Shader compilation failed for {}: {}", key, result.error());
                return 0;
            }

            m_ProgramCache.emplace(key, *result);
            return *result;
        }

        // Uniform location 查询 (通过后端抽象，不直接调用 GL)
        int32_t getUniformLocation(uint32_t program, std::string_view name)
        {
            if (!m_Backend || program == 0) return -1;
            return m_Backend->GetUniformLocation(program, name);
        }

        // =============== 批量编译 / 进度统计 ===============

        void enqueue(const std::string& key, const std::string& vs, const std::string& fs)
        {
            if (m_ProgramCache.contains(key)) return;
            auto it = std::ranges::find_if(m_Queue, [&](const CompileJob& j){ return j.key == key; });
            if (it != m_Queue.end()) return;
            m_Queue.push_back(CompileJob{ key, vs, fs, JobStatus::Pending, 0, {} });
        }

        // 每次编译一个，返回是否还有未编译任务
        bool compileNext()
        {
            if (!m_Backend) return false;

            for (auto& job : m_Queue)
            {
                if (job.status != JobStatus::Pending) continue;
                job.status = JobStatus::Compiling;

                auto result = m_Backend->CreateShaderProgram(job.vsSource, job.fsSource);
                if (result) {
                    m_ProgramCache.emplace(job.key, *result);
                    job.program = *result;
                    job.status  = JobStatus::Completed;
                } else {
                    job.program = 0;
                    job.status  = JobStatus::Failed;
                    job.log     = std::move(result.error());
                }
                break;
            }
            return std::ranges::any_of(m_Queue, [](const CompileJob& j){ return j.status == JobStatus::Pending; });
        }

        // 编译全部（阻塞）
        void compileAllBlocking(const std::function<void(float, const std::string&)>& onProgress = {})
        {
            if (!m_Backend) return;

            const size_t total = m_Queue.size();
            size_t done = 0;
            for (auto& job : m_Queue)
            {
                if (job.status != JobStatus::Pending) { ++done; continue; }
                job.status = JobStatus::Compiling;

                auto result = m_Backend->CreateShaderProgram(job.vsSource, job.fsSource);
                if (result) {
                    m_ProgramCache.emplace(job.key, *result);
                    job.program = *result;
                    job.status  = JobStatus::Completed;
                } else {
                    job.program = 0;
                    job.status  = JobStatus::Failed;
                    job.log     = std::move(result.error());
                }
                ++done;
                if (onProgress) {
                    onProgress(total == 0 ? 1.0f : static_cast<float>(done) / static_cast<float>(total),
                               job.key);
                }
            }
        }

        struct CompileStats {
            size_t total     = 0;
            size_t completed = 0;
            size_t failed    = 0;
            size_t pending   = 0;
        };

        [[nodiscard]]
        CompileStats getStats() const
        {
            CompileStats s{};
            s.total = m_Queue.size();
            for (const auto& j : m_Queue) {
                switch (j.status) {
                    case JobStatus::Completed: ++s.completed; break;
                    case JobStatus::Failed:    ++s.failed;    break;
                    case JobStatus::Pending:   ++s.pending;   break;
                    default: break;
                }
            }
            return s;
        }

        [[nodiscard]]
        float getProgress() const
        {
            auto s = getStats();
            return static_cast<float>(s.completed) / static_cast<float>(std::max<size_t>(1, s.total));
        }

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

        backend::IRenderBackend* m_Backend{ nullptr };
        std::unordered_map<std::string, uint32_t> m_ProgramCache;

        enum class JobStatus { Pending, Compiling, Completed, Failed };
        struct CompileJob {
            std::string key;
            std::string vsSource;
            std::string fsSource;
            JobStatus   status;
            uint32_t    program;
            std::string log;
        };
        std::vector<CompileJob> m_Queue;
    };
}
