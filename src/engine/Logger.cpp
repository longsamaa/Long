#include "engine/Logger.hpp"
#include "raylib.h"

#include <cstdio>

namespace Long {

	std::vector<LogEntry> Logger::s_entries;

	void Logger::Install() {
		SetTraceLogCallback(&Logger::Callback);
	}

	void Logger::Callback(int logLevel, const char* text, va_list args) {
		// Format the message (raylib passes printf-style text + args).
		char buffer[1024];
		std::vsnprintf(buffer, sizeof(buffer), text, args);

		// Still echo to stdout so the normal console keeps working.
		std::printf("%s\n", buffer);

		// Drop the oldest entry if we hit the cap (cheap ring behaviour).
		if (s_entries.size() >= kMaxEntries) {
			s_entries.erase(s_entries.begin());
		}
		s_entries.push_back(LogEntry{ logLevel, buffer });
	}

} // namespace Long
