#pragma once

#include <memory>

#include "Database.h"

struct sqlite3_stmt;
struct sqlite3;

namespace sqlgen
{
	class Database;
	class DatabaseCursor;

	class DatabaseQuery
	{
	public:
		DatabaseQuery(const std::string& pStmt_str, sqlite3_stmt* prepared_statement, Database* pDB);
		~DatabaseQuery();

		virtual void Bind(const std::string& str);
		virtual void Bind(int p);
		virtual void Bind(unsigned int p);
		virtual void Bind(double p);
		virtual void Bind(long long int p);
#if defined(_WIN64) || defined(_LP64)
		virtual void Bind(size_t p);
#endif
		virtual void Bind(const char* buffer, unsigned int bsize);

		virtual void Reset(void);

		virtual bool Write(int timeoutms = -1);
		db_results Read(int* timeoutms = NULL);

		virtual DatabaseCursor* Cursor(int* timeoutms = NULL);

		void setupStepping(int* timeoutms);
		void shutdownStepping(int err, int* timeoutms);

		int step(db_single_result& res, int* timeoutms, int& tries, bool& reset);

		bool resultOkay(int rc);

		std::string getStatement(void);

		std::string getErrMsg(void);

	private:
		bool Execute(int timeoutms);

		sqlite3_stmt* ps;
		std::string stmt_str;
		Database* db;
		int curr_idx;
		DatabaseCursor* cursor;
	};

}