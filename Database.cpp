/**
 * Copyright (C) Martin Raiber
 * SPDX-License-Identifier: Apache-2.0.
 */

#include <utility>
#include "sqlite/sqlite3.h"
#include <stdlib.h>
#include "Database.h"
#include "stringtools.h"
#include "DatabaseLogger.h"
#include "DatabaseQuery.h"

using namespace sqlgen;

namespace
{
	size_t get_sqlite_cache_size()
	{
		return 2*1024; //2MB
	}

	void errorLogCallback(void *pArg, int iErrCode, const char *zMsg)
	{
		switch (iErrCode)
		{
		case SQLITE_LOCKED:
		case SQLITE_BUSY:
		case SQLITE_SCHEMA:
			return;
		case SQLITE_NOTICE_RECOVER_ROLLBACK:
		case SQLITE_NOTICE_RECOVER_WAL:
			getDatabaseLogger()->Log("SQLite: "+ std::string(zMsg) + " code: " + std::to_string(iErrCode), LL_INFO);
			break;
		default:
			getDatabaseLogger()->Log("SQLite: " + std::string(zMsg) + " errorcode: " + std::to_string(iErrCode), LL_WARNING);
			break;
		}
	}
}

Database::~Database()
{
	sqlite3_close(db);
}

Database::Database(Database&& other)
{
	*this = std::move(other);
}

Database& Database::operator=(Database &&other)
{
	db = std::exchange(other.db, nullptr),
	in_transaction = other.in_transaction,
	attached_dbs = std::move(other.attached_dbs);
	params = std::move(other.params);
	return *this;
}

bool initLogging()
{
	sqlite3_config(SQLITE_CONFIG_LOG, errorLogCallback, NULL);
	return true;
}

Database::Database(const std::string& pFile, std::vector<std::pair<std::string,std::string> > attach,
	size_t allocation_chunk_size, str_map p_params)
	: params(std::move(p_params)), attached_dbs(std::move(attach))
{
	static auto initRes = initLogging();
	if(!initRes)
	{
		throw DatabaseOpenError("Static init failed");
	}

	if( sqlite3_open(pFile.c_str(), &db) )
	{
		getDatabaseLogger()->Log("Could not open db ["+pFile+"]");
		throw DatabaseOpenError("Could not open db ["+pFile+"]");
	}
	else
	{
		str_map::const_iterator it = params.find("synchronous");
		if (it != params.end())
		{
			write("PRAGMA synchronous="+it->second);
		}
		else
		{
			write("PRAGMA synchronous=NORMAL");
		}
		write("PRAGMA foreign_keys = ON");
		write("PRAGMA threads = 2");

		it = params.find("wal_autocheckpoint");
		if (it != params.end())
		{
			write("PRAGMA wal_autocheckpoint=" + it->second);

			if (atoi(it->second.c_str())<=0)
			{
				int enable = 1;
				sqlite3_file_control(db, NULL, SQLITE_FCNTL_PERSIST_WAL, &enable);
#ifdef SQLITE_DBCONFIG_NO_CKPT_ON_CLOSE
				int was_enabled = 0;
				sqlite3_db_config(db, SQLITE_DBCONFIG_NO_CKPT_ON_CLOSE, 1, &was_enabled);
#endif
			}
		}

		it = params.find("page_size");
		if (it != params.end())
		{
			write("PRAGMA page_size=" + it->second);
		}
		else
		{
			write("PRAGMA page_size=4096");
		}

		it = params.find("mmap_size");
		if (it != params.end())
		{
			write("PRAGMA mmap_size=" + it->second);
		}

		if(allocation_chunk_size!=std::string::npos)
		{
			int chunk_size = static_cast<int>(allocation_chunk_size);
			sqlite3_file_control(db, NULL, SQLITE_FCNTL_CHUNK_SIZE, &chunk_size);
		}

		static size_t sqlite_cache_size = get_sqlite_cache_size();
		write("PRAGMA cache_size = -"+std::to_string(sqlite_cache_size));	

		sqlite3_busy_timeout(db, c_sqlite_busy_timeout_default);

		attachDBs();
	}
}


db_results Database::read(const std::string& query)
{
	return prepare(query).read();
}

void Database::write(const std::string& query)
{
	prepare(query).write();
}


void Database::beginReadTransaction()
{
	write("BEGIN");
	in_transaction = true;
}

void Database::beginWriteTransaction()
{
	write("BEGIN IMMEDIATE");
	in_transaction = true;
}

void Database::endTransaction()
{
	write("END;");
	in_transaction=false;
}

void Database::rollbackTransaction()
{
	write("ROLLBACK");
}

DatabaseQuery Database::prepare(std::string pQuery)
{
	int prepare_tries = 0;
#ifdef SQLITE_PREPARE_RETRIES
	prepare_tries = SQLITE_PREPARE_RETRIES;
#endif

	sqlite3_stmt *prepared_statement;
	const char* tail;
	int err;
	bool reset_busy = false;
	while((err=sqlite3_prepare_v2(db, pQuery.c_str(), (int)pQuery.size(), &prepared_statement, &tail) )==SQLITE_LOCKED 
		|| err==SQLITE_BUSY
		|| err==SQLITE_PROTOCOL
		|| (err!=SQLITE_OK && prepare_tries>0) )
	{
		--prepare_tries;

		if(err==SQLITE_LOCKED)
		{
			getDatabaseLogger()->Log("DATABASE DEADLOCKED in Database::Prepare", LL_ERROR);
		}
		else if(err== SQLITE_BUSY
			|| err==SQLITE_PROTOCOL)
		{
			sqlite3_busy_timeout(db, 10000);
			reset_busy = true;
		}
		else
		{
			getDatabaseLogger()->Log("Error preparing Query [" + pQuery + "]: " + sqlite3_errmsg(db)+". Retrying...", LL_ERROR);
		}
	}

	if(reset_busy)
	{
		sqlite3_busy_timeout(db, 50);
	}

	if( err!=SQLITE_OK )
	{
		const auto msg = "Error preparing Query ["+pQuery+"]: "+sqlite3_errmsg(db);
		getDatabaseLogger()->Log(msg,LL_ERROR);
		throw PrepareError(msg);
	}

	return DatabaseQuery(pQuery, prepared_statement, this);
}

long long int Database::getLastInsertID(void)
{
	return sqlite3_last_insert_rowid(db);
}

sqlite3 *Database::getDatabase(void)
{
	return db;
}

bool Database::isInTransaction(void)
{
	return in_transaction;
}

std::string Database::getEngineName(void)
{
	return "sqlite";
}

void Database::attachDBs(void)
{
	for(size_t i=0;i<attached_dbs.size();++i)
	{
		write("ATTACH DATABASE '"+attached_dbs[i].first+"' AS "+attached_dbs[i].second);

		str_map::const_iterator it = params.find("synchronous");
		if (it != params.end())
		{
			write("PRAGMA "+ attached_dbs[i].second+".synchronous=" + it->second);
		}
		else
		{
			write("PRAGMA "+ attached_dbs[i].second+".synchronous=NORMAL");
		}
	}
}

void Database::detachDBs(void)
{
	for(size_t i=0;i<attached_dbs.size();++i)
	{
		write("DETACH DATABASE "+attached_dbs[i].second);
	}
}

void Database::freeMemory()
{
	sqlite3_db_release_memory(db);
}

int Database::getLastChanges()
{
	return sqlite3_changes(db);
}

std::string Database::getTempDirectoryPath()
{
	char* tmpfn = NULL;
	if(sqlite3_file_control(db, NULL, SQLITE_FCNTL_TEMPFILENAME, &tmpfn)==SQLITE_OK && tmpfn!=NULL)
	{
		std::string ret = ExtractFilePath(tmpfn);
		sqlite3_free(tmpfn);
		return ret;
	}
	else
	{
		return std::string();
	}
}