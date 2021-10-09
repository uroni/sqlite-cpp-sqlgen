/**
 * Copyright (C) Martin Raiber
 * SPDX-License-Identifier: Apache-2.0.
 */

#include "Query.h"
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
	destroyAllQueries();
	for(std::map<int, DatabaseQuery*>::iterator iter=prepared_queries.begin();iter!=prepared_queries.end();++iter)
	{
		DatabaseQuery *q=(DatabaseQuery*)iter->second;
		delete q;
	}
	prepared_queries.clear();

	sqlite3_close(db);
}

bool Database::Open(std::string pFile, const std::vector<std::pair<std::string,std::string> > &attach,
	size_t allocation_chunk_size, const str_map& p_params)
{
	params = p_params;

	attached_dbs=attach;
	in_transaction=false;
	if( sqlite3_open(pFile.c_str(), &db) )
	{
		getDatabaseLogger()->Log("Could not open db ["+pFile+"]");
		sqlite3_close(db);
		db = NULL;
		return false;
	}
	else
	{
		str_map::const_iterator it = params.find("synchronous");
		if (it != params.end())
		{
			Write("PRAGMA synchronous="+it->second);
		}
		else
		{
			Write("PRAGMA synchronous=NORMAL");
		}
		Write("PRAGMA foreign_keys = ON");
		Write("PRAGMA threads = 2");

		it = params.find("wal_autocheckpoint");
		if (it != params.end())
		{
			Write("PRAGMA wal_autocheckpoint=" + it->second);

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
			Write("PRAGMA page_size=" + it->second);
		}
		else
		{
			Write("PRAGMA page_size=4096");
		}

		it = params.find("mmap_size");
		if (it != params.end())
		{
			Write("PRAGMA mmap_size=" + it->second);
		}

		if(allocation_chunk_size!=std::string::npos)
		{
			int chunk_size = static_cast<int>(allocation_chunk_size);
			sqlite3_file_control(db, NULL, SQLITE_FCNTL_CHUNK_SIZE, &chunk_size);
		}

		static size_t sqlite_cache_size = get_sqlite_cache_size();
		Write("PRAGMA cache_size = -"+std::to_string(sqlite_cache_size));	

		sqlite3_busy_timeout(db, c_sqlite_busy_timeout_default);

		AttachDBs();

		return true;
	}
}

void Database::initMutex(void)
{
	sqlite3_config(SQLITE_CONFIG_LOG, errorLogCallback, NULL);
}

void Database::destroyMutex(void)
{
}

db_results Database::Read(std::string pQuery)
{
	DatabaseQuery*q=Prepare(pQuery, false);
	if(q!=NULL)
	{
		db_results ret=q->Read();
		delete ((DatabaseQuery*)q);
		return ret;
	}
	return db_results();
}

bool Database::Write(std::string pQuery)
{
	DatabaseQuery*q=Prepare(pQuery, false);
	if(q!=NULL)
	{
		bool b=q->Write();
		delete ((DatabaseQuery*)q);
		return b;
	}
	else
	{
		return false;
	}
}


bool Database::BeginReadTransaction()
{
	in_transaction = true;
	if(Write("BEGIN"))
	{
		return true;
	}
	else
	{
		in_transaction = false;
		return false;
	}
}

bool Database::BeginWriteTransaction()
{
	in_transaction = true;
	if(Write("BEGIN IMMEDIATE;"))
	{
		return true;
	}
	else
	{
		in_transaction = false;
		return false;
	}
}

bool Database::EndTransaction(void)
{
	bool ret = Write("END;");
	in_transaction=false;
	return ret;
}

bool Database::RollbackTransaction()
{
	bool ret = Write("ROLLBACK;");
	in_transaction = false;
	return ret;
}

DatabaseQuery* Database::Prepare(std::string pQuery, bool autodestroy)
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
		getDatabaseLogger()->Log("Error preparing Query ["+pQuery+"]: "+sqlite3_errmsg(db),LL_ERROR);
		return NULL;
	}
	DatabaseQuery* q=new DatabaseQuery(pQuery, prepared_statement, this);
	if( autodestroy )
	{
		queries.push_back(q);
	}

	return q;
}

DatabaseQuery* Database::Prepare(int id, std::string pQuery)
{
	std::map<int, DatabaseQuery*>::iterator iter=prepared_queries.find(id);
	if( iter!=prepared_queries.end() )
	{
		iter->second->Reset();
		return iter->second;
	}
	else
	{
		DatabaseQuery *q=Prepare(pQuery, false);
		prepared_queries.insert(std::pair<int, DatabaseQuery*>(id, q) );
		return q;
	}
}

void Database::destroyQuery(DatabaseQuery *q)
{
	if(q==NULL)
	{
		return;
	}

	for(size_t i=0;i<queries.size();++i)
	{
		if( queries[i]==q )
		{
			DatabaseQuery *cq=(DatabaseQuery*)q;
			delete cq;
			queries.erase( queries.begin()+i);
			return;
		}
	}
	DatabaseQuery*cq=(DatabaseQuery*)q;
	delete cq;
}

void Database::destroyAllQueries(void)
{
	for(size_t i=0;i<queries.size();++i)
	{
		DatabaseQuery* cq=(DatabaseQuery*)queries[i];
		delete cq;
	}
	queries.clear();
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

void Database::AttachDBs(void)
{
	for(size_t i=0;i<attached_dbs.size();++i)
	{
		Write("ATTACH DATABASE '"+attached_dbs[i].first+"' AS "+attached_dbs[i].second);

		str_map::const_iterator it = params.find("synchronous");
		if (it != params.end())
		{
			Write("PRAGMA "+ attached_dbs[i].second+".synchronous=" + it->second);
		}
		else
		{
			Write("PRAGMA "+ attached_dbs[i].second+".synchronous=NORMAL");
		}
	}
}

void Database::DetachDBs(void)
{
	for(size_t i=0;i<attached_dbs.size();++i)
	{
		Write("DETACH DATABASE "+attached_dbs[i].second);
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