#pragma once
#ifndef _LOGGER_HPP_
#define _LOGGER_HPP_

#include <string>
#include <vector>
#include <cstdarg>

namespace Long {

	// Captures raylib's TraceLog output into an in-memory ring of entries that
	// the editor's Console panel can display (with color per level).
	//
	// raylib's log callback is a C function (no 'this'), so the storage is a
	// process-wide singleton accessed via Get().
	struct LogEntry {
		int level;          // TraceLogLevel (LOG_INFO, LOG_WARNING, ...)
		std::string text;
	};

	class Logger {
	public:
		// Install as raylib's trace-log callback. Call once after InitWindow.
		static void Install();

		static const std::vector<LogEntry>& Entries() { return s_entries; }
		static void Clear() { s_entries.clear(); }

	private:
		static std::vector<LogEntry> s_entries;
		static constexpr size_t kMaxEntries = 1000;

		static void Callback(int logLevel, const char* text, va_list args);
	};

} // namespace Long

#endif // !_LOGGER_HPP_
