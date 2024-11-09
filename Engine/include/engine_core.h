#pragma once

#include <core\logger.hpp>
#include <performance_timer.hpp>
#include<memory>
namespace SE {
#define VK_CHECK(expr) (void)expr

	template <typename T>
	using Shared = std::shared_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Shared<T> CreateShared(Args&& ... args)
	{
		return std::make_shared<T>(std::forward<Args>(args)...);
	}

	template <typename T, typename Deleter = std::default_delete<T>>
	using Scoped = std::unique_ptr<T, Deleter>;
	template<typename T, typename ... Args>
	constexpr Scoped<T> CreateScoped(Args&& ... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

	template <typename T>
	using Weak = std::weak_ptr<T>;
}
