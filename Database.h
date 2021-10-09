#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

struct sqlite3;

namespace sqlgen
{
	class DatabaseQuery;

	const int c_sqlite_busy_timeout_default = 10000; //10 seconds

	typedef std::map<std::string, std::string> str_map;
	typedef str_map db_single_result;
	typedef std::vector<str_map> db_results;

	class Database
	{
	public:
		bool Open(std::string pFile, const std::vector<std::pair<std::string, std::string> >& attach,
			size_t allocation_chunk_size, const str_map& p_params);
		~Database();

		virtual db_results Read(std::string pQuery);
		virtual bool Write(std::string pQuery);

		virtual bool BeginReadTransaction();
		virtual bool BeginWriteTransaction();
		virtual bool EndTransaction(void);
		virtual bool RollbackTransaction();

		virtual DatabaseQuery* Prepare(std::string pQuery, bool autodestroy = true);
		virtual DatabaseQuery* Prepare(int id, std::string pQuery);
		virtual void destroyQuery(DatabaseQuery* q);
		virtual void destroyAllQueries(void);

		virtual long long int getLastInsertID(void);

		sqlite3* getDatabase(void);

		bool isInTransaction(void);

		static void initMutex(void);
		static void destroyMutex(void);


		virtual std::string getEngineName(void);

		virtual void DetachDBs(void);
		virtual void AttachDBs(void);

		virtual void freeMemory();

		virtual int getLastChanges();

		virtual std::string Database::getTempDirectoryPath();

	private:
		sqlite3* db;
		bool in_transaction;

		std::vector<DatabaseQuery*> queries;
		std::map<int, DatabaseQuery*> prepared_queries;

		std::vector<std::pair<std::string, std::string> > attached_dbs;
		str_map params;
	};

}