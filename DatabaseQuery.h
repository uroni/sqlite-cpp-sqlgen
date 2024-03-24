#pragma once

#include <memory>

#include "Database.h"
#include "DatabaseCursor.h"

struct sqlite3_stmt;
struct sqlite3;

namespace sqlgen
{
	class Database;
	class DatabaseCursor;

	class DatabaseQuery
	{
		friend class Database;

		DatabaseQuery(const std::string& pStmt_str, sqlite3_stmt* prepared_statement, Database* pDB);
	public:		
		DatabaseQuery() {}
		~DatabaseQuery();

		DatabaseQuery(const DatabaseQuery&) = delete;
		DatabaseQuery(DatabaseQuery&& other) {
			*this = std::move(other);
		}
		DatabaseQuery& operator=(const DatabaseQuery&) = delete;
		DatabaseQuery& operator=(DatabaseQuery&& other);

		bool prepared()
		{
			return ps != nullptr;
		}

		virtual void bind(const std::string& str);
		virtual void bind(int p);
		virtual void bind(unsigned int p);
		virtual void bind(double p);
		virtual void bind(int64_t p);
#if defined(_WIN64) || defined(_LP64)
		virtual void bind(size_t p);
#endif
		virtual void bind(const char* buffer, size_t bsize);

		virtual void reset();

		virtual bool write(int timeoutms = -1);
		db_results read(int timeoutms = -1);

		virtual DatabaseCursor& cursor(int timeoutms = -1);

		std::string getStatement(void);

		std::string getErrMsg(void);

	private:
		bool Execute(int timeoutms);
		int step(db_single_result* res, int timeoutms, int& tries, bool& reset);

		void setupStepping(int timeoutms);
		void shutdownStepping(int err, int timeoutms);

		bool resultOkay(int rc);

		sqlite3_stmt* getSQliteStmt() {
			return ps;
		}

		std::string ustring_sqlite3_column_name(int N);

		sqlite3_stmt* ps = nullptr;
		std::string stmt_str;
		Database* db = nullptr;
		int curr_idx = 1;
		std::unique_ptr<DatabaseCursor> _cursor;

		friend class DatabaseCursor;
	};

}