#pragma once

#include "shine_define.h"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <memory>
#include <functional>

namespace shine::util
{
	// ============================================================================
	// 内存统计信息
	// ============================================================================

	/**
	 * @brief 内存池统计信息
	 */
	struct MemoryPoolStats
	{
		size_t allocatedBytes = 0;      ///< 已分配字节数
		size_t peakBytes = 0;           ///< 峰值字节数
		size_t allocationCount = 0;     ///< 分配次数
		size_t deallocationCount = 0;   ///< 释放次数
		size_t activeAllocations = 0;   ///< 当前活跃分配数

		/**
		 * @brief 重置统计信息
		 */
		void reset() noexcept
		{
			allocatedBytes = 0;
			peakBytes = 0;
			allocationCount = 0;
			deallocationCount = 0;
			activeAllocations = 0;
		}
	};

	// ============================================================================
	// 内存分配器接口
	// ============================================================================

	/**
	 * @brief 内存分配器接口
	 * 
	 * 定义内存分配和释放的标准接口，可以自定义实现
	 */
	struct IMemoryAllocator
	{
		virtual ~IMemoryAllocator() = default;

		/**
		 * @brief 分配内存
		 * @param size 要分配的字节数
		 * @return 分配的内存指针，失败返回 nullptr
		 */
		virtual void* allocate(size_t size) = 0;

		/**
		 * @brief 释放内存
		 * @param ptr 要释放的内存指针
		 */
		virtual void deallocate(void* ptr) = 0;

		/**
		 * @brief 重新分配内存
		 * @param ptr 原内存指针
		 * @param oldSize 原大小
		 * @param newSize 新大小
		 * @return 新内存指针，失败返回 nullptr
		 */
		virtual void* reallocate(void* ptr, size_t oldSize, size_t newSize)
		{
			if (newSize == 0)
			{
				deallocate(ptr);
				return nullptr;
			}

			void* newPtr = allocate(newSize);
			if (newPtr && ptr)
			{
				std::memcpy(newPtr, ptr, oldSize < newSize ? oldSize : newSize);
				deallocate(ptr);
			}
			return newPtr;
		}
	};

	// ============================================================================
	// 标准内存分配器（使用 malloc/free）
	// ============================================================================

	/**
	 * @brief 标准内存分配器
	 * 
	 * 使用标准库的 malloc/free 进行内存分配
	 */
	class StandardAllocator : public IMemoryAllocator
	{
	public:
		void* allocate(size_t size) override
		{
			return std::malloc(size);
		}

		void deallocate(void* ptr) override
		{
			std::free(ptr);
		}

		void* reallocate(void* ptr, size_t oldSize, size_t newSize) override
		{
			return std::realloc(ptr, newSize);
		}
	};

	// ============================================================================
	// 命名内存池
	// ============================================================================

	/**
	 * @brief 命名内存池
	 * 
	 * 管理一个命名内存池，追踪该池的内存使用情况
	 */
	class NamedMemoryPool
	{
	public:
		/**
		 * @brief 构造函数
		 * @param name 内存池名称
		 * @param allocator 内存分配器（可选，默认使用标准分配器）
		 */
		explicit NamedMemoryPool(std::string_view name, 
			std::shared_ptr<IMemoryAllocator> allocator = nullptr)
			: poolName(name)
			, allocator_(allocator ? allocator : std::make_shared<StandardAllocator>())
		{
		}

		/**
		 * @brief 分配内存
		 * @param size 要分配的字节数
		 * @return 分配的内存指针，失败返回 nullptr
		 */
		void* allocate(size_t size)
		{
			void* ptr = allocator_->allocate(size);
			if (ptr)
			{
				std::lock_guard<std::mutex> lock(statsMutex_);
				stats_.allocatedBytes += size;
				stats_.allocationCount++;
				stats_.activeAllocations++;
				if (stats_.allocatedBytes > stats_.peakBytes)
				{
					stats_.peakBytes = stats_.allocatedBytes;
				}
			}
			return ptr;
		}

