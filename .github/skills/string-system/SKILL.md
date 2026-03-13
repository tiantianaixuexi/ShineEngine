---
name: string-system
description: "Mandatory rules for using the Shine Engine UTF-8 custom string types (SString and STextView). Invoke when writing ANY project-internal code that handles strings — including new functions, parameters, return values, data members, and containers. All custom strings MUST be used within the project unless the code is part of an external interface (OS APIs, third-party libs, serialization boundaries). Prevents accidental use of std::string/std::string_view/char* in internal code and ensures allocation-efficient engine code."
---

# Shine Engine Custom String System

## Mandatory Usage Condition

> **All string handling in project-internal code MUST use `SString` / `STextView`.**
> The only exception is code that directly interfaces with external systems:
> - OS / Win32 / POSIX APIs that require `std::string` or `char*`
> - Third-party library headers that cannot be changed (e.g. `std::filesystem`, `fmt`, `rapidjson`)
> - Public serialization / network protocol boundaries where a standard type is mandated by the protocol

If you are writing internal engine code (gameplay, editor, utilities, asset pipeline, etc.) and find yourself reaching for `std::string`, stop and use `SString` instead.

---

## Core Types

| Type        | Purpose                          | Size  |
|-------------|----------------------------------|-------|
| `SString`   | Owning, mutable UTF-8 string     | 32 B  |
| `STextView` | Non-owning, read-only UTF-8 view | 16 B  |

Both live in `src/string/shine_string.h` (which includes `shine_text_view.h`).

---

## Which Type to Use

| Situation                                   | Type        |
|---------------------------------------------|-------------|
| Function **parameter** (read-only input)    | `STextView` |
| Function **return value** (new string)      | `SString`   |
| Class **data member** (stored string)       | `SString`   |
| Temporary view over existing storage        | `STextView` |
| String literal (no allocation)              | `STextView` via `from_literal` |
| External API boundary (OS/third-party)      | `std::string` / `const char*` (exception) |

---

## Construction & Factories

```cpp
// STextView — zero allocation, borrows lifetime
STextView tv = "hello";                          // implicit from const char*
STextView tv = some_sstring;                     // implicit from SString (operator STextView())
STextView tv = std::string_view("hello");        // implicit from string_view
STextView tv = STextView::from_literal("hello"); // compile-time length, preferred for literals
STextView tv = STextView::from_cstring(ptr);     // runtime strlen, nullable
STextView tv = STextView::from_sv(sv);           // explicit from std::string_view
STextView tv = STextView::from_string(s);        // explicit from std::string (NOT s.size()-1!)

// SString — may allocate (SSO up to 30 bytes, heap above)
SString s = "hello";                   // implicit from const char*
SString s("hello");                    // explicit from const char*
SString s(STextView("hello"));         // explicit from STextView
SString s(std::string_view("hello"));  // explicit from string_view
SString s(existing_std_string);        // explicit from std::string
```

---

## Implicit Conversion Matrix

| From               | To                  | Implicit? |
|--------------------|---------------------|-----------|
| `const char*`      | `STextView`         | **YES**   |
| `std::string_view` | `STextView`         | **YES**   |
| `SString`          | `STextView`         | **YES** (`operator STextView()`) |
| `std::string`      | `STextView`         | NO — two UDC chain blocked by standard |
| `STextView`        | `std::string_view`  | **YES** (`operator std::string_view()`) |
| `STextView`        | `SString`           | NO — explicit ctor |
| `const char*`      | `SString`           | **YES**   |
| `STextView`        | `std::string`       | NO — explicit `.to_string()` |
| `SString`          | `std::string_view`  | NO — two UDC chain |
| `SString`          | `std::string`       | NO — explicit `.to_string()` |

---

## Common Operations

```cpp
// Modification (SString only)
s += " world";
s.append(view);
s.push_back('!');
s.insert(3, "...");
s.erase(0, 5);
s.resize(10, ' ');
s.clear();

// Searching (both types)
s.find("sub");          // byte offset or npos
s.contains("sub");
s.starts_with("pre");
s.ends_with("suf");
s.find_first_of("abc");
s.rfind('x');

// Slicing
STextView sub = s.subview(2, 5);  // zero-cost view into SString
SString   copy = s.substr(2, 5);  // allocates

// Trimming (returns STextView — no allocation)
STextView trimmed = s.trim();

// Replacement
SString replaced  = s.replace("old", "new");   // new string
s.replace_inplace("old", "new");               // mutate in-place
s.replace_first("old", "new");                 // replace first occurrence

// Conversion to std (only at external boundaries)
std::string  out = s.to_string();
std::string_view sv = s.sv();    // or STextView implicit conversion
const char*  c = s.c_str();
```

---

## UTF-8 Operations

Use the `code_point_` / `cp_` variants for character-level (not byte-level) operations:

```cpp
tv.code_point_count()                // number of Unicode code points
tv.byte_index_from_code_point(n)     // byte offset of nth code point
tv.substr_code_points(pos, count)    // slice by code points
tv.find_code_point(U'文')            // find code point
tv.contains_code_point(U'文')
tv.for_each_code_point([](char32_t cp){ ... });
```

Default `size()`, `substr()`, `find()`, `operator[]` all operate on **bytes**.

---

## Formatting (`std::format` / `fmt`)

`std::formatter<SString>` and `std::formatter<STextView>` are defined in `shine_string.h` / `shine_text_view.h`.
**Do NOT redefine them** in individual `.cpp` or test files.

```cpp
fmt::println("name={}", some_sstring);
fmt::println("view={}", some_stextview);
auto s = std::format("key={} val={}", sstring, textview);
```

---

## Lifetime Warning (`SHINE_LIFETIMEBOUND`)

`operator STextView()` on `SString` is annotated with `SHINE_LIFETIMEBOUND`.
The compiler (MSVC/Clang) will warn if you create a dangling view from a temporary:

```cpp
STextView tv = SString("hello");  // ⚠ WARNING: temporary SString destroyed immediately
```

Always ensure the source `SString` outlives the `STextView`.

---

## Hashing

`std::hash<SString>` and `SString::hash()` both use the same FNV-1a implementation as `SString::static_hash()`. Results are consistent across all three call sites.

```cpp
std::unordered_map<SString, int> map;
map["key"] = 1;

std::size_t h = SString::static_hash("compile_time_key"); // constexpr
```

---

## AI Mandatory Checklist

1. **Every string parameter** → `STextView` (unless mutated inside the function)
2. **Every returned / stored string** → `SString`
3. **Never introduce `std::string`** in internal code — use `SString` and convert at the boundary
4. **Never introduce `std::string_view`** in internal code — use `STextView`
5. **Literals** → `STextView::from_literal("...")` (avoids runtime strlen)
6. **UTF-8 character ops** → use `code_point_` functions, never manual byte arithmetic
7. **Formatters** → do not redeclare; already provided by headers
8. **Dangling views** → ensure source `SString` lifetime covers all `STextView` uses
9. **External boundary** → convert with `.to_string()` / `.sv()` / `.c_str()`, only at the call site
