#pragma once

#include <fmt/core.h>
#include <fmt/color.h>
#include <fmt/chrono.h>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <vulkan/vulkan_core.h>
#include <source_location>

namespace SE {
	enum class LogLevel : uint8_t {
		Trace,
		Debug,
		Info,
		Warn,
		Error,
		Critical,
		Off
	};

	class Logger {
	public:
		static Logger& getInstance();

		// Delete copy/move constructors and assignment operators
		Logger(const Logger&) = delete;
		Logger& operator=(const Logger&) = delete;
		Logger(Logger&&) = delete;
		Logger& operator=(Logger&&) = delete;

		// Initialize logger with optional file output
		void create(const std::filesystem::path& logFile = "");
		void setLogLevel(LogLevel level);
		void setShowTimestamps(bool show);
		void setShowSourceLocation(bool show);

		// Core logging functions
		template<typename... Args>
		void trace(const char* fmtStr, Args&&... args) {
			log(LogLevel::Trace, std::source_location::current(), fmtStr, std::forward<Args>(args)...);
		}

		template<typename... Args>
		void debug(const char* fmtStr, Args&&... args) {
			log(LogLevel::Debug, std::source_location::current(), fmtStr, std::forward<Args>(args)...);
		}

		template<typename... Args>
		void info(const char* fmtStr, Args&&... args) {
			log(LogLevel::Info, std::source_location::current(), fmtStr, std::forward<Args>(args)...);
		}

		template<typename... Args>
		void warn(const char* fmtStr, Args&&... args) {
			log(LogLevel::Warn, std::source_location::current(), fmtStr, std::forward<Args>(args)...);
		}

		template<typename... Args>
		void error(const char* fmtStr, Args&&... args) {
			log(LogLevel::Error, std::source_location::current(), fmtStr, std::forward<Args>(args)...);
		}

		template<typename... Args>
		void critical(const char* fmtStr, Args&&... args) {
			log(LogLevel::Critical, std::source_location::current(), fmtStr, std::forward<Args>(args)...);
		}

		// Assertion support
		void assertFailed(const char* expr, const std::source_location& loc, const std::string& msg);

		template<typename... Args>
		void assertFailed(const char* expr, const std::source_location& loc, const char* fmtStr, Args&&... args) {
			std::string msg = fmt::format(fmt::runtime(fmtStr), std::forward<Args>(args)...);
			logAssert(expr, msg, loc);
#ifdef _MSC_VER
			__debugbreak();
#else
			std::abort();
#endif
		}

		void assertFailed(const char* expr, const std::source_location& loc, const char* msg) {
			logAssert(expr, msg, loc);
#ifdef _MSC_VER
			__debugbreak();
#else
			std::abort();
#endif
		}

		bool vkCheck(VkResult result, const std::source_location& loc, const std::string& msg);

		template<typename... Args>
		bool vkCheck(VkResult result, const std::source_location& loc, const char* fmtStr, Args&&... args) {
			if (result != VK_SUCCESS) {
				std::string msg = fmt::format(fmt::runtime(fmtStr), std::forward<Args>(args)...);
				logVulkanError(result, msg, loc);
				return false;
			}
			return true;
		}

	private:
		Logger();
		~Logger();

		template<typename... Args>
		void log(LogLevel level, const std::source_location& location, const char* fmtStr, Args&&... args) {
			if (level < m_MinLevel) return;

			std::lock_guard<std::mutex> lock(m_Mutex);

			std::string message = fmt::format(fmt::runtime(fmtStr), std::forward<Args>(args)...);
			std::string formattedMsg = formatLogMessage(level, location, message);

			// Output to console with color
			fmt::print("{}\n", fmt::styled(formattedMsg, getLevelStyle(level)));

			// Output to file if enabled
			if (m_FileStream.is_open()) {
				m_FileStream << formattedMsg << std::endl;
			}
		}

		std::string formatLogMessage(LogLevel level, const std::source_location& location, const std::string& message);

		void logAssert(const char* expr, const std::string& msg, const std::source_location& location);

		void logVulkanError(VkResult result, const std::string& msg, const std::source_location& loc);

		const char* vkResultToString(VkResult result) const;

		fmt::text_style getLevelStyle(LogLevel level) const;
		const char* getLevelString(LogLevel level) const;

		LogLevel m_MinLevel = LogLevel::Info;
		bool m_ShowTimestamps = true;
		bool m_ShowSourceLocation = true;
		std::mutex m_Mutex;
		std::ofstream m_FileStream;
	};

	// Convenient global functions
	template<typename... Args>
	void LogTrace(const char* fmtStr, Args&&... args) {
		Logger::getInstance().trace(fmtStr, std::forward<Args>(args)...);
	}

	template<typename... Args>
	void LogDebug(const char* fmtStr, Args&&... args) {
		Logger::getInstance().debug(fmtStr, std::forward<Args>(args)...);
	}

	template<typename... Args>
	void LogInfo(const char* fmtStr, Args&&... args) {
		Logger::getInstance().info(fmtStr, std::forward<Args>(args)...);
	}

	template<typename... Args>
	void LogWarn(const char* fmtStr, Args&&... args) {
		Logger::getInstance().warn(fmtStr, std::forward<Args>(args)...);
	}

	template<typename... Args>
	void LogError(const char* fmtStr, Args&&... args) {
		Logger::getInstance().error(fmtStr, std::forward<Args>(args)...);
	}

	template<typename... Args>
	void LogCritical(const char* fmtStr, Args&&... args) {
		Logger::getInstance().critical(fmtStr, std::forward<Args>(args)...);
	}

	// Assertion macros
#if defined(_DEBUG) || !defined(NDEBUG)
	namespace detail {
		template<typename... Args>
		inline void se_assert_impl(bool condition,
			const char* exprStr,
			const std::source_location& loc,
			Args&&... args)
		{
			if (!condition) {
				// If no extra arguments at all, use a default message
				if constexpr (sizeof...(args) == 0) {
					SE::Logger::getInstance().assertFailed(
						exprStr,
						loc,
						"Assertion '{}' failed",
						exprStr
					);
				}
				// Else forward all user-supplied arguments
				else {
					SE::Logger::getInstance().assertFailed(
						exprStr,
						loc,
						std::forward<Args>(args)...
					);
				}
			}
		}
	}

#define SE_ASSERT(expr, ...) \
    SE::detail::se_assert_impl((expr), #expr, std::source_location::current(), ##__VA_ARGS__)

#define SE_ASSERT_MSG(expr, msg) SE_ASSERT(expr, msg)

#define SE_ASSERT_NOMSG(expr) SE_ASSERT(expr)

#else
#define SE_ASSERT(expr, ...) (void)(expr)
#define SE_ASSERT_MSG(expr, msg) (void)(expr)
#define SE_ASSERT_NOMSG(expr) (void)(expr)
#endif

#define VK_CHECK_LOG(call, ...) \
        do { \
            VkResult result = (call); \
            if (!SE::Logger::getInstance().vkCheck( \
                    result, std::source_location::current(), __VA_ARGS__)) { \
                SE_ASSERT_NOMSG(false); \
            } \
        } while(0)

#define VK_CHECK_RETURN(call, returnVal, ...) \
        do { \
            VkResult result = (call); \
            if (!SE::Logger::getInstance().vkCheck( \
                    result, std::source_location::current(), __VA_ARGS__)) { \
                return returnVal; \
            } \
        } while(0)

#define VK_CHECK_RETURN_VOID(call, ...) \
        do { \
            VkResult result = (call); \
            if (!SE::Logger::getInstance().vkCheck( \
                    result, std::source_location::current(), __VA_ARGS__)) { \
                return; \
            } \
        } while(0)

#define VK_CHECK(call) VK_CHECK_LOG(call, "Vulkan call failed: {}", #call)
} // namespace SE