#pragma once

#include "../util/wasm_compat.h"

namespace shine::wasm
{
	template<typename T>
	class SArray
	{
		unsigned int length = 0;
		void* pointer = nullptr;

	public:

		SArray() noexcept = default;

		explicit SArray(unsigned int counter) noexcept
		{
			if (counter < 1) return;
			const size_t _size = sizeof(T);

			if (_size == 0) return;

			length = counter;
			pointer = shine::wasm::raw_malloc((shine::wasm::size_t)(length * _size));
		}

		// Non-copyable (avoids double-free); movable.
		SArray(const SArray&) = delete;
		SArray& operator=(const SArray&) = delete;

		SArray(SArray&& other) noexcept
		{
			length = other.length;
			pointer = other.pointer;
			other.length = 0;
			other.pointer = nullptr;
		}

		SArray& operator=(SArray&& other) noexcept
		{
			if (this == &other) return *this;
			reset();
			length = other.length;
			pointer = other.pointer;
			other.length = 0;
			other.pointer = nullptr;
			return *this;
		}

		~SArray()
		{
			if (pointer != nullptr)
			{
				shine::wasm::raw_free(pointer);
				pointer = nullptr;
			}
		}

		inline T* Value(const unsigned int index) noexcept
		{
			return &static_cast<T*>(pointer)[index];
		}

		inline T* data() noexcept { return static_cast<T*>(pointer); }
		inline const T* data() const noexcept { return static_cast<const T*>(pointer); }
		inline unsigned int size() const noexcept { return length; }

		inline void reset() noexcept
		{
			if (pointer != nullptr)
			{
				shine::wasm::raw_free(pointer);
				pointer = nullptr;
			}
			length = 0;
		}

		// Zero-fill payload only (safe for POD/trivial T).
		inline void clear_zero() noexcept
		{
			if (!pointer || length == 0) return;
			shine::wasm::raw_memset(pointer, 0, (shine::wasm::size_t)length * (shine::wasm::size_t)sizeof(T));
		}
	};

}


