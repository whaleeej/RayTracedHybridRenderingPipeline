#pragma once

namespace rdcboost
{
	void Assert(bool b);
	void LogError(const char* logFmt, ...);
	void LogWarn(const char* logFmt, ...);
	void LogInfo(const char* logFmt, ...);
}

