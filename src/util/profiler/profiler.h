#pragma once

#include <string>
#include <vector>
#include <algorithm>

#include "shine_define.h"
#include "util/timer/timer_util.h"

SHINE_NAMESPACE(::util)

    struct PerformanceStats {
        float minTime = 0.0f;
        float maxTime = 0.0f;
        float avgTime = 0.0f;
        u32 callCount = 0;
        u32 frameCount = 0;
    };

    class Profiler {
    public:
        Profiler() = default;
        ~Profiler() = default;

        void BeginFrame() {
            _frameStartTime = get_now_ms_platform<float>();
        }

        void EndFrame() {
            _frameCount++;
        }

        u32 CreateEntry(const std::string& name) {
            u32 id = static_cast<u32>(_entries.size());
            _entries.push_back({ name, PerformanceStats{} });
            return id;
        }

        void Record(u32 id, float duration) {
            if (id >= _entries.size())
                return;

            auto& stats = _entries[id].stats;
            
            if (stats.callCount == 0) {
                stats.minTime = duration;
                stats.maxTime = duration;
            } else {
                stats.minTime = std::min(stats.minTime, duration);
                stats.maxTime = std::max(stats.maxTime, duration);
            }

            stats.avgTime = (stats.avgTime * stats.callCount + duration) / (stats.callCount + 1);
            stats.callCount++;
            stats.frameCount = _frameCount;
        }

        void Reset(u32 id) {
            if (id < _entries.size()) {
                _entries[id].stats = PerformanceStats{};
            }
        }

        void ResetAll() {
            _frameCount = 0;
            for (auto& entry : _entries) {
                entry.stats = PerformanceStats{};
            }
        }

        struct PerformanceReport {
            u32 id;
            std::string name;
            float minTime;
            float maxTime;
            float avgTime;
            u32 callCount;
        };

        std::vector<PerformanceReport> GetTopSlowest(u32 count = 10) {
            std::vector<PerformanceReport> reports;
            
            for (u32 i = 0; i < _entries.size(); ++i) {
                if (_entries[i].stats.callCount > 0) {
                    reports.push_back({
                        i,
                        _entries[i].name,
                        _entries[i].stats.minTime,
                        _entries[i].stats.maxTime,
                        _entries[i].stats.avgTime,
                        _entries[i].stats.callCount
                    });
                }
            }

            std::sort(reports.begin(), reports.end(), 
                [](const PerformanceReport& a, const PerformanceReport& b) {
                    return a.avgTime > b.avgTime;
                });

            if (reports.size() > count) {
                reports.resize(count);
            }

            return reports;
        }

        const PerformanceStats& GetStats(u32 id) const {
            static const PerformanceStats empty{};
            if (id < _entries.size()) {
                return _entries[id].stats;
            }
            return empty;
        }

        u32 GetFrameCount() const {
            return _frameCount;
        }

    private:
        struct ProfilerEntry {
            std::string name;
            PerformanceStats stats;
        };

        std::vector<ProfilerEntry> _entries;
        float _frameStartTime = 0.0f;
        u32 _frameCount = 0;
    };

    class ScopedProfiler {
    public:
        ScopedProfiler(Profiler& profiler, u32 id)
            : _profiler(profiler)
            , _id(id)
        {
            _startTime = get_now_ms_platform<float>();
        }

        ~ScopedProfiler() {
            float duration = get_now_ms_platform<float>() - _startTime;
            _profiler.Record(_id, duration);
        }

    private:
        Profiler& _profiler;
        u32 _id;
        float _startTime;
    };

SHINE_NAMESPACE_END