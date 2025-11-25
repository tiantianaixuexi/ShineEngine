#include "memory_util.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

namespace shine::util
{
#ifdef __EMSCRIPTEN__
	namespace wasm
	{
		size_t getMemoryUsage() noexcept
		{
			// 使用 Emscripten 的 JavaScript API 获取当前堆内存使用量
			return EM_ASM_INT({
				return HEAP8.length;
			});
		}

		size_t getTotalMemory() noexcept
		{
			// 获取 WASM 总内存大小
			return EM_ASM_INT({
				return TOTAL_MEMORY;
			});
		}

		bool hasEnoughMemory(size_t required) noexcept
		{
			size_t total = getTotalMemory();
			size_t current = getMemoryUsage();
			
			// 检查可用内存是否足够
			// 预留一些安全余量（10%）
			size_t available = total - current;
			size_t safetyMargin = total / 10;
			
			return available >= (required + safetyMargin);
		}
	}
#endif

} // namespace shine::util

