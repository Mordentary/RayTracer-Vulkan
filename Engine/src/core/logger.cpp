#include "Logger.hpp"

namespace SE {

	void Logger::init(const std::filesystem::path& logFile) {
		if (!logFile.empty()) {
			m_FileStream.open(logFile, std::ios::out | std::ios::app);
			if (!m_FileStream.is_open()) {
				fmt::print(stderr, "Failed to open log file: {}\n", logFile.string());
			}
		}
	}

	Logger::~Logger() {
		if (m_FileStream.is_open()) {
			m_FileStream.close();
		}
	}

	void Logger::logAssert(const char* expr, const std::string& msg,
		const std::source_location& location) {
		const auto assertStyle = fg(fmt::color::red) | bg(fmt::color::black) | fmt::emphasis::bold;

		std::string assertMsg = fmt::format(
			"\nAssertion failed!\n"
			"Expression: {}\n"
			"Message: {}\n"
			"File: {}\n"
			"Line: {}\n"
			"Function: {}\n",
			expr, msg,
			std::filesystem::path(location.file_name()).string(),
			location.line(),
			location.function_name()
		);

		// Log to console with styling
		fmt::print("{}", fmt::styled(assertMsg, assertStyle));

		// Log to file if enabled
		if (m_FileStream.is_open()) {
			m_FileStream << assertMsg << std::endl;
		}
	}

	fmt::text_style Logger::getLevelStyle(LogLevel level) const {
		switch (level) {
		case LogLevel::Trace:
			return fg(fmt::color::gray);
		case LogLevel::Debug:
			return fg(fmt::color::light_blue);
		case LogLevel::Info:
			return fg(fmt::color::white);
		case LogLevel::Warn:
			return fg(fmt::color::yellow) | fmt::emphasis::bold;
		case LogLevel::Error:
			return fg(fmt::color::red) | fmt::emphasis::bold;
		case LogLevel::Critical:
			return fg(fmt::color::red) | bg(fmt::color::black) | fmt::emphasis::bold;
		default:
			return {};
		}
	}

	const char* Logger::getLevelString(LogLevel level) const {
		switch (level) {
		case LogLevel::Trace:    return "TRACE";
		case LogLevel::Debug:    return "DEBUG";
		case LogLevel::Info:     return "INFO";
		case LogLevel::Warn:     return "WARN";
		case LogLevel::Error:    return "ERROR";
		case LogLevel::Critical: return "CRITICAL";
		default:                 return "UNKNOWN";
		}
	}

	void Logger::logVulkanError(VkResult result, const std::string& msg,
		const std::source_location& loc) {
		const auto vulkanErrorStyle = fg(fmt::color::red) | fmt::emphasis::bold;

		std::string errorMsg = fmt::format(
			"\nVulkan Error!\n"
			"Result: {} ({})\n"
			"Message: {}\n"
			"File: {}\n"
			"Line: {}\n"
			"Function: {}\n",
			vkResultToString(result),
			static_cast<int>(result),
			msg,
			std::filesystem::path(loc.file_name()).string(),
			loc.line(),
			loc.function_name()
		);

		// Log to console with styling
		fmt::print("{}", fmt::styled(errorMsg, vulkanErrorStyle));

		// Log to file if enabled
		if (m_FileStream.is_open()) {
			m_FileStream << errorMsg << std::endl;
		}

		// Print stack trace in debug builds
#if defined(_DEBUG) || !defined(NDEBUG)
		if (auto st = std::stacktrace::current(); st.size() > 0) {
			m_FileStream << "\nStack trace:\n" << st << std::endl;
			fmt::print("\nStack trace:\n{}\n", st);
		}
#endif
	}

	const char* Logger::vkResultToString(VkResult result) const {
		switch (result) {
		case VK_SUCCESS: return "VK_SUCCESS";
		case VK_NOT_READY: return "VK_NOT_READY";
		case VK_TIMEOUT: return "VK_TIMEOUT";
		case VK_EVENT_SET: return "VK_EVENT_SET";
		case VK_EVENT_RESET: return "VK_EVENT_RESET";
		case VK_INCOMPLETE: return "VK_INCOMPLETE";
		case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
		case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
		case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
		case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
		case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";
		case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
		case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
		case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
		case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
		case VK_ERROR_TOO_MANY_OBJECTS: return "VK_ERROR_TOO_MANY_OBJECTS";
		case VK_ERROR_FORMAT_NOT_SUPPORTED: return "VK_ERROR_FORMAT_NOT_SUPPORTED";
		case VK_ERROR_FRAGMENTED_POOL: return "VK_ERROR_FRAGMENTED_POOL";
		case VK_ERROR_UNKNOWN: return "VK_ERROR_UNKNOWN";
		case VK_ERROR_OUT_OF_POOL_MEMORY: return "VK_ERROR_OUT_OF_POOL_MEMORY";
		case VK_ERROR_INVALID_EXTERNAL_HANDLE: return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
		case VK_ERROR_SURFACE_LOST_KHR: return "VK_ERROR_SURFACE_LOST_KHR";
		case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
		case VK_SUBOPTIMAL_KHR: return "VK_SUBOPTIMAL_KHR";
		case VK_ERROR_OUT_OF_DATE_KHR: return "VK_ERROR_OUT_OF_DATE_KHR";
		case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
		case VK_ERROR_VALIDATION_FAILED_EXT: return "VK_ERROR_VALIDATION_FAILED_EXT";
		case VK_ERROR_INVALID_SHADER_NV: return "VK_ERROR_INVALID_SHADER_NV";
		default: return "VK_UNKNOWN_ERROR";
		}
	}


} 