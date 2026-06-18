#include "engine/Logger.hpp"
#include "raylib.h"
#include <source_location>
#include <cstdio>

namespace Long {

	std::vector<LogEntry> Logger::s_entries;

	void Logger::Install() {
		SetTraceLogCallback(&Logger::Callback);
	}

	void Logger::TraceLog(const TraceLogLevel level, const std::string& log, const std::source_location location)
	{
		::TraceLog(level, "%s:%d: %s", location.file_name(), (int)location.line(), log.c_str());
	}

	void Logger::Callback(int logLevel, const char* text, va_list args) {
		char buffer[1024];
		std::vsnprintf(buffer, sizeof(buffer), text, args);
		std::printf("%s\n", buffer);
		if (s_entries.size() >= kMaxEntries) {
			s_entries.erase(s_entries.begin());
		}
		s_entries.push_back(LogEntry{ logLevel, buffer });
	}

} // namespace Long
