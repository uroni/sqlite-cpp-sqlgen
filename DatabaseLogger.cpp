/**
 * Copyright (C) Martin Raiber
 * SPDX-License-Identifier: Apache-2.0.
 */

#include "DatabaseLogger.h"
#include <iostream>

using namespace sqlgen;

namespace sqlgen
{
	void DatabaseLogger::Log(const std::string& msg, LogLevel loglevel)
	{
		const char* ll_str;
		switch (loglevel)
		{
		case LL_DEBUG:
			ll_str = "DEBUG: ";
			break;
		case LL_INFO:
			ll_str = "INFO: ";
			break;
		case LL_WARNING:
			ll_str = "WARNING: ";
			break;
		case LL_ERROR:
			ll_str = "ERROR: ";
			break;
		default:
			ll_str = "";
			break;
		}

		std::cout << ll_str << msg << std::endl;
	}

	IDatabaseLogger* getDatabaseLogger()
	{
		static DatabaseLogger logger;
		return &logger;
	}
}
