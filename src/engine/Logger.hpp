#pragma once
#ifndef _LOGGER_HPP_
#define _LOGGER_HPP_

#include <string>
#include <vector>
#include <cstdarg>
#include <raylib-cpp.hpp>
#include <source_location>

namespace Long {
	//Entry Log for the logger. Each entry contains a log level and the log text.
	struct LogEntry {
		int level;          // TraceLogLevel (LOG_INFO, LOG_WARNING, ...)
		std::string text;
	};

	class Logger {
	public:
		static void Install();
		// Defined in the .cpp (not inline) so they live inside the DLL alongside
		// s_entries -- WINDOWS_EXPORT_ALL_SYMBOLS doesn't export data symbols, so an
		// inline accessor in the exe couldn't reach s_entries across the DLL boundary.
		static const std::vector<LogEntry>& Entries();
		static void Clear();
		static void TraceLog(const TraceLogLevel level,
			const std::string& log,
			const std::source_location location = std::source_location::current());
	private:
		static std::vector<LogEntry> s_entries;
		static constexpr size_t kMaxEntries = 1000;
		static void Callback(int logLevel, const char* text, va_list args);
	};

} // namespace Long

#endif // !_LOGGER_HPP_
