#pragma once

#include "huffman_tree.h"
#include "bit_reader.h"

namespace shine::util
{
	/**
	 * @brief Huffman 解码符号
	 * 
	 * @param reader 位读取器
	 * @param tree Huffman 树
	 * @return 解码的符号值，如果失败返回 INVALIDSYMBOL (0xFFFF)
	 */
	uint32_t huffmanDecodeSymbol(BitReader& reader, const HuffmanTree& tree);

} // namespace shine::util