		/**
		 * @brief 释放内存
		 * @param ptr 要释放的内存指针
		 * @param size 释放的字节数（用于统计）
		 */
		void deallocate(void* ptr, size_t size)
		{
			if (ptr)
			{
				allocator_->deallocate(ptr);
				std::lock_guard<std::mutex> lock(statsMutex_);
				if (stats_.allocatedBytes >= size)
				{
					stats_.allocatedBytes -= size;
				}
				stats_.deallocationCount++;
				if (stats_.activeAllocations > 0)
				{
					stats_.activeAllocations--;
				}
			}
		}

		/**
		 * @brief 获取内存池名称
		 * @return 内存池名称
		 */
		std::string_view getName() const noexcept { return poolName; }

		/**
		 * @brief 获取统计信息
		 * @return 统计信息副本
		 */
		MemoryPoolStats getStats() const noexcept
		{
			std::lock_guard<std::mutex> lock(statsMutex_);
			return stats_;
		}

		/**
		 * @brief 重置统计信息
		 */
		void resetStats() noexcept
		{
			std::lock_guard<std::mutex> lock(statsMutex_);
			stats_.reset();
		}

	private:
		std::string poolName;                                    ///< 内存池名称
		std::shared_ptr<IMemoryAllocator> allocator_;            ///< 内存分配器
		mutable std::mutex statsMutex_;                          ///< 统计信息互斥锁
		MemoryPoolStats stats_;                                  ///< 统计信息
	};

	// ============================================================================
	// 内存池管理器（单例）
	// ============================================================================

	/**
	 * @brief 内存池管理器
	 * 
	 * 管理所有命名内存池，提供全局访问接口
	 */
	class MemoryPoolManager
	{
	public:
		/**
		 * @brief 获取单例实例
		 * @return 管理器实例引用
		 */
		static MemoryPoolManager& getInstance() noexcept
		{
			static MemoryPoolManager instance;
			return instance;
		}

		/**
		 * @brief 获取或创建内存池
		 * @param name 内存池名称
		 * @return 内存池的共享指针
		 */
		std::shared_ptr<NamedMemoryPool> getOrCreatePool(std::string_view name)
		{
			std::lock_guard<std::mutex> lock(poolsMutex_);
			auto it = pools_.find(std::string(name));
			if (it != pools_.end())
			{
				return it->second;
			}

			auto pool = std::make_shared<NamedMemoryPool>(name);
			pools_[std::string(name)] = pool;
			return pool;
		}

		/**
		 * @brief 获取内存池
		 * @param name 内存池名称
		 * @return 内存池的共享指针，不存在返回 nullptr
		 */
		std::shared_ptr<NamedMemoryPool> getPool(std::string_view name) const
		{
			std::lock_guard<std::mutex> lock(poolsMutex_);
			auto it = pools_.find(std::string(name));
			return (it != pools_.end()) ? it->second : nullptr;
		}

		/**
		 * @brief 移除内存池
		 * @param name 内存池名称
		 * @return true 如果成功移除
		 */
		bool removePool(std::string_view name)
		{
			std::lock_guard<std::mutex> lock(poolsMutex_);
			return pools_.erase(std::string(name)) > 0;
		}

		/**
		 * @brief 获取所有内存池的统计信息
		 * @return 内存池名称到统计信息的映射
		 */
		std::unordered_map<std::string, MemoryPoolStats> getAllStats() const
		{
			std::lock_guard<std::mutex> lock(poolsMutex_);
			std::unordered_map<std::string, MemoryPoolStats> result;
			for (const auto& [name, pool] : pools_)
			{
				result[name] = pool->getStats();
			}
			return result;
		}

		/**
		 * @brief 重置所有内存池的统计信息
		 */
		void resetAllStats()
		{
			std::lock_guard<std::mutex> lock(poolsMutex_);
			for (auto& [name, pool] : pools_)
			{
				pool->resetStats();
			}
		}

		/**
		 * @brief 获取总内存使用量
		 * @return 所有内存池的总使用量（字节）
		 */
		size_t getTotalMemoryUsage() const
		{
			std::lock_guard<std::mutex> lock(poolsMutex_);
			size_t total = 0;
			for (const auto& [name, pool] : pools_)
			{
				total += pool->getStats().allocatedBytes;
			}
			return total;
		}

	private:
		MemoryPoolManager() = default;
		~MemoryPoolManager() = default;
		MemoryPoolManager(const MemoryPoolManager&) = delete;
		MemoryPoolManager& operator=(const MemoryPoolManager&) = delete;

