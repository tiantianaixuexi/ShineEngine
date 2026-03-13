---
name: glaze-json
description: "JSON serialization/deserialization using the Glaze library (glz::). Invoke when reading or writing JSON from structs, files, or strings — including custom key names, optional fields, error handling, prettify/minify, and generic JSON traversal."
---

# Glaze JSON Usage

Use this skill for all JSON serialization tasks in ShineEngine.  
Header: `#include "glaze/json.hpp"` (or `"glaze/json/read.hpp"` + `"glaze/json/write.hpp"` separately).  
Version in this workspace: **Glaze 7.1.0**

---

## 1. Include

```cpp
#include "glaze/json.hpp"   // read + write + prettify + minify + generic
```

---

## 2. Automatic Reflection (No Meta Needed)

For aggregate structs (no private members, no custom constructors), Glaze auto-reflects
member names via C++20 reflection. No `glz::meta` specialization is required.

```cpp
struct Config {
    int width  = 1920;
    int height = 1080;
    bool fullscreen = false;
};

// Write to string
auto result = glz::write_json(cfg);          // returns glz::expected<std::string, glz::error_ctx>
std::string json = result.value();           // {"width":1920,"height":1080,"fullscreen":false}

// Read from string
Config cfg2{};
glz::error_ctx ec = glz::read_json(cfg2, json);
if (ec) {
    // handle error
}
```

---

## 3. Custom Key Names via `glz::meta`

Use `glz::meta` when you need custom JSON keys, rename fields, or exclude members.

```cpp
struct Player {
    std::string name;
    int         hp   = 100;
    float       mana = 50.f;
};

template <>
struct glz::meta<Player> {
    using T = Player;
    static constexpr auto value = glz::object(
        "name", &T::name,
        "hp",   &T::hp,
        "mana", &T::mana
    );
};
```

### Rename a field

```cpp
// JSON key "health_points" → C++ member hp
"health_points", &T::hp,
```

### Exclude a field from JSON

Wrap with `glz::hide`:

```cpp
static constexpr auto value = glz::object(
    "name", &T::name,
    "internal_id", glz::hide{&T::id}  // never written or read
);
```

---

## 4. Inner `glaze` struct (alternative to external `glz::meta`)

Define metadata inside the struct itself:

```cpp
struct Settings {
    int volume = 80;
    bool muted = false;

    struct glaze {
        using T = Settings;
        static constexpr auto value = glz::object(
            "volume", &T::volume,
            "muted",  &T::muted
        );
    };
};
```

---

## 5. Write JSON

### To `std::string` (allocated)

```cpp
auto res = glz::write_json(value);
// res is glz::expected<std::string, glz::error_ctx>
if (!res) { /* error */ }
std::string json = std::move(res.value());
```

### To existing buffer (non-allocating)

```cpp
std::string buf;
glz::error_ctx ec = glz::write_json(value, buf);
if (ec) { /* error */ }
```

### To file

```cpp
std::string buf;
glz::error_ctx ec = glz::write_file_json(value, "config.json", buf);
```

### Prettified output

```cpp
// Option A: write with opts
auto json = glz::write<glz::opts{.prettify = true}>(value).value_or("{}");

// Option B: prettify an existing JSON string
std::string pretty = glz::prettify_json(existing_json_str);
```

### Minified output

```cpp
std::string minified = glz::minify_json(existing_json_str);
```

---

## 6. Read JSON

### From `std::string` or `std::string_view`

```cpp
MyStruct obj{};
glz::error_ctx ec = glz::read_json(obj, json_string);
if (ec) {
    std::string msg = glz::format_error(ec, json_string);
    // log/report msg
}
```

### Return-value form (glz::expected)

```cpp
auto result = glz::read_json<MyStruct>(json_string);
if (!result) {
    std::string msg = glz::format_error(result, json_string);
} else {
    MyStruct obj = std::move(result.value());
}
```

### From file

```cpp
MyStruct obj{};
std::string buf;
glz::error_ctx ec = glz::read_file_json(obj, "config.json", buf);
```

### JSONC (with `// comments`)

```cpp
glz::error_ctx ec = glz::read_jsonc(obj, json_with_comments);
// or from file:
glz::error_ctx ec2 = glz::read_file_jsonc(obj, "config.jsonc", buf);
```

