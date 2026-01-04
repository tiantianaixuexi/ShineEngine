#pragma once

#include "shine_define.h"
#include <compare>
#include <array>
#include <random>
#include <string>
#include <expected>

#include "timer/timer_util.h"

// 注意：本项目使用 C++23，所有 C++23 特性默认可用
// 只需针对 WASM 平台做特殊处理

namespace shine::util
{

	enum class ParseError { InvalidLength, InvalidChar, InvalidFormat };

	// 仿Unreal Engine FGuid 类的128-bit 标识符
	struct FGuid
	{
		u32 A{ 0 };
		u32 B{ 0 };
		u32 C{ 0 };
		u32 D{ 0 };

		// 构造
		constexpr FGuid() = default;
		constexpr FGuid(u32 a, u32 b, u32 c, u32 d) noexcept
			: A(a), B(b), C(c), D(d) {
		}

		// 验证
		[[nodiscard]] constexpr bool IsValid() const noexcept
		{
			return (A | B | C | D) != 0u;
		}

		void Invalidate() noexcept { A = B = C = D = 0u; }

		// c++23  显式对象参数 (Explicit Object Parameter) - CWG 2586
		//[[nodiscard]] std::strong_ordering operator<=>(this FGuid const& self, FGuid const& other) const noexcept = default;
		//[[nodiscard]] bool operator==(this FGuid const& self, FGuid const& other) const noexcept = default;
		 //比较
		[[nodiscard]] std::strong_ordering operator<=>(const FGuid& other) const noexcept
		{
			if (A != other.A) return A < other.A ? std::strong_ordering::less : std::strong_ordering::greater;
			if (B != other.B) return B < other.B ? std::strong_ordering::less : std::strong_ordering::greater;
			if (C != other.C) return C < other.C ? std::strong_ordering::less : std::strong_ordering::greater;
			if (D != other.D) return D < other.D ? std::strong_ordering::less : std::strong_ordering::greater;
			return std::strong_ordering::equal;
		}

		[[nodiscard]] bool operator==(const FGuid& other) const noexcept
		{
			return A == other.A && B == other.B && C == other.C && D == other.D;
		}


		[[nodiscard]] std::array<u8, 16> ToBytes() const noexcept
		{
			return {
				static_cast<u8>((A >> 24) & 0xFF), static_cast<u8>((A >> 16) & 0xFF),
				static_cast<u8>((A >> 8) & 0xFF),  static_cast<u8>(A & 0xFF),
				static_cast<u8>((B >> 24) & 0xFF), static_cast<u8>((B >> 16) & 0xFF),
				static_cast<u8>((B >> 8) & 0xFF),  static_cast<u8>(B & 0xFF),
				static_cast<u8>((C >> 24) & 0xFF), static_cast<u8>((C >> 16) & 0xFF),
				static_cast<u8>((C >> 8) & 0xFF),  static_cast<u8>(C & 0xFF),
				static_cast<u8>((D >> 24) & 0xFF), static_cast<u8>((D >> 16) & 0xFF),
				static_cast<u8>((D >> 8) & 0xFF),  static_cast<u8>(D & 0xFF)
			};
		}


		// 重载：接受指针和大小（WASM 兼容）
		static FGuid FromBytes(const std::uint8_t* b) noexcept
		{
			FGuid g{};
			g.A = (static_cast<u32>(b[0]) << 24) | (static_cast<u32>(b[1]) << 16) | (static_cast<u32>(b[2]) << 8) | b[3];
			g.B = (static_cast<u32>(b[4]) << 24) | (static_cast<u32>(b[5]) << 16) | (static_cast<u32>(b[6]) << 8) | b[7];
			g.C = (static_cast<u32>(b[8]) << 24) | (static_cast<u32>(b[9]) << 16) | (static_cast<u32>(b[10]) << 8) | b[11];
			g.D = (static_cast<u32>(b[12]) << 24) | (static_cast<u32>(b[13]) << 16) | (static_cast<u32>(b[14]) << 8) | b[15];
			return g;
		}

