#pragma once

#include <cstdint>
#include <vector>
#include <expected>
#include <string>


namespace shine::util
{
	/**
	 * @brief 通用 Huffman 树结构
	 * 
	 * 用于 PNG (Deflate/Zlib) 和 WebP (VP8L) 等格式的 Huffman 解码
	 */
	struct HuffmanTree
	{
		std::vector<uint32_t> codes;      ///< Huffman 码
		std::vector<uint32_t> lengths;    ///< 码长度
		std::vector<uint8_t> table_len;   ///< 查找表：长度
		std::vector<uint16_t> table_value; ///< 查找表：值
		uint32_t maxbitlen = 0;           ///< 最大码长度
		uint32_t numcodes = 0;            ///< 符号数量

		/**
		 * @brief 初始化 Huffman 树
		 * @param numcodes_val 符号数量
		 * @param maxbitlen_val 最大码长度
		 */
		void init(uint32_t numcodes_val, uint32_t maxbitlen_val)
		{
			numcodes = numcodes_val;
			maxbitlen = maxbitlen_val;
			codes.clear();
			lengths.clear();
			table_len.clear();
			table_value.clear();
		}
	};

	/**
	 * @brief 从码长度构建 Huffman 树
	 * 
	 * 使用标准的 Canonical Huffman 编码算法
	 * 
	 * @param bitlen 每个符号的码长度数组
	 * @param numcodes 符号数量
	 * @param maxbitlen 最大码长度（通常为 15）
	 * @return Huffman 树，失败返回错误信息
	 */
	std::expected<HuffmanTree, std::string> buildHuffmanTree(
		const uint32_t* bitlen, size_t numcodes, uint32_t maxbitlen);

	/**
	 * @brief 反转位顺序（用于 Huffman 解码）
	 * @param bits 要反转的位
	 * @param num 位数
	 * @return 反转后的位
	 */
	constexpr uint32_t reverseBits(uint32_t bits, uint32_t num) noexcept
	{
		uint32_t result = 0;
		for (uint32_t i = 0; i < num; ++i)
		{
			result |= ((bits >> (num - i - 1)) & 1) << i;
		}
		return result;
	}

} // namespace shine::util

