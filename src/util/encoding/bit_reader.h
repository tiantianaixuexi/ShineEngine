#pragma once

#include <cstdint>
#include <span>

namespace shine::util
{
	/**
	 * @brief 位读取器类，用于从字节流中按位读取数据
	 * 
	 * 主要用于 Deflate/Zlib 等需要位级别操作的压缩格式解码
	 */
	class BitReader
	{
	public:
		/**
		 * @brief 构造函数
		 * @param data 数据指针
		 * @param size 数据大小（字节）
		 */
		explicit BitReader(const uint8_t* data, size_t size) noexcept
			: data_(data), size_(size), bitPos_(0), buffer_(0)
		{
			bitsize_ = (size < (SIZE_MAX / 8)) ? size * 8 : SIZE_MAX;
		}

		/**
		 * @brief 从 span 构造
		 */
		explicit BitReader(std::span<const uint8_t> data) noexcept
			: BitReader(data.data(), data.size())
		{}

		/**
		 * @brief 确保有足够的位可用（最多32位）
		 * @param nbits 需要的位数
		 */
		void ensureBits(size_t nbits) noexcept
		{
			size_t start = bitPos_ >> 3;
			if (start + 4 < size_)
			{
				buffer_ = static_cast<uint32_t>(data_[start + 0]) |
				         (static_cast<uint32_t>(data_[start + 1]) << 8) |
				         (static_cast<uint32_t>(data_[start + 2]) << 16) |
				         (static_cast<uint32_t>(data_[start + 3]) << 24);
				buffer_ >>= (bitPos_ & 7);
				if (start + 4 < size_)
				{
					buffer_ |= (static_cast<uint32_t>(data_[start + 4]) << 24) << (8 - (bitPos_ & 7));
				}
			}
			else
			{
				buffer_ = 0;
				if (start + 0 < size_) buffer_ |= data_[start + 0];
				if (start + 1 < size_) buffer_ |= static_cast<uint32_t>(data_[start + 1]) << 8;
				if (start + 2 < size_) buffer_ |= static_cast<uint32_t>(data_[start + 2]) << 16;
				if (start + 3 < size_) buffer_ |= static_cast<uint32_t>(data_[start + 3]) << 24;
				buffer_ >>= (bitPos_ & 7);
			}
		}

		/**
		 * @brief 读取 nbits 位（最多31位）
		 * @param nbits 要读取的位数
		 * @return 读取的值
		 */
		uint32_t readBits(size_t nbits) noexcept
		{
			ensureBits(nbits);
			uint32_t result = buffer_ & ((1u << nbits) - 1u);
			buffer_ >>= nbits;
			bitPos_ += nbits;
			return result;
		}

		/**
		 * @brief 查看位但不推进指针
		 * @param nbits 要查看的位数
		 * @return 查看的值
		 */
		uint32_t peekBits(size_t nbits) noexcept
		{
			ensureBits(nbits);
			return buffer_ & ((1u << nbits) - 1u);
		}

		/**
		 * @brief 推进位指针
		 * @param nbits 要推进的位数
		 */
		void advanceBits(size_t nbits) noexcept
		{
			buffer_ >>= nbits;
			bitPos_ += nbits;
		}

		/**
		 * @brief 对齐到字节边界
		 */
		void alignToByte() noexcept
		{
			if ((bitPos_ & 7) != 0)
			{
				bitPos_ = (bitPos_ + 7) & ~7;
				buffer_ = 0;
			}
		}

		/**
		 * @brief 获取当前位位置
		 */
		size_t getBitPos() const noexcept { return bitPos_; }

		/**
		 * @brief 获取当前字节位置
		 */
		size_t getBytePos() const noexcept { return bitPos_ >> 3; }

		/**
		 * @brief 检查是否还有数据可读
		 */
		bool hasMoreData() const noexcept
		{
			return getBytePos() < size_;
		}

		/**
		 * @brief 获取剩余字节数
		 */
		size_t remainingBytes() const noexcept
		{
			size_t pos = getBytePos();
			return (pos < size_) ? (size_ - pos) : 0;
		}

	private:
		const uint8_t* data_;
		size_t size_;
		size_t bitsize_;
		size_t bitPos_;
		uint32_t buffer_;
	};
}