		// GUID 字符串：xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx（可选大写字母）
		[[nodiscard]] std::string ToString(bool withBraces = false, bool uppercase = false) const
		{
			static constexpr char lower[16] = { '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f' };
			static constexpr char upper[16] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' };
			const char* tbl = uppercase ? upper : lower;

			const auto bytes = ToBytes();
			std::string s;
			s.resize(withBraces ? 38 : 36);

			size_t pos = 0;
			if (withBraces) s[pos++] = '{';

			auto push2 = [&](int i) {
				s[pos++] = tbl[(bytes[i] >> 4) & 0xF];
				s[pos++] = tbl[bytes[i] & 0xF];
				};

			for (int i = 0; i < 4; ++i) push2(i);
			s[pos++] = '-';
			for (int i = 4; i < 6; ++i) push2(i);
			s[pos++] = '-';
			for (int i = 6; i < 8; ++i) push2(i);
			s[pos++] = '-';
			for (int i = 8; i < 10; ++i) push2(i);
			s[pos++] = '-';
			for (int i = 10; i < 16; ++i) push2(i);

			if (withBraces) s[pos] = '}';
			return s;
		}



#ifdef SHINE_PLATFORM_WASM
		// WASM 兼容：使用 std::optional + ParseError 输出参数
		struct ParseResult {
			std::optional<FGuid> value;
			ParseError error = ParseError::InvalidLength;
			bool has_value() const noexcept { return value.has_value(); }
			FGuid operator*() const { return *value; }
		};

		static ParseResult Parse(SString text)
		{
			ParseResult result;

			size_t startPos = 0;
			if (text.size() == 38 && text.front() == '{' && text.back() == '}') {
				startPos = 1;
			}
			if (text.size() != 36 + (startPos > 0 ? 2 : 0)) {
				result.error = ParseError::InvalidLength;
				return result;
			}
			const char* data = text.data() + startPos;

			if (data[8] != '-' || data[13] != '-' || data[18] != '-' || data[23] != '-') {
				result.error = ParseError::InvalidFormat;
				return result;
			}

			std::array<std::uint8_t, 16> b{};
			int idx = 0;

			auto hexVal = [](const char c) noexcept ->int {
				if (c >= '0' && c <= '9') return c - '0';
				if (c >= 'a' && c <= 'f') return c - 'a' + 10;
				if (c >= 'A' && c <= 'F') return c - 'A' + 10;
				return -1;
				};

			for (int i = 0; i < 8; i += 2) {
				int h = hexVal(data[i]), l = hexVal(data[i + 1]);
				if (h < 0 || l < 0) { result.error = ParseError::InvalidChar; return result; }
				b[idx++] = static_cast<std::uint8_t>((h << 4) | l);
			}
			for (int i = 9; i < 13; i += 2) {
				int h = hexVal(data[i]), l = hexVal(data[i + 1]);
				if (h < 0 || l < 0) { result.error = ParseError::InvalidChar; return result; }
				b[idx++] = static_cast<std::uint8_t>((h << 4) | l);
			}
			for (int i = 14; i < 18; i += 2) {
				int h = hexVal(data[i]), l = hexVal(data[i + 1]);
				if (h < 0 || l < 0) { result.error = ParseError::InvalidChar; return result; }
				b[idx++] = static_cast<std::uint8_t>((h << 4) | l);
			}
			for (int i = 19; i < 23; i += 2) {
				int h = hexVal(data[i]), l = hexVal(data[i + 1]);
				if (h < 0 || l < 0) { result.error = ParseError::InvalidChar; return result; }
				b[idx++] = static_cast<std::uint8_t>((h << 4) | l);
			}
			for (int i = 24; i < 36; i += 2) {
				int h = hexVal(data[i]), l = hexVal(data[i + 1]);
				if (h < 0 || l < 0) { result.error = ParseError::InvalidChar; return result; }
				b[idx++] = static_cast<std::uint8_t>((h << 4) | l);
			}

			result.value = FromBytes(b);
			return result;
		}
#else
		// C++23: 使用 std::expected
		static std::expected<FGuid, ParseError> Parse(std::string_view text)
		{
			size_t startPos = 0;
			if (text.size() == 38 && text.front() == '{' && text.back() == '}') {
				startPos = 1;
			}
			if (text.size() != 36 + (startPos > 0 ? 2 : 0)) {
				return std::unexpected(ParseError::InvalidLength);
			}
			const char* data = text.data() + startPos;

			if (data[8] != '-' || data[13] != '-' || data[18] != '-' || data[23] != '-')
				return std::unexpected(ParseError::InvalidFormat);

			std::array<u8, 16> b{};
			int idx = 0;

			auto hexVal = [](const char c) noexcept ->int {
				if (c >= '0' && c <= '9') return c - '0';
				if (c >= 'a' && c <= 'f') return c - 'a' + 10;
				if (c >= 'A' && c <= 'F') return c - 'A' + 10;
				return -1;
				};

			for (int i = 0; i < 8; i += 2) {
				int h = hexVal(data[i]), l = hexVal(data[i + 1]);
				if (h < 0 || l < 0) return std::unexpected(ParseError::InvalidChar);
				b[idx++] = static_cast<u8>((h << 4) | l);
			}
			for (int i = 9; i < 13; i += 2) {
				int h = hexVal(data[i]), l = hexVal(data[i + 1]);
				if (h < 0 || l < 0) return std::unexpected(ParseError::InvalidChar);
				b[idx++] = static_cast<u8>((h << 4) | l);
			}
			for (int i = 14; i < 18; i += 2) {
				int h = hexVal(data[i]), l = hexVal(data[i + 1]);
				if (h < 0 || l < 0) return std::unexpected(ParseError::InvalidChar);
				b[idx++] = static_cast<u8>((h << 4) | l);
			}
			for (int i = 19; i < 23; i += 2) {
				int h = hexVal(data[i]), l = hexVal(data[i + 1]);
				if (h < 0 || l < 0) return std::unexpected(ParseError::InvalidChar);
				b[idx++] = static_cast<u8>((h << 4) | l);
			}
			for (int i = 24; i < 36; i += 2) {
				int h = hexVal(data[i]), l = hexVal(data[i + 1]);
				if (h < 0 || l < 0) return std::unexpected(ParseError::InvalidChar);
				b[idx++] = static_cast<u8>((h << 4) | l);
			}

			return FromBytes(b.data());
		}
#endif

