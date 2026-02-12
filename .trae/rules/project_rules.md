# Shine String Usage

- use c++23 and c23 standard , msvc

- Prefer `shine::SString` over `std::string` for general string manipulation to benefit from SSO and other optimizations.
- Use `shine::SString` for UI text, file paths (convertible to `std::filesystem::path`), and internal logic.
