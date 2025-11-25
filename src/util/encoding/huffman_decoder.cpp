#include "huffman_decoder.h"

namespace shine::util
{
	namespace
	{
		constexpr uint32_t FIRSTBITS = 9;
		constexpr uint32_t INVALIDSYMBOL = 0xFFFF;
	}

	uint32_t huffmanDecodeSymbol(BitReader& reader, const HuffmanTree& tree)
	{
		// 优化：确保有足够的位用于第一层查找（最多15位）
		reader.ensureBits(FIRSTBITS);
		uint32_t code = reader.peekBits(FIRSTBITS);
		if (code >= tree.table_len.size())
		{
			return INVALIDSYMBOL;
		}

		uint8_t l = tree.table_len[code];
		uint16_t value = tree.table_value[code];

		if (l <= FIRSTBITS)
		{
			reader.advanceBits(l);
			return value;
		}
		else
		{
			reader.advanceBits(FIRSTBITS);
			// 优化：确保有足够的位用于第二层查找（最多15位）
			reader.ensureBits(l - FIRSTBITS);
			uint32_t code2 = reader.peekBits(l - FIRSTBITS);
			uint32_t index2 = value + code2;

			if (index2 >= tree.table_len.size())
			{
				return INVALIDSYMBOL;
			}

			uint8_t l2 = tree.table_len[index2];
			reader.advanceBits(l2 - FIRSTBITS);
			return tree.table_value[index2];
		}
	}

} // namespace shine::util

