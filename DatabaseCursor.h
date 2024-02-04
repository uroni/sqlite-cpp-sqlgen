#pragma once

#include "Database.h"

namespace sqlgen
{
	class DatabaseQuery;

	class DatabaseCursor
	{
	public:
		DatabaseCursor(DatabaseQuery* query, int timeoutms);
		~DatabaseCursor();

		DatabaseCursor(const DatabaseCursor&) = delete;
		DatabaseCursor(DatabaseCursor&& other) = delete;
		DatabaseCursor& operator=(const DatabaseCursor&) = delete;
		DatabaseCursor& operator=(DatabaseCursor&& other) = delete;


		bool next(db_single_result& res);

		bool reset();

		bool hasError();

		virtual void shutdown();

	private:
		DatabaseQuery* query;

		int tries;
		int timeoutms;
		int lastErr;
		bool _has_error;
		bool is_shutdown;
	};
}