		// 生成随机 GUID（符合v4/variant 标准）
		static FGuid NewGuid()
		{
			static std::atomic<u64> state{ 0 };

			const u64 now = get_now_ms_platform<u64>() & 0x0000FFFFFFFFFFFFull;

			u64 old = state.load(std::memory_order_relaxed);
			u64 next;
			do
			{
				const u64 last_ms = old >> 16;
				const u64 counter = old & 0xFFFF;
				next = (now > last_ms)
					? (now << 16)
					: ((last_ms << 16) | ((counter + 1) & 0xFFFF));
			} while (!state.compare_exchange_weak(old, next, std::memory_order_release, std::memory_order_relaxed));

			const u64 ts = next >> 16;
			const u16 seq = static_cast<u16>(next);

			const u32 A = static_cast<u32>(ts >> 16);
			const u32 B = (static_cast<u32>(ts & 0xFFFF) << 16) | (0x7000u | (seq >> 4));
			const u32 C = (0x80000000u) | ((seq & 0x000F) << 16) | static_cast<u32>((next >> 32) & 0xFFFF);
			const u32 D = static_cast<u32>(next ^ (next >> 32));

			return FGuid{ A, B, C, D };
		}
	};
}

// 哈希支持
namespace std
{
	template<>
	struct hash<shine::util::FGuid>
	{
		size_t operator()(const shine::util::FGuid& g) const noexcept
		{
			// 简单地将两个32-bit 合并
			u64 h1 = (static_cast<u64>(g.A) << 32) | g.B;
			u64 h2 = (static_cast<u64>(g.C) << 32) | g.D;
			// 64-bit 合并
			h1 ^= (h2 + 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2));
			return h1 ^ (h1 >> 32);
		}
	};
}