---

## 7. Custom Options (`glz::opts`)

Override defaults by passing a `constexpr glz::opts` struct:

```cpp
constexpr glz::opts my_opts {
    .prettify              = true,   // human-readable output
    .skip_null_members     = false,  // write null fields too
    .error_on_unknown_keys = false,  // ignore unknown JSON keys
    .error_on_missing_keys = true,   // require all keys to be present
};

auto json = glz::write<my_opts>(value);
glz::error_ctx ec = glz::read<my_opts>(obj, json_str);
```

Common `glz::opts` fields:

| Field | Default | Meaning |
|---|---|---|
| `prettify` | `false` | Indent output |
| `skip_null_members` | `true` | Skip null/default fields on write |
| `error_on_unknown_keys` | `true` | Error on unknown JSON key |
| `error_on_missing_keys` | `false` | Error on missing required key |
| `comments` | `false` | Allow JSONC-style `/* */` comments |
| `minified` | `false` | Expect minified input for faster reads |

---

## 8. Error Handling

```cpp
glz::error_ctx ec = glz::read_json(obj, buf);
if (ec) {
    // ec.ec is glz::error_code enum
    // format_error produces a human-readable diagnostic string
    std::string diag = glz::format_error(ec, buf);
}
```

`glz::expected<T, glz::error_ctx>` — same as `std::expected`, use `.value()` / `.error()` / `operator bool`.

---

## 9. Partial Write (selected fields only)

```cpp
// Write only specific keys at runtime
std::vector<std::string> keys = {"name", "hp"};
std::string buf;
glz::error_ctx ec = glz::write_json_partial(player, keys, buf);
```

---

## 10. Generic JSON (schema-less / dynamic)

```cpp
#include "glaze/json/generic.hpp"

glz::generic_json<> doc{};  // default: numbers as double
auto ec = glz::read_json(doc, raw_json);

// Access values
auto& obj = doc.get<glz::generic_json<>::object_t>();
auto& val = obj["key"];
if (val.holds<double>()) {
    double n = val.get<double>();
}
if (val.holds<std::string>()) {
    std::string s = val.get<std::string>();
}

// Write back
auto out = doc.dump();  // returns glz::expected<std::string, glz::error_ctx>
```

For precise integer handling:

```cpp
glz::generic_json<glz::num_mode::i64> doc{};  // integers stored as int64_t
```

---

## 11. Enum Serialization

Enums serialize as **strings** by default when registered:

```cpp
enum class Color { Red, Green, Blue };

template <>
struct glz::meta<Color> {
    static constexpr auto value = glz::enumerate(
        "Red",   Color::Red,
        "Green", Color::Green,
        "Blue",  Color::Blue
    );
};
```

---

## 12. STL Type Support (automatic)

| C++ type | JSON |
|---|---|
| `bool` | `true` / `false` |
| `int`, `float`, `double`, … | number |
| `std::string` | string |
| `std::optional<T>` | value or `null` |
| `std::vector<T>` | array |
| `std::array<T,N>` | array |
| `std::map<K,V>` / `std::unordered_map` | object |
| `std::tuple<…>` | array |
| `std::variant<…>` | typed object via `"type"` key |
| `std::chrono` durations | numeric count |

---

## 13. Common Patterns

### Struct → JSON string

```cpp
std::string s = glz::write_json(obj).value_or("{}");
```

### JSON string → struct (with error logging)

```cpp
MyStruct obj{};
if (auto ec = glz::read_json(obj, s); ec) {
    // log glz::format_error(ec, s)
}
```

### JSON file round-trip

```cpp
// Load
std::string buf;
glz::read_file_json(cfg, "config.json", buf);

// Save
glz::write_file_json(cfg, "config.json", buf);
```

### Pretty-print any JSON string

```cpp
std::string pretty = glz::prettify_json(compact_json);
```

---

## Notes

- Glaze uses **SWAR / SIMD** intrinsics internally for fast parsing — avoid manual padding of buffers.
- `write_padding_bytes = 256` — Glaze pre-allocates this overhead automatically for resizable buffers.
- For raw `char[]` buffers (fixed size): pass raw pointer overloads.
- Do **not** mix `glz::meta` and inner `glaze` struct — use one or the other per type.
- `glz::meta` specializations must be in the **global namespace**.