		mutable std::mutex poolsMutex_;                                    ///< 内存池映射互斥锁
		std::unordered_map<std::string, std::shared_ptr<NamedMemoryPool>> pools_;  ///< 内存池映射
	};

	// ============================================================================
	// RAII 内存指针（支持指定内存池）
	// ============================================================================

	/**
	 * @brief 内存指针 RAII 包装器
	 * 
	 * 自动管理内存，支持指定内存池进行统计
	 * 
	 * @tparam T 指针指向的类型
	 * 
	 * @example
	 * ```cpp
	 * // 使用命名内存池
	 * auto pool = MemoryPoolManager::getInstance().getOrCreatePool("加载内存使用量");
	 * MemoryPtr<uint8_t> ptr(pool->allocate(1024), 1024, pool);
	 * // 使用 ptr.get() 访问数据
	 * // 析构时自动释放并更新统计
	 * ```
	 */
	template<typename T>
	class MemoryPtr
	{
	public:
		/**
		 * @brief 默认构造函数，创建空指针
		 */
		MemoryPtr() noexcept 
			: ptr(nullptr), size_(0), pool_(nullptr) {}

		/**
		 * @brief 从原始指针构造
		 * @param p 原始指针（可以是 nullptr）
		 * @param size 内存大小（字节）
		 * @param pool 所属内存池（可选）
		 */
		MemoryPtr(T* p, size_t size, std::shared_ptr<NamedMemoryPool> pool = nullptr) noexcept
			: ptr(p), size_(size), pool_(pool) {}

		/**
		 * @brief 禁用拷贝构造
		 */
		MemoryPtr(const MemoryPtr&) = delete;

		/**
		 * @brief 禁用拷贝赋值
		 */
		MemoryPtr& operator=(const MemoryPtr&) = delete;

		/**
		 * @brief 移动构造函数
		 * @param other 要移动的对象
		 */
		MemoryPtr(MemoryPtr&& other) noexcept
			: ptr(other.ptr), size_(other.size_), pool_(std::move(other.pool_))
		{
			other.ptr = nullptr;
			other.size_ = 0;
		}

		/**
		 * @brief 移动赋值运算符
		 * @param other 要移动的对象
		 * @return 当前对象的引用
		 */
		MemoryPtr& operator=(MemoryPtr&& other) noexcept
		{
			if (this != &other)
			{
				reset();
				ptr = other.ptr;
				size_ = other.size_;
				pool_ = std::move(other.pool_);
				other.ptr = nullptr;
				other.size_ = 0;
			}
			return *this;
		}

		/**
		 * @brief 析构函数：自动释放内存
		 */
		~MemoryPtr()
		{
			reset();
		}

		/**
		 * @brief 重置指针
		 * @param p 新的指针（默认为 nullptr）
		 * @param size 新的内存大小（字节）
		 * @param pool 新的内存池（可选）
		 */
		void reset(T* p = nullptr, size_t size = 0, 
			std::shared_ptr<NamedMemoryPool> pool = nullptr) noexcept
		{
			if (ptr)
			{
				if (pool_)
				{
					pool_->deallocate(ptr, size_);
				}
				else
				{
					std::free(ptr);
				}
			}
			ptr = p;
			size_ = size;
			pool_ = pool;
		}

		/**
		 * @brief 获取原始指针
		 * @return 原始指针
		 */
		T* get() const noexcept { return ptr; }

		/**
		 * @brief 获取内存大小
		 * @return 内存大小（字节）
		 */
		size_t size() const noexcept { return size_; }

		/**
		 * @brief 获取所属内存池
		 * @return 内存池共享指针
		 */
		std::shared_ptr<NamedMemoryPool> getPool() const noexcept { return pool_; }

		/**
		 * @brief 指针解引用运算符
		 * @return 引用
		 */
		T& operator*() const noexcept { return *ptr; }

		/**
		 * @brief 成员访问运算符
		 * @return 指针
		 */
		T* operator->() const noexcept { return ptr; }

		/**
		 * @brief 布尔转换运算符
		 * @return true 如果指针不为空
		 */
		explicit operator bool() const noexcept { return ptr != nullptr; }

