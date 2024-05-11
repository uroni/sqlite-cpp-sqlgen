#pragma once

#include <stdexcept>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <optional>

struct sqlite3;

namespace sqlgen
{
	class DatabaseQuery;

	const int c_sqlite_busy_timeout_default = 10000; //10 seconds

	typedef std::map<std::string, std::string> str_map;
	typedef str_map db_single_result;
	typedef std::vector<str_map> db_results;

	class DatabaseOpenError : public std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;	
	};

	class PrepareError : public std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;
	};

	class Database
	{
	public:
		Database(const std::string& pFile, std::vector<std::pair<std::string, std::string> > attach = {},
			size_t allocation_chunk_size = std::string::npos, str_map p_params = {});
		~Database();
		Database(const Database&) = delete;
		Database(Database&& other);
		Database& operator=(const Database&) = delete;
		Database& operator=(Database&& other);

		virtual db_results read(const std::string& query);
		virtual void write(const std::string& query);

		virtual void beginReadTransaction();
		virtual void beginWriteTransaction();
		virtual void endTransaction();
		virtual void rollbackTransaction();

		virtual DatabaseQuery prepare(std::string pQuery);

		virtual long long int getLastInsertID();

		sqlite3* getDatabase();

		bool isInTransaction();

		virtual std::string getEngineName();

		virtual void detachDBs();
		virtual void attachDBs();

		virtual void freeMemory();

		virtual int getLastChanges();

		virtual std::string getTempDirectoryPath();

	private:
		bool openInternal(std::string pFile, std::vector<std::pair<std::string, std::string> > attach,
			size_t allocation_chunk_size, str_map p_params);

		sqlite3* db = nullptr;
		bool in_transaction = false;

		std::vector<std::pair<std::string, std::string> > attached_dbs;
		str_map params;
	};

	class ScopedAutoCommitWriteTransaction
	{
	public:
		ScopedAutoCommitWriteTransaction(Database* db = nullptr)
			: db(db) {
			if (db != nullptr) db->beginWriteTransaction();
		}
		~ScopedAutoCommitWriteTransaction() {
			if (db != nullptr) db->endTransaction();
		}

		void reset(Database* pdb)
		{
			if (db != nullptr) {
				db->endTransaction();
			}
			db = pdb;
			if (db != nullptr) {
				db->beginWriteTransaction();
			}
		}

		void restart() {
			if (db != nullptr) {
				db->endTransaction();
				db->beginWriteTransaction();
			}
		}

		void rollback() {
			if (db != nullptr)
				db->rollbackTransaction();
			db = nullptr;
		}

		void end() {
			if (db != nullptr) db->endTransaction();
			db = nullptr;
		}
	private:
		Database* db;
	};

	class ScopedManualCommitWriteTransaction
	{
	public:
		ScopedManualCommitWriteTransaction(Database* db = nullptr)
			: db(db) {
			if (db != nullptr) db->beginWriteTransaction();
		}
		~ScopedManualCommitWriteTransaction() {
			if (db != nullptr) db->rollbackTransaction();
		}

		void reset(Database* pdb)
		{
			if (db != nullptr) {
				db->rollbackTransaction();
			}
			db = pdb;
			if (db != nullptr) {
				db->beginWriteTransaction();
			}
		}

		void restart() {
			if (db != nullptr) {
				db->rollbackTransaction();
				db->beginWriteTransaction();
			}
		}

		void rollback() {
			if (db != nullptr)
				db->rollbackTransaction();
			db = nullptr;
		}

		void commit() {
			if (db != nullptr) db->endTransaction();
			db = nullptr;
		}
	private:
		Database* db;
	};

	class ScopedSynchronous
	{
	public:
		ScopedSynchronous(Database* pdb = nullptr)
			:db(nullptr)
		{
			reset(pdb);
		}

		~ScopedSynchronous()
		{
			if (db != nullptr)
			{
				db->write("PRAGMA synchronous = " + synchronous);
			}
		}

		void reset(Database* pdb)
		{
			if (db != nullptr)
			{
				db->write("PRAGMA synchronous = " + synchronous);
			}
			db = pdb;
			if (db != nullptr)
			{
				synchronous = db->read("PRAGMA synchronous")[0]["synchronous"];
				db->write("PRAGMA synchronous=FULL");
			}
		}

	private:
		Database* db;
		std::string synchronous;
	};

}