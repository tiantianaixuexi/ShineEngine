#ifndef SHINE_WASM_SVECTOR_H
#define SHINE_WASM_SVECTOR_H

#include "../util/wasm_compat.h"

namespace shine::wasm
{

	template<typename T>
	class SVector
	{
		static_assert(__is_trivially_copyable(T), "SVector<T> requires trivially copyable T");
		static_assert(__is_trivially_destructible(T), "SVector<T> requires trivially destructible T");

		unsigned int length = 0;
		unsigned int cap = 0;
		void* pointer = nullptr;

		static inline unsigned int grow_cap(unsigned int cur, unsigned int need) noexcept
		{
			unsigned int n = cur ? cur : 8u;
			while (n < need) n *= 2u;
			return n;
		}

	public:
		SVector() noexcept = default;

		explicit SVector(unsigned int initialCap) noexcept
		{
			reserve(initialCap);
		}

		// Non-copyable (avoids double-free); movable.
		SVector(const SVector&) = delete;
		SVector& operator=(const SVector&) = delete;

		SVector(SVector&& other) noexcept
		{
			length = other.length;
			cap = other.cap;
			pointer = other.pointer;
			other.length = 0;
			other.cap = 0;
			other.pointer = nullptr;
		}

		SVector& operator=(SVector&& other) noexcept
		{
			if (this == &other) return *this;
			reset();
			length = other.length;
			cap = other.cap;
			pointer = other.pointer;
			other.length = 0;
			other.cap = 0;
			other.pointer = nullptr;
			return *this;
		}

		~SVector()
		{
			if (pointer != nullptr)
			{
				shine::wasm::raw_free(pointer);
				pointer = nullptr;
			}
			length = 0;
			cap = 0;
		}

		inline T* data() noexcept { return static_cast<T*>(pointer); }
		inline const T* data() const noexcept { return static_cast<const T*>(pointer); }

		inline unsigned int size() const noexcept { return length; }
		inline unsigned int capacity() const noexcept { return cap; }
		inline bool empty() const noexcept { return length == 0u; }

		inline T& operator[](unsigned int i) noexcept { return static_cast<T*>(pointer)[i]; }
		inline const T& operator[](unsigned int i) const noexcept { return static_cast<const T*>(pointer)[i]; }

		inline T& back() noexcept { return static_cast<T*>(pointer)[length - 1u]; }
		inline const T& back() const noexcept { return static_cast<const T*>(pointer)[length - 1u]; }

		inline T* begin() noexcept { return data(); }
		inline T* end() noexcept { return data() + length; }
		inline const T* begin() const noexcept { return data(); }
		inline const T* end() const noexcept { return data() + length; }

		inline void clear() noexcept { length = 0; }

		inline void reset() noexcept
		{
			if (pointer != nullptr)
			{
				shine::wasm::raw_free(pointer);
				pointer = nullptr;
			}
			length = 0;
			cap = 0;
		}

		inline void reserve(unsigned int newCap) noexcept
		{
			shine::wasm::svector_reserve_impl(&pointer, &cap, length, newCap, sizeof(T));
		}

		inline void resize(unsigned int newSize) noexcept
		{
			if (newSize <= length) { length = newSize; return; }
			if (newSize > cap) reserve(grow_cap(cap, newSize));

			// Value-initialize new tail.
			T* p = data();
			for (unsigned int i = length; i < newSize; ++i) p[i] = T{};
			length = newSize;
		}

		// Like resize(), but does NOT initialize new tail.
		// Use only when caller will fill all newly exposed elements.
		inline void resize_uninitialized(unsigned int newSize) noexcept
		{
			if (newSize <= length) { length = newSize; return; }
			if (newSize > cap) reserve(grow_cap(cap, newSize));
			length = newSize;
		}

		inline void push_back(const T& v) noexcept
		{
			const unsigned int n = length + 1u;
			if (n > cap) reserve(grow_cap(cap, n));
			static_cast<T*>(pointer)[length] = v;
			length = n;
		}

		template<typename... Args>
		inline void emplace_back(Args&&... args) noexcept
		{
			T tmp((Args&&)args...);
			push_back(tmp);
		}

		inline void pop_back() noexcept
		{
			if (length == 0u) return;
			length -= 1u;
		}

		// Unordered erase: swap-with-last then pop.
		inline bool erase_unordered_at(unsigned int idx) noexcept
		{
			if (idx >= length) return false;
			if (length > 1u && idx != (length - 1u))
			{
				static_cast<T*>(pointer)[idx] = static_cast<T*>(pointer)[length - 1u];
			}
			length -= 1u;
			return true;
		}

		inline bool erase_first_unordered(const T& v) noexcept
		{
			for (unsigned int i = 0; i < length; ++i)
			{
				if (static_cast<T*>(pointer)[i] == v) return erase_unordered_at(i);
			}
			return false;
		}
	};
}

#endif // SHINE_WASM_SVECTOR_H
