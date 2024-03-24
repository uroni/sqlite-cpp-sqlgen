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

		bool next(db_single_result& res)
		{
			return next(&res);
		}

		bool next(db_single_result* res = nullptr);

		bool reset();

		bool has_error();

		virtual void shutdown();

		bool get(int col, std::string& v);
		bool get(int col, int& v);
		bool get(int col, long long int& v);
		bool get(int col, double& v);

		bool get(const std::string& col, std::string& v);
		bool get(const std::string& col, int& v);
		bool get(const std::string& col, long long int& v);
		bool get(const std::string& col, double& v);

	private:
		DatabaseQuery* query;

		void init_col_names();
		int get_col_idx(const std::string& col);

		int tries;
		int* timeoutms;
		int lastErr;
		bool _has_error;
		bool is_shutdown;

		std::map<std::string, int> col_names;
	};
}

