#pragma once
#include "RdcBoostPCH.h"
RDCBOOST_NAMESPACE_BEGIN
void Assert(bool b);
void LogError(const char* logFmt, ...);
void LogWarn(const char* logFmt, ...);
void LogInfo(const char* logFmt, ...);
RDCBOOST_NAMESPACE_END