		/**
		 * @brief 释放所有权并返回指针
		 * @return 原始指针
		 */
		T* release() noexcept
		{
			T* p = ptr;
			ptr = nullptr;
			size_ = 0;
			pool_ = nullptr;
			return p;
		}

		/**
		 * @brief 交换两个指针
		 * @param other 要交换的对象
		 */
		void swap(MemoryPtr& other) noexcept
		{
			std::swap(ptr, other.ptr);
			std::swap(size_, other.size_);
			std::swap(pool_, other.pool_);
		}

	private:
		T* ptr;                                          ///< 原始指针
		size_t size_;                                     ///< 内存大小（字节）
		std::shared_ptr<NamedMemoryPool> pool_;          ///< 所属内存池
	};

	/**
	 * @brief 交换两个 MemoryPtr
	 */
	template<typename T>
	void swap(MemoryPtr<T>& lhs, MemoryPtr<T>& rhs) noexcept
	{
		lhs.swap(rhs);
	}

	/**
	 * @brief 内存分配辅助函数（使用命名内存池）
	 * 
	 * @tparam T 要分配的类型
	 * @param count 要分配的元素数量
	 * @param poolName 内存池名称
	 * @return MemoryPtr 包装的内存指针
	 * 
	 * @example
	 * ```cpp
	 * auto buffer = Allocate<uint8_t>(1024, "加载内存使用量");
	 * if (!buffer) {
	 *     // 内存分配失败
	 *     return;
	 * }
	 * // 使用 buffer.get() 访问数据
	 * ```
	 */
	template<typename T>
	MemoryPtr<T> Allocate(size_t count, std::string_view poolName = "")
	{
		std::shared_ptr<NamedMemoryPool> pool;
		if (!poolName.empty())
		{
			pool = MemoryPoolManager::getInstance().getOrCreatePool(poolName);
		}

		size_t size = sizeof(T) * count;
		void* mem = nullptr;

		if (pool)
		{
			mem = pool->allocate(size);
		}
		else
		{
			mem = std::malloc(size);
		}

		if (!mem)
		{
			return MemoryPtr<T>(nullptr, 0, nullptr);
		}

		return MemoryPtr<T>(static_cast<T*>(mem), size, pool);
	}

	// ============================================================================
	// 通用内存分配器工厂
	// ============================================================================

	/**
	 * @brief 内存分配器工厂
	 * 
	 * 提供创建各种类型分配器的便捷方法
	 */
	class AllocatorFactory
	{
	public:
		/**
		 * @brief 创建标准分配器
		 * @return 标准分配器的共享指针
		 */
		static std::shared_ptr<IMemoryAllocator> createStandard()
		{
			return std::make_shared<StandardAllocator>();
		}

		/**
		 * @brief 创建自定义分配器
		 * 
		 * @tparam AllocatorType 分配器类型（必须继承自 IMemoryAllocator）
		 * @tparam Args 构造函数参数类型
		 * @param args 构造函数参数
		 * @return 分配器的共享指针
		 * 
		 * @example
		 * ```cpp
		 * // 创建自定义分配器
		 * struct CustomAllocator : IMemoryAllocator {
		 *     void* allocate(size_t size) override { return std::malloc(size); }
		 *     void deallocate(void* ptr) override { std::free(ptr); }
		 * };
		 * auto allocator = AllocatorFactory::create<CustomAllocator>();
		 * ```
		 */
		template<typename AllocatorType, typename... Args>
		static std::shared_ptr<IMemoryAllocator> create(Args&&... args)
		{
			return std::make_shared<AllocatorType>(std::forward<Args>(args)...);
		}

		/**
		 * @brief 从函数创建分配器
		 * 
		 * @param allocFunc 分配函数
		 * @param freeFunc 释放函数
		 * @return 分配器的共享指针
		 * 
		 * @example
		 * ```cpp
		 * // 从函数指针创建分配器
		 * auto allocator = AllocatorFactory::createFromFunctions(
		 *     [](size_t size) { return std::malloc(size); },
		 *     [](void* ptr) { std::free(ptr); }
		 * );
		 * ```
		 */
		static std::shared_ptr<IMemoryAllocator> createFromFunctions(
			std::function<void*(size_t)> allocFunc,
			std::function<void(void*)> freeFunc)
		{
			class FunctionAllocator : public IMemoryAllocator
			{
			public:
				FunctionAllocator(std::function<void*(size_t)> alloc,
					std::function<void(void*)> free)
					: allocFunc_(alloc), freeFunc_(free) {}

				void* allocate(size_t size) override { return allocFunc_(size); }
				void deallocate(void* ptr) override { freeFunc_(ptr); }

			private:
				std::function<void*(size_t)> allocFunc_;
				std::function<void(void*)> freeFunc_;
			};

			return std::make_shared<FunctionAllocator>(allocFunc, freeFunc);
		}
	};


	// ============================================================================
	// STL 分配器适配器（用于 std::vector 等容器）
	// ============================================================================

	/**
	 * @brief 内存池分配器适配器
	 * 
	 * 将 NamedMemoryPool 适配为 std::allocator，使其可以在 STL 容器中使用
	 * 
	 * @tparam T 分配的元素类型
	 * 
	 * @example
	 * ```cpp
	 * auto pool = MemoryPoolManager::getInstance().getOrCreatePool("JPEG解码");
	 * PoolAllocator<uint8_t> allocator(pool);
	 * std::vector<uint8_t, PoolAllocator<uint8_t>> data(allocator);
	 * ```
	 */
	template<typename T>
	class PoolAllocator
	{
	public:
		using value_type = T;
		using pointer = T*;
		using const_pointer = const T*;
		using reference = T&;
		using const_reference = const T&;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;

		/**
		 * @brief 构造函数
		 * @param pool 内存池（可选，如果为 nullptr 则使用标准分配）
		 */
		explicit PoolAllocator(std::shared_ptr<NamedMemoryPool> pool = nullptr) noexcept
			: pool_(pool)
		{
		}

		/**
		 * @brief 拷贝构造函数
		 */
		template<typename U>
		PoolAllocator(const PoolAllocator<U>& other) noexcept
			: pool_(other.pool_)
		{
		}

		/**
		 * @brief 分配内存
		 * @param n 元素数量
		 * @return 分配的内存指针
		 */
		pointer allocate(size_type n)
		{
			if (pool_)
			{
				void* ptr = pool_->allocate(sizeof(T) * n);
				if (!ptr)
				{
					throw std::bad_alloc();
				}
				return static_cast<pointer>(ptr);
			}
			else
			{
				pointer ptr = static_cast<pointer>(std::malloc(sizeof(T) * n));
				if (!ptr)
				{
					throw std::bad_alloc();
				}
				return ptr;
			}
		}

		/**
		 * @brief 释放内存
		 * @param ptr 内存指针
		 * @param n 元素数量
		 */
		void deallocate(pointer ptr, size_type n) noexcept
		{
			if (pool_)
			{
				pool_->deallocate(ptr, sizeof(T) * n);
			}
			else
			{
				std::free(ptr);
			}
		}

		/**
		 * @brief 获取内存池
		 * @return 内存池共享指针
		 */
		std::shared_ptr<NamedMemoryPool> getPool() const noexcept
		{
			return pool_;
		}

		/**
		 * @brief 比较运算符（相同类型的分配器总是相等）
		 */
		template<typename U>
		bool operator==(const PoolAllocator<U>& other) const noexcept
		{
			return pool_ == other.pool_;
		}

		/**
		 * @brief 比较运算符
		 */
		template<typename U>
		bool operator!=(const PoolAllocator<U>& other) const noexcept
		{
			return !(*this == other);
		}

	private:
		std::shared_ptr<NamedMemoryPool> pool_;

		template<typename U>
		friend class PoolAllocator;
	};

	// ============================================================================
	// WASM 内存管理辅助功能
	// ============================================================================

#ifdef __EMSCRIPTEN__
	namespace wasm
	{
		/**
		 * @brief 获取当前 WASM 内存使用量（字节）
		 * @return 当前内存使用量
		 */
		size_t getMemoryUsage() noexcept;

		/**
		 * @brief 检查是否有足够的内存
		 * @param required 需要的字节数
		 * @return true 如果有足够的内存
		 */
		bool hasEnoughMemory(size_t required) noexcept;

		/**
		 * @brief 获取 WASM 总内存大小（字节）
		 * @return 总内存大小
		 */
		size_t getTotalMemory() noexcept;
	}
#endif

} // namespace shine::util
