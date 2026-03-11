#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <format>
#include <functional>
#include <limits>
#include <numeric>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "fmt/base.h"
#include "fmt/format.h"

namespace shine::test
{
    // =========================================================
    // Anti-optimization helpers
    // =========================================================

    template <typename T>
    __declspec(noinline) void DoNotOptimize(const T& value)
    {
        volatile const void* p = &value;
        (void)p;
    }

    inline void ClobberMemory()
    {
        std::atomic_signal_fence(std::memory_order_seq_cst);
    }

    // =========================================================
    // Correctness test context
    // =========================================================

    struct TestContext
    {
        int pass = 0;
        int fail = 0;

        void check(std::string_view name, bool cond)
        {
            if (cond)
            {
                fmt::println("  [PASS] {}", name);
                ++pass;
            }
            else
            {
                fmt::println("  [FAIL] {}", name);
                ++fail;
            }
        }

        [[nodiscard]] bool all_passed() const noexcept
        {
            return fail == 0;
        }

        void print_summary(std::string_view title = "Correctness Result") const
        {
            fmt::println("══════════════════════════════════════════════════════════════════");
            fmt::println("  {}: {} passed, {} failed\n", title, pass, fail);
        }
    };

    // =========================================================
    // Benchmark stats
    // =========================================================

    struct BenchmarkStats
    {
        double mean_ns = 0.0;
        double min_ns = 0.0;
        double max_ns = 0.0;
        double median_ns = 0.0;
    };

    template <typename Fn>
    BenchmarkStats measure_benchmark(Fn&& fn, int rounds, int inner_iterations)
    {
        std::vector<double> samples;
        samples.reserve(static_cast<std::size_t>(rounds));

        for (int i = 0; i < 3; ++i)
        {
            fn();
        }

        for (int r = 0; r < rounds; ++r)
        {
            const auto start = std::chrono::steady_clock::now();
            for (int i = 0; i < inner_iterations; ++i)
            {
                fn();
            }
            const auto end = std::chrono::steady_clock::now();

            const double total_ns =
                static_cast<double>(
                    std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());

            samples.push_back(total_ns / static_cast<double>(inner_iterations));
        }

        std::sort(samples.begin(), samples.end());

        BenchmarkStats st;
        st.min_ns = samples.front();
        st.max_ns = samples.back();
        st.median_ns = samples[samples.size() / 2];
        st.mean_ns =
            std::accumulate(samples.begin(), samples.end(), 0.0) /
            static_cast<double>(samples.size());
        return st;
    }

    // =========================================================
    // Comparison result
    // =========================================================

    struct ComparisonResult
    {
        std::string category;
        std::string name;
        std::string group;
        BenchmarkStats lhs;
        BenchmarkStats rhs;
        std::string lhs_name = "lhs";
        std::string rhs_name = "rhs";
        bool lhs_wins = false;
        double ratio = 1.0; // rhs / lhs
    };

    struct GroupSummary
    {
        std::string group;
        int case_count = 0;
        int lhs_wins = 0;
        int rhs_wins = 0;
    };

    struct SummaryStats
    {
        int total_cases = 0;
        int lhs_wins = 0;
        int rhs_wins = 0;
        double best_ratio = 0.0;
        std::string best_name;
        double worst_ratio = std::numeric_limits<double>::max();
        std::string worst_name;
        std::vector<GroupSummary> groups;
    };

    inline void print_stats_line(std::string_view left, const BenchmarkStats& s)
    {
        fmt::println(
            "    {:<14} mean {:>12.2f} ns | median {:>12.2f} ns | min {:>12.2f} ns | max {:>12.2f} ns",
            left,
            s.mean_ns,
            s.median_ns,
            s.min_ns,
            s.max_ns);
    }

    inline void print_comparison(const ComparisonResult& r)
    {
        fmt::println("【{}】{}", r.category, r.name);
        print_stats_line(r.lhs_name, r.lhs);
        print_stats_line(r.rhs_name, r.rhs);

        if (r.lhs_wins)
        {
            fmt::println("    => winner: {}  ({:.2f}x faster)\n", r.lhs_name, r.ratio);
        }
        else
        {
            fmt::println("    => winner: {}  ({:.2f}x faster)\n", r.rhs_name, 1.0 / r.ratio);
        }
    }

    template <typename LhsFn, typename RhsFn>
    ComparisonResult run_compare(
        std::string category,
        std::string name,
        int rounds,
        int inner_iterations,
        LhsFn&& lhs_fn,
        RhsFn&& rhs_fn,
        std::string lhs_name = "lhs",
        std::string rhs_name = "rhs",
        std::string group = {})
    {
        ComparisonResult r;
        r.category = std::move(category);
        r.name = std::move(name);
        r.group = std::move(group);
        r.lhs_name = std::move(lhs_name);
        r.rhs_name = std::move(rhs_name);

        r.lhs = measure_benchmark(std::forward<LhsFn>(lhs_fn), rounds, inner_iterations);
        r.rhs = measure_benchmark(std::forward<RhsFn>(rhs_fn), rounds, inner_iterations);

        r.ratio = r.rhs.mean_ns / r.lhs.mean_ns;
        r.lhs_wins = r.lhs.mean_ns <= r.rhs.mean_ns;
        return r;
    }

    inline SummaryStats build_summary(const std::vector<ComparisonResult>& results)
    {
        SummaryStats summary;
        summary.total_cases = static_cast<int>(results.size());

        for (const auto& r : results)
        {
            if (r.lhs_wins)
            {
                ++summary.lhs_wins;
                if (r.ratio > summary.best_ratio)
                {
                    summary.best_ratio = r.ratio;
                    summary.best_name = std::format("[{}] {}", r.category, r.name);
                }
            }
            else
            {
                ++summary.rhs_wins;
                if (r.ratio < summary.worst_ratio)
                {
                    summary.worst_ratio = r.ratio;
                    summary.worst_name = std::format("[{}] {}", r.category, r.name);
                }
            }

            const std::string group_name = r.group.empty() ? std::string("未分组") : r.group;

            auto it = std::find_if(
                summary.groups.begin(),
                summary.groups.end(),
                [&](const GroupSummary& g) { return g.group == group_name; });

            if (it == summary.groups.end())
            {
                summary.groups.push_back(GroupSummary{ group_name, 0, 0, 0 });
                it = summary.groups.end() - 1;
            }

            ++it->case_count;
            if (r.lhs_wins)
            {
                ++it->lhs_wins;
            }
            else
            {
                ++it->rhs_wins;
            }
        }

        return summary;
    }

    inline void print_group_summary(
        const SummaryStats& summary,
        std::string_view lhs_name = "lhs",
        std::string_view rhs_name = "rhs")
    {
        fmt::println("╔══════════════════════════════════════════════════════════════════╗");
        fmt::println("║                         分类统计                                ║");
        fmt::println("╠══════════════════════════════════════════════════════════════════╣");

        for (const auto& g : summary.groups)
        {
            fmt::println(
                "║  {:<10} cases {:<4} | {} {:<4} | {} {:<4}                 ║",
                g.group,
                g.case_count,
                lhs_name,
                g.lhs_wins,
                rhs_name,
                g.rhs_wins);
        }

        fmt::println("╚══════════════════════════════════════════════════════════════════╝");
    }

    // =========================================================
    // Summary reporting
    // =========================================================

    inline void print_summary(
        const std::vector<ComparisonResult>& results,
        std::string_view lhs_name = "lhs",
        std::string_view rhs_name = "rhs")
    {
        const SummaryStats summary = build_summary(results);

        fmt::println("╔══════════════════════════════════════════════════════════════════╗");
        fmt::println("║                            总结                                 ║");
        fmt::println("╠══════════════════════════════════════════════════════════════════╣");
        fmt::println("║  总 case 数: {:<50} ║", summary.total_cases);
        fmt::println(
            "║  {} 胜: {:<5}   {} 胜: {:<5}                             ║",
            lhs_name,
            summary.lhs_wins,
            rhs_name,
            summary.rhs_wins);
        fmt::println("╠══════════════════════════════════════════════════════════════════╣");

        if (!summary.best_name.empty())
        {
            fmt::println("║  最大优势: {:<52} ║", summary.best_name);
            fmt::println("║  加速比  : {:>8.2f}x                                          ║", summary.best_ratio);
        }
        else
        {
            fmt::println("║  最大优势: 无                                                  ║");
        }

        if (!summary.worst_name.empty() && summary.worst_ratio < std::numeric_limits<double>::max())
        {
            fmt::println("║  最大劣势: {:<52} ║", summary.worst_name);
            fmt::println("║  减速比  : {:>8.2f}x                                          ║", 1.0 / summary.worst_ratio);
        }
        else
        {
            fmt::println("║  最大劣势: 无                                                  ║");
        }

        fmt::println("╚══════════════════════════════════════════════════════════════════╝");

        if (!summary.groups.empty())
        {
            print_group_summary(summary, lhs_name, rhs_name);
        }
    }

    // =========================================================
    // Convenience macros
    // =========================================================

    #define SHINE_TEST_CHECK(ctx, name, cond) \
        do { (ctx).check((name), static_cast<bool>(cond)); } while (0)

} // namespace shine::test