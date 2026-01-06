
#ifdef SHINE_USE_MODULE

module;

#include "shine_define.h"

export module shine.byte_convert;

import <string>;
import <string_view>;
import <bit>;

#else

#pragma once

#include "shine_define.h"

#include <string>
#include <string_view>
#include <bit>

#endif


namespace shine::util
{

	SHINE_MODULE_EXPORT enum class Endian
	{
		Native,
		Big,
		Little
	};

	// 字节单位
	SHINE_MODULE_EXPORT enum class  SizeUnit
	{
		Byte,
		KB,
		MB,
		GB,
		TB,
		PB
	};

	SHINE_MODULE_EXPORT struct  SizeUnitInfo
	{

		double size;
		SizeUnit unit;

		SizeUnitInfo() = default;
		SizeUnitInfo(double _size, SizeUnit _unit) :size(_size), unit(_unit) {
		}
	};



	/**
	 * 将指定大小从一个单位转换到另一个单位。
	 *
	 * 参数:
	 *  - size: 要转换的数值（以最初单位为准，使用 uint64_t 以支持较大字节值）。
	 *  - from: 输入数值的单位（SizeUnit）。
	 *  - to:   目标单位（SizeUnit）。
	 *
	 * 返回:
	 *  - 转换后的 double 值。
	 */
	SHINE_MODULE_EXPORT constexpr double convert_size(s64 size, SizeUnit from, SizeUnit to) noexcept;


	/**
	 * 从原始字节缓冲区创建一个只读字符串视图（不拷贝）。
	 *
	 * @param data       指向原始字节缓冲区的指针（只读）。
	 * @param data_size  缓冲区总字节数。
	 * @param offset     起始字节偏移，超出范围将返回空视图。
	 * @param length     视图长度；为 0 表示直到缓冲区末尾（越界自动截断）。
	 *
	 * @return           指向指定区段的 std::string_view（不拥有内存）。
	 */
	SHINE_MODULE_EXPORT constexpr std::string_view bytes_to_string(const unsigned char* data, size_t data_size, size_t offset = 0, size_t length = 0);
	SHINE_MODULE_EXPORT constexpr std::string_view bytes_to_string(const char* data, size_t data_size, size_t offset = 0, size_t length = 0);

	/**
	 * 将字节数格式化为可读的文件大小字符串（例如 "1.23 MB"）。
	 *
	 * @param size_in_bytes       字节数（uint64_t。
	 * @param precision  保留的小数位数（默认 2）。
	 *
	 * @return           格式化后的 std::string，包含数值和单位。
	 */
	SHINE_MODULE_EXPORT std::string  format_file_size(u64 size_in_bytes, int precision = 2) noexcept;


	/**
	 * 将 SizeUnit 枚举转换为对应的字符串视图（例如 SizeUnit::KB -> "KB"）。
	 *
	 * 返回的 string_view 指向内部常量数据，调用者无需释放。
	 */
	SHINE_MODULE_EXPORT std::string_view unit_to_string(SizeUnit unit) noexcept;



	namespace detail
	{


		inline bool can_read(size_t size,
			size_t offset,
			size_t bytes) noexcept
		{
			return offset + bytes <= size;
		}

	}

	template<typename T>
	T read(const unsigned char* data,
		size_t size,
		size_t offset,
		std::endian data_endian = std::endian::native) noexcept
	{


		if (!detail::can_read(size, offset, sizeof(T)))
			return T{};

		T tmp{};
		memcpy(&tmp, data + offset, sizeof(T));

		if constexpr (sizeof(T) > 1)
		{
			if (data_endian != std::endian::native) {
				tmp = std::byteswap(tmp);
			}
		}

		return static_cast<T>(tmp);
	}

	// ===============================
	// Convenience wrappers
	// ===============================
	/**
	* 从缓冲区读取一个无符号 8 位整数（按大端读取，但对单字节值无字节序差异）。
	*/
	SHINE_MODULE_EXPORT inline u8 read_u8(const unsigned char* d, size_t sz, size_t o = 0) noexcept
	{
		return read<u8>(d, sz, o, std::endian::native);
	}

	SHINE_MODULE_EXPORT inline u8 read_s8(const unsigned char* d, size_t sz, size_t o = 0) noexcept
	{
		return read<s8>(d, sz, o, std::endian::native);
	}

	SHINE_MODULE_EXPORT inline u16 read_be16(const unsigned char* d, size_t sz, size_t o = 0) noexcept
	{
		return read<u16>(d, sz, o, std::endian::big);
	}


	SHINE_MODULE_EXPORT inline u32 read_be32(const unsigned char* d, size_t sz, size_t o = 0) noexcept
	{
		return read<u32>(d, sz, o, std::endian::big);
	}

	SHINE_MODULE_EXPORT inline u16 read_le16(const unsigned char* d, size_t sz, size_t o = 0) noexcept
	{
		return read<u16>(d, sz, o, std::endian::little);
	}

	SHINE_MODULE_EXPORT inline u32 read_le32(const unsigned char* d, size_t sz, size_t o = 0) noexcept
	{
		return read<u32>(d, sz, o, std::endian::little);
	}

	SHINE_MODULE_EXPORT inline u32 read_le24(const unsigned char* d, size_t sz, size_t o = 0) noexcept
	{
		if (!detail::can_read(sz, o, 3)) return 0;
		return static_cast<u32>(d[o]) | (static_cast<u32>(d[o + 1]) << 8) | (static_cast<u32>(d[o + 2]) << 16);
	}

	SHINE_MODULE_EXPORT inline u64 read_be64(const unsigned char* d, size_t sz, size_t o = 0) noexcept
	{
		return read<u64>(d, sz, o, std::endian::big);
	}

	SHINE_MODULE_EXPORT inline s64 read_le64(const unsigned char* d, size_t sz, size_t o = 0) noexcept
	{
		return read<s64>(d, sz, o, std::endian::little);
	}

}

