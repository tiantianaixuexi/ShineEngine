#pragma once

#include "shine_define.h"

#ifdef SHINE_PLATFORM_WASM

namespace shine::wasm
{
	template<typename T>
	class SArray
	{
		u32				length  = 0;		  //总的数据长度
		void*		    pointer = nullptr;    //连续的内存数据结构

	public:

		SArray() noexcept
		{
			length = 0;
			pointer = nullptr;

		}

		explicit  SArray(int counter) noexcept
		{
			if (counter < 1) return;
			const _size = sizeof(T);

			if (_size == 0) return;

			length = counter;
			pointer = malloc((size_t)(length * _size));
		}

		virtual ~SArray()
		{
			if (pointer != nullptr)
			{
				free(pointer);
				pointer = nullptr;
			}
		}

		inline T* Value(const u32 index) noexcept
		{
			return &static_cast<T*>(pointer)[index];
		}


		inline void clear() noexcept
		{
			memset(this, 0, sizeof(HashArray) + length * sizeof(T));
		}
	};

}






#endif