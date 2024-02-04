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
		virtual void endTransaction(void);
		virtual void rollbackTransaction();

		virtual DatabaseQuery prepare(std::string pQuery);

		virtual long long int getLastInsertID(void);

		sqlite3* getDatabase(void);

		bool isInTransaction(void);

		virtual std::string getEngineName(void);

		virtual void detachDBs(void);
		virtual void attachDBs(void);

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

}