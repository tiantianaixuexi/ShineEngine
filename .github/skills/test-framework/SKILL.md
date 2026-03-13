---
name: test-framework
description: "ShineEngine test program structure, module registration, build command, and test/benchmark API. Invoke when creating a new test executable, adding correctness checks, or writing benchmark comparisons using the shine::test framework."
---

# ShineEngine Test Framework

## Invoke When

- Creating a new test or benchmark executable under `dev/test/`
- Adding correctness checks (`TestContext`)
- Writing side-by-side performance benchmarks (`run_compare`)
- Registering a new test target (module JSON + build command)

---

## File Layout

```
dev/test/<TestName>/
    <TestName>.cpp        ← single translation unit
Module/test/
    <TestName>.json       ← module registration
```

---

## Module JSON (`Module/test/<TestName>.json`)

```json
{
  "name": "TestName",
  "files": [
    "dev/test/TestName/TestName.cpp"
  ],
  "deps": [
    "fmt"
  ],
  "defines": [
    "TEST_BUILD"
  ],
  "link": {
    "debug":   { "lib": ["mimallocd.lib"] },
    "release": { "lib": ["mimalloc.lib"]  }
  },
  "type": ["exe"],
  "platform": ["Windows"],
  "buildMode": ["both"],
  "output": "exe/TestName.exe",
  "comment": "..."
}
```

Add extra engine deps to `"deps"` only when needed (e.g. `"string_util"`, `"math"`).

---

## Build & Run

```powershell
.\build.bat test <TestName>            # Debug
.\build.bat test <TestName> --release  # Release (use for benchmarks)
```

---

## Standard Includes

```cpp
#include "../../src/string/shine_string.h"   // if using SString/STextView
#include "../common/test_benchmark_framework.h"
#include "fmt/base.h"
#include "fmt/format.h"
```

`std::formatter<shine::SString>` and `std::formatter<shine::STextView>` are defined in
`shine_string.h` / `shine_text_view.h` — **do not redefine them** in test files.

---

## Correctness Tests — `TestContext`

```cpp
static shine::test::TestContext g_ctx;
#define CHECK(name, cond) SHINE_TEST_CHECK(g_ctx, name, cond)

void test_something()
{
    CHECK("default empty",     s.empty());
    CHECK("size after assign", s.size() == 5);
}
```

Print summary and return exit code:

```cpp
g_ctx.print_summary("My Test Results");
if (!g_ctx.all_passed()) { return 1; }
return 0;
```

---

## Benchmark — `run_compare`

```cpp
#include <vector>
using shine::test::ComparisonResult;
using shine::test::run_compare;
using shine::test::print_comparison;
using shine::test::print_summary;
using shine::test::DoNotOptimize;

std::vector<ComparisonResult> results;

results.push_back(run_compare(
    "category",        // display category label
    "case name",       // display case name
    24,                // rounds
    4000,              // inner iterations per round
    [&]() {            // lhs lambda (shine side)
        shine::SString s("hello");
        DoNotOptimize(s);
    },
    [&]() {            // rhs lambda (std side)
        std::string s("hello");
        DoNotOptimize(s);
    },
    "shine",           // lhs label
    "std",             // rhs label
    "Group/SubGroup"   // group name for summary
));

// Print each result inline:
for (const auto& r : results) { print_comparison(r); }

// Print final summary table:
print_summary(results, "shine", "std");
```

### Rounds & Iterations Guidelines

| Test type            | Rounds | Inner iters |
|----------------------|--------|-------------|
| Hot path (< 10 ns)   | 24     | 4 000       |
| Medium path (10–100 ns) | 16  | 1 000       |
| Heavy path (> 1 µs)  | 8      | 20          |

---

## Single Benchmark — `measure_benchmark`

```cpp
shine::test::BenchmarkStats st = shine::test::measure_benchmark(
    [&]() { /* work */ }, /*rounds=*/16, /*iters=*/2000);

fmt::println("mean {:.2f} ns", st.mean_ns);
```

---

## Anti-optimization Helpers

```cpp
shine::test::DoNotOptimize(value);   // prevent dead-code elimination on a value
shine::test::ClobberMemory();        // prevent load/store reordering across boundary
```

Always call `DoNotOptimize` on the result of the benchmarked expression.

---

## `main()` Template

```cpp
int main()
{
    fmt::println("╔══════════════════════════════════════════════════════════════════╗");
    fmt::println("║                ShineEngine — TestName                          ║");
    fmt::println("╚══════════════════════════════════════════════════════════════════╝\n");

    // --- correctness ---
    test_something();
    g_ctx.print_summary("Correctness");
    if (!g_ctx.all_passed()) { return 1; }

    // --- benchmarks (optional) ---
    std::vector<shine::test::ComparisonResult> results;
    benchmark_all(results);
    shine::test::print_summary(results, "shine", "std");

    return 0;
}
```

---

## Checklist

- [ ] `TestName.cpp` placed under `dev/test/TestName/`
- [ ] `Module/test/TestName.json` created with correct file path
- [ ] `DoNotOptimize` called on every benchmarked value
- [ ] `--release` used when running benchmarks
- [ ] `g_ctx.all_passed()` checked; `main()` returns 1 on failure
- [ ] No formatter redefinitions for `SString`/`STextView`
