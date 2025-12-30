#include "singleton.h"


namespace shine::util
{
	template<typename T>
	T& Singleton<T>::get() noexcept
	{
		return *s_instance;
	}

	template <typename T>
	void Singleton<T>::set_instance(T* ptr) noexcept
	{
		s_instance = ptr;
	}
}
