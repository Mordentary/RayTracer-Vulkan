#include "logger.hpp"
#include <ctime>     // For std::time
#include <iostream>  // For std::cerr

namespace SE {
	Logger& Logger::getInstance() {
		static Logger instance;
		return instance;
	}

	Logger::Logger() = default;

	Logger::~Logger() {
		if (m_FileStream.is_open()) {
			m_FileStream.close();
		}
	}

	void Logger::create(const std::filesystem::path& logFile) {
		if (!logFile.empty()) {
			m_FileStream.open(logFile, std::ios::out | std::ios::app);
		}
	}

	void Logger::setLogLevel(LogLevel level) {
		m_MinLevel = level;
	}

	void Logger::setShowTimestamps(bool show) {
		m_ShowTimestamps = show;
	}

	void Logger::setShowSourceLocation(bool show) {
		m_ShowSourceLocation = show;
	}

	void Logger::assertFailed(const char* expr, const std::source_location& loc, const std::string& msg) {
		logAssert(expr, msg, loc);
#ifdef _MSC_VER
		__debugbreak();
#else
		std::abort();
#endif
	}

	void Logger::logAssert(const char* expr, const std::string& msg, const std::source_location& location) {
		std::string formattedMsg = formatLogMessage(
			LogLevel::Critical, location, fmt::format("Assertion failed: {}. Message: {}", expr, msg));

		// Output to console with color
		fmt::print("{}\n", fmt::styled(formattedMsg, getLevelStyle(LogLevel::Critical)));

		// Output to file if enabled
		if (m_FileStream.is_open()) {
			m_FileStream << formattedMsg << std::endl;
		}
	}

	bool Logger::vkCheck(VkResult result, const std::source_location& loc, const std::string& msg) {
		if (result != VK_SUCCESS) {
			logVulkanError(result, msg, loc);
			return false;
		}
		return true;
	}

	void Logger::logVulkanError(VkResult result, const std::string& msg, const std::source_location& loc) {
		std::string errorMsg = fmt::format("Vulkan error {}: {}", vkResultToString(result), msg);
		log(LogLevel::Error, loc, "{}", errorMsg);
	}

	std::string Logger::formatLogMessage(LogLevel level, const std::source_location& location, const std::string& message) {
		std::string result;

		// Add timestamp if enabled
		if (m_ShowTimestamps) {
			auto time = std::time(nullptr);
			result += fmt::format("[{:%Y-%m-%d %H:%M:%S}] ", *std::localtime(&time));
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

	fmt::text_style Logger::getLevelStyle(LogLevel level) const {
		switch (level) {
		case LogLevel::Trace: return fmt::fg(fmt::color::light_gray);
		case LogLevel::Debug: return fmt::fg(fmt::color::white);
		case LogLevel::Info: return fmt::fg(fmt::color::green);
		case LogLevel::Warn: return fmt::fg(fmt::color::yellow);
		case LogLevel::Error: return fmt::fg(fmt::color::red);
		case LogLevel::Critical: return fmt::fg(fmt::color::red) | fmt::emphasis::bold;
		default: return fmt::text_style();
		}
	}

	const char* Logger::getLevelString(LogLevel level) const {
		switch (level) {
		case LogLevel::Trace: return "Trace";
		case LogLevel::Debug: return "Debug";
		case LogLevel::Info: return "Info";
		case LogLevel::Warn: return "Warn";
		case LogLevel::Error: return "Error";
		case LogLevel::Critical: return "Critical";
		default: return "Unknown";
		}
	}
}  // namespace SE