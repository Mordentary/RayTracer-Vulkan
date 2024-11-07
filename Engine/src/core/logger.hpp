#pragma once

#include <fmt/core.h>
#include <fmt/color.h>
#include <fmt/chrono.h>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <vulkan\vulkan_core.h>
#include <source_location>
#include <stacktrace>

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
		static Logger& getInstance() {
			static Logger instance;
			return instance;
		}

		// Delete copy/move constructors and assignment operators
		Logger(const Logger&) = delete;
		Logger& operator=(const Logger&) = delete;
		Logger(Logger&&) = delete;
		Logger& operator=(Logger&&) = delete;

		// Initialize logger with optional file output
		void init(const std::filesystem::path& logFile = "");
		void setLogLevel(LogLevel level) { m_MinLevel = level; }
		void setShowTimestamps(bool show) { m_ShowTimestamps = show; }
		void setShowSourceLocation(bool show) { m_ShowSourceLocation = show; }

		// Core logging functions
		template<typename... Args>
		void trace(fmt::format_string<Args...> fmt, Args&&... args) {
			log(LogLevel::Trace, fmt, std::forward<Args>(args)...);
		}

		template<typename... Args>
		void debug(fmt::format_string<Args...> fmt, Args&&... args) {
			log(LogLevel::Debug, fmt, std::forward<Args>(args)...);
		}

		template<typename... Args>
		void info(fmt::format_string<Args...> fmt, Args&&... args) {
			log(LogLevel::Info, fmt, std::forward<Args>(args)...);
		}

		template<typename... Args>
		void warn(fmt::format_string<Args...> fmt, Args&&... args) {
			log(LogLevel::Warn, fmt, std::forward<Args>(args)...);
		}

		template<typename... Args>
		void error(fmt::format_string<Args...> fmt, Args&&... args) {
			log(LogLevel::Error, fmt, std::forward<Args>(args)...);
		}

		template<typename... Args>
		void critical(fmt::format_string<Args...> fmt, Args&&... args) {
			log(LogLevel::Critical, fmt, std::forward<Args>(args)...);
		}

		// Assertion support
		template<typename... Args>
		void assertFailed(const char* expr, fmt::format_string<Args...> fmt, Args&&... args,
			const std::source_location& loc = std::source_location::current()) {
			std::string msg = fmt::format(fmt, std::forward<Args>(args)...);
			logAssert(expr, msg, loc);

			if (auto st = std::stacktrace::current(); st.size() > 0) {
				m_FileStream << "\nStack trace:\n" << st << std::endl;
				fmt::print("\nStack trace:\n{}\n", st);
			}

#ifdef _MSC_VER
			__debugbreak();
#else
			std::abort();
#endif
		}
		template<typename... Args>
		bool vkCheck(VkResult result, fmt::format_string<Args...> fmt, Args&&... args,
			const std::source_location& loc = std::source_location::current()) {
			if (result != VK_SUCCESS) {
				std::string msg = fmt::format(fmt, std::forward<Args>(args)...);
				logVulkanError(result, msg, loc);
				return false;
			}
			return true;
		}
	private:
		Logger() = default;
		~Logger();

		template<typename... Args>
		void log(LogLevel level, fmt::format_string<Args...> fmt, Args&&... args,
			const std::source_location& location = std::source_location::current()) {
			if (level < m_MinLevel) return;

			std::lock_guard<std::mutex> lock(m_Mutex);

			std::string formattedMsg = formatLogMessage(level, fmt, std::forward<Args>(args)..., location);

			// Output to console with color
			fmt::print("{}\n", fmt::styled(formattedMsg, getLevelStyle(level)));

			// Output to file if enabled
			if (m_FileStream.is_open()) {
				m_FileStream << formattedMsg << std::endl;
			}
		}

		void logAssert(const char* expr, const std::string& msg,
			const std::source_location& location);

		template<typename... Args>
		std::string formatLogMessage(LogLevel level, fmt::format_string<Args...> fmt, Args&&... args,
			const std::source_location& location) {
			std::string message = fmt::format(fmt, std::forward<Args>(args)...);
			std::string result;

			// Add timestamp if enabled
			if (m_ShowTimestamps) {
				result += fmt::format("[{}] ", fmt::localtime(std::time(nullptr)));
			}

			// Add log level
			result += fmt::format("[{}] ", getLevelString(level));

			// Add source location if enabled
			if (m_ShowSourceLocation) {
				result += fmt::format("[{}:{}] ",
					std::filesystem::path(location.file_name()).filename().string(),
					location.line());
			}

			result += message;
			return result;
		}
		void logVulkanError(VkResult result, const std::string& msg,
			const std::source_location& loc);

		// Helper to convert VkResult to string
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
	void LogTrace(fmt::format_string<Args...> fmt, Args&&... args) {
		Logger::getInstance().trace(fmt, std::forward<Args>(args)...);
	}

	template<typename... Args>
	void LogDebug(fmt::format_string<Args...> fmt, Args&&... args) {
		Logger::getInstance().debug(fmt, std::forward<Args>(args)...);
	}

	template<typename... Args>
	void LogInfo(fmt::format_string<Args...> fmt, Args&&... args) {
		Logger::getInstance().info(fmt, std::forward<Args>(args)...);
	}

	template<typename... Args>
	void LogWarn(fmt::format_string<Args...> fmt, Args&&... args) {
		Logger::getInstance().warn(fmt, std::forward<Args>(args)...);
	}

	template<typename... Args>
	void LogError(fmt::format_string<Args...> fmt, Args&&... args) {
		Logger::getInstance().error(fmt, std::forward<Args>(args)...);
	}

	template<typename... Args>
	void LogCritical(fmt::format_string<Args...> fmt, Args&&... args) {
		Logger::getInstance().critical(fmt, std::forward<Args>(args)...);
	}

	// Assertion macros
#if defined(_DEBUG) || !defined(NDEBUG)
#define SE_ASSERT(expr, ...) \
        do { \
            if (!(expr)) { \
                ::SE::Logger::getInstance().assertFailed(#expr, __VA_ARGS__); \
            } \
        } while(0)

#define SE_ASSERT_MSG(expr, msg) SE_ASSERT(expr, "{}", msg)
#define SE_ASSERT_NOMSG(expr) SE_ASSERT(expr, "Assertion '{}' failed", #expr)
#else
#define SE_ASSERT(expr, ...) expr
#define SE_ASSERT_MSG(expr, msg) expr
#define SE_ASSERT_NOMSG(expr) expr
#endif

#define VK_CHECK_LOG(call, ...) \
    do { \
        VkResult result = (call); \
        if (!::SE::Logger::getInstance().vkCheck(result, __VA_ARGS__)) { \
            SE_ASSERT_NOMSG(false); \
        } \
    } while(0)

#define VK_CHECK_RETURN(call, returnVal, ...) \
    do { \
        VkResult result = (call); \
        if (!::SE::Logger::getInstance().vkCheck(result, __VA_ARGS__)) { \
            return returnVal; \
        } \
    } while(0)

#define VK_CHECK_RETURN_VOID(call, ...) \
    do { \
        VkResult result = (call); \
        if (!::SE::Logger::getInstance().vkCheck(result, __VA_ARGS__)) { \
            return; \
        } \
    } while(0)

#define VK_CHECK(call) VK_CHECK_LOG(call, "Vulkan call failed: {}", #call)
}