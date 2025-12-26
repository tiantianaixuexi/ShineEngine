#pragma once

#include "shine_define.h"


namespace shine::wasm
{
	template<typename T>
	class HashArray
	{
		u32				length  = 0;		  //总的数据长度
		void*		    pointer = nullptr;    //连续的内存数据结构

	public:

		HashArray() noexcept
		{
			length = 0;
			pointer = nullptr;

		}

		explicit  HashArray(int counter) noexcept
		{
			if (counter < 1) return;
			const size_t _size = sizeof(T);

			if (_size == 0) return;

			length = counter;
			pointer = malloc((length * _size));
		}

		virtual ~HashArray()
		{
			if (pointer != nullptr)
			{
				free(pointer);
				pointer = nullptr;
			}
		}

		inline T* value(const u32 index) noexcept
		{
			return &static_cast<T*>(pointer)[index];
		}


		inline void clear() noexcept
		{
			memset(this, 0, sizeof(HashArray) + length * sizeof(T));
		}
	};

}
