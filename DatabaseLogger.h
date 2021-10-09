#pragma once
#include <string>

namespace sqlgen
{
	enum LogLevel
	{
		LL_DEBUG,
		LL_INFO,
		LL_WARNING,
		LL_ERROR
	};

	class IDatabaseLogger
	{
	public:
		virtual void Log(const std::string& msg, LogLevel loglevel = LL_INFO) = 0;
	};

	class DatabaseLogger : public IDatabaseLogger
	{
		virtual void Log(const std::string& msg, LogLevel loglevel = LL_INFO);
	};

	IDatabaseLogger* getDatabaseLogger();

}