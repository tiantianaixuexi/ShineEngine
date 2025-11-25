#include "huffman_tree.h"

#include <algorithm>
#include <cstring>

namespace shine::util
{
	namespace
	{
		constexpr uint32_t FIRSTBITS = 9;        // 第一层查找表的位数
		constexpr uint32_t INVALIDSYMBOL = 0xFFFF; // 无效符号标记
	}

	std::expected<HuffmanTree, std::string> buildHuffmanTree(
		const uint32_t* bitlen, size_t numcodes, uint32_t maxbitlen)
	{
		HuffmanTree tree;
		tree.init(static_cast<uint32_t>(numcodes), maxbitlen);

		// 优化：一次性分配并复制长度
		tree.codes.resize(numcodes);
		tree.lengths.assign(bitlen, bitlen + numcodes);

		// 步骤1：统计每个长度的数量
		std::vector<uint32_t> blcount(maxbitlen + 1, 0);
		std::vector<uint32_t> nextcode(maxbitlen + 1, 0);

		for (size_t i = 0; i < numcodes; ++i)
		{
			if (tree.lengths[i] <= maxbitlen)
			{
				++blcount[tree.lengths[i]];
			}
		}

		// 步骤2：生成下一个码值
		uint32_t code = 0;
		for (uint32_t bits = 1; bits <= maxbitlen; ++bits)
		{
			code = (code + blcount[bits - 1]) << 1;
			nextcode[bits] = code;
		}

		// 步骤3：生成所有码
		for (size_t n = 0; n < numcodes; ++n)
		{
			if (tree.lengths[n] != 0)
			{
				tree.codes[n] = nextcode[tree.lengths[n]]++;
				tree.codes[n] &= ((1u << tree.lengths[n]) - 1u);
			}
		}

		// 步骤4：构建查找表
		constexpr uint32_t headsize = 1u << FIRSTBITS;
		constexpr uint32_t mask = headsize - 1u;

		// 优化：计算最大长度并预分配查找表空间
		std::vector<uint32_t> maxlens(headsize, 0);
		for (size_t i = 0; i < numcodes; ++i)
		{
			uint32_t l = tree.lengths[i];
			if (l <= FIRSTBITS) continue;

			uint32_t symbol = tree.codes[i];
			uint32_t index = reverseBits(symbol >> (l - FIRSTBITS), FIRSTBITS);
			if (index < headsize)
			{
				maxlens[index] = (std::max)(maxlens[index], l);
			}
		}

		// 计算总表大小并一次性分配
		size_t size = headsize;
		for (uint32_t i = 0; i < headsize; ++i)
		{
			uint32_t l = maxlens[i];
			if (l > FIRSTBITS)
			{
				size += (1u << (l - FIRSTBITS));
			}
		}

		// 优化：一次性分配所有空间，避免多次resize
		tree.table_len.resize(size, 16); // 16 表示未使用
		tree.table_value.resize(size, INVALIDSYMBOL);

		// 填充第一层表（长符号）
		size_t pointer = headsize;
		for (uint32_t i = 0; i < headsize; ++i)
		{
			uint32_t l = maxlens[i];
			if (l > FIRSTBITS)
			{
				tree.table_len[i] = static_cast<uint8_t>(l);
				tree.table_value[i] = static_cast<uint16_t>(pointer);
				pointer += (1u << (l - FIRSTBITS));
			}
		}

		// 填充符号
		for (size_t i = 0; i < numcodes; ++i)
		{
			uint32_t l = tree.lengths[i];
			if (l == 0) continue;

			uint32_t symbol = tree.codes[i];
			uint32_t reverse = reverseBits(symbol, l);

			if (l <= FIRSTBITS)
			{
				// 短符号，完全在第一层表中
				uint32_t num = 1u << (FIRSTBITS - l);
				for (uint32_t j = 0; j < num; ++j)
				{
					uint32_t index = reverse | (j << l);
					if (index < headsize && tree.table_len[index] == 16)
					{
						tree.table_len[index] = static_cast<uint8_t>(l);
						tree.table_value[index] = static_cast<uint16_t>(i);
					}
				}
			}
			else
			{
				// 长符号，需要第二层查找
				uint32_t index = reverse & mask;
				if (index < headsize)
				{
					uint32_t maxlen = tree.table_len[index];
					if (maxlen >= l)
					{
						uint32_t tablelen = maxlen - FIRSTBITS;
						uint32_t start = tree.table_value[index];
						uint32_t num = 1u << (tablelen - (l - FIRSTBITS));

						for (uint32_t j = 0; j < num; ++j)
						{
							uint32_t reverse2 = reverse >> FIRSTBITS;
							uint32_t index2 = start + (reverse2 | (j << (l - FIRSTBITS)));
							if (index2 < size)
							{
								tree.table_len[index2] = static_cast<uint8_t>(l);
								tree.table_value[index2] = static_cast<uint16_t>(i);
							}
						}
					}
				}
			}
		}

		// 填充无效符号
		for (size_t i = 0; i < size; ++i)
		{
			if (tree.table_len[i] == 16)
			{
				tree.table_len[i] = (i < headsize) ? 1 : (FIRSTBITS + 1);
				tree.table_value[i] = INVALIDSYMBOL;
			}
		}

		return tree;
	}

} // namespace shine::util

