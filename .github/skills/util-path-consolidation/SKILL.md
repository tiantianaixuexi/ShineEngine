---
name: "util-path-consolidation"
description: "Consolidates path normalization and join logic in util. Invoke when adding or refactoring path handling across file_util, path_util, string_util, or asset path flows."
---

# Util Path Consolidation

## Goal

Unify path handling logic in `src/util` to avoid duplicate implementations across `file_util`, `path_util`, `string_util`, and `encoding/url_util`.

## Invoke When

- Adding new capabilities for path normalization, joining, or absolutization
- Modifying asset path key behavior
- Discovering duplicate implementations of `NormalizePath`/`JoinPath`/`combinePath`
- Finding that `get_file_extension`/`get_file_directory`/`get_file_stem`/`get_file_name` are still being used in `file_util` as string utilities

## Canonical Rules

1. Resource key paths must uniformly call `shine::util::normalize_asset_path`
2. Platform filesystem path conversions should preferentially call `shine::util::normalize_path`
3. Extension normalization should uniformly call `shine::util::StringUtil::NormalizeFileExtension`
4. Pure string splitting operations such as filename/directory extraction should go through `string_util` and not flow back to `file_util`
5. Do not re‑implement path normalization logic in business modules
6. Any change in path behavior must verify the stability of asset index keys

## API Quick Reference

| API | Location | Purpose |
|---|---|---|
| `normalize_asset_path(const std::string&)` | `src/util/path_util.h` | Normalize logical asset paths (`\\` → `/`, lower‑case, remove trailing `/`) |
| `normalize_path(const std::string&)` | `src/util/path_util.h` | Normalize platform‑specific path separators |
| `to_absolute_path(const std::string&, const std::string&)` | `src/util/path_util.h` | Convert relative path to absolute path |
| `StringUtil::NormalizeFileExtension(std::string_view)` | `src/util/string_util.ixx` | Normalize file extension (remove dot and convert to lower‑case) |
| `StringUtil::GetDirectory(std::string_view)` | `src/util/string_util.ixx` | Extract directory string from a path |
| `get_executable_directory()` | `src/util/path_util.h` | Obtain the executable directory |
| `get_script_path(const std::string&)` | `src/util/path_util.h` | Obtain the script path |

## API Usage Pattern

- Asset keys, cache keys, index keys: use only `normalize_asset_path`
- Filesystem access paths: first call `normalize_path`, then perform file I/O
- Extension checks: uniformly use `StringUtil::NormalizeFileExtension`
- Directory string extraction: uniformly use `StringUtil::GetDirectory`
- Avoid writing local `NormalizePath` implementations

## Checklist

- Check if new duplicate path utility functions are introduced
- Check if any pure‑string interfaces from `file_util` are flowing back
- Uniformly include `util/path_util.h` or `util/string_util.ixx`
- Ensure consistent behavior for path separators on Windows and non‑Windows systems