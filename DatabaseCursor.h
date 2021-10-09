#pragma once

#include "Database.h"

namespace sqlgen
{
	class DatabaseQuery;

	class DatabaseCursor
	{
	public:
		DatabaseCursor(DatabaseQuery* query, int* timeoutms);
		~DatabaseCursor(void);

		bool next(db_single_result& res);

		bool reset();

		bool has_error();

		virtual void shutdown();

	private:
		DatabaseQuery* query;

		int tries;
		int* timeoutms;
		int lastErr;
		bool _has_error;
		bool is_shutdown;
	};
}

