/**
 * Copyright (C) Martin Raiber
 * SPDX-License-Identifier: Apache-2.0.
 */

#include "DatabaseQuery.h"
#include "sqlite/sqlite3.h"
#include "Database.h"
#include "DatabaseCursor.h"
#include <memory.h>
#include <algorithm>
#include <thread>
#include <chrono>
#include "stringtools.h"
#include "DatabaseLogger.h"

using namespace std::chrono_literals;
using namespace sqlgen;

DatabaseQuery::DatabaseQuery(const std::string &pStmt_str, sqlite3_stmt *prepared_statement, Database *pDB)
	: stmt_str(pStmt_str)
{
	ps=prepared_statement;
	curr_idx=1;
	db=pDB;
}

DatabaseQuery::~DatabaseQuery()
{
	int err=sqlite3_finalize(ps);
	if( err!=SQLITE_OK && err!=SQLITE_BUSY && err!=SQLITE_IOERR_BLOCKED )
		getDatabaseLogger()->Log("SQL: "+(std::string)sqlite3_errmsg(db->getDatabase())+ " Stmt: ["+stmt_str+"]", LL_ERROR);
}


void DatabaseQuery::Bind(const std::string &str)
{
	int err=sqlite3_bind_text(ps, curr_idx, str.c_str(), (int)str.size(), SQLITE_TRANSIENT);
	if( err!=SQLITE_OK )
		getDatabaseLogger()->Log("Error binding text to DatabaseQuery  Stmt: ["+stmt_str+"]", LL_ERROR);
	++curr_idx;
}

void DatabaseQuery::Bind(const char* buffer, unsigned int bsize)
{
	int err=sqlite3_bind_blob(ps, curr_idx, buffer, bsize, SQLITE_TRANSIENT);
	if( err!=SQLITE_OK )
		getDatabaseLogger()->Log("Error binding blob to DatabaseQuery  Stmt: ["+stmt_str+"]", LL_ERROR);
	++curr_idx;
}

void DatabaseQuery::Bind(int p)
{
	int err=sqlite3_bind_int(ps, curr_idx, p);
	if( err!=SQLITE_OK )
		getDatabaseLogger()->Log("Error binding int to DatabaseQuery  Stmt: ["+stmt_str+"]", LL_ERROR);
	++curr_idx;
}

void DatabaseQuery::Bind(unsigned int p)
{
	Bind((unsigned long long)p);
}

void DatabaseQuery::Bind(double p)
{
	int err=sqlite3_bind_double(ps, curr_idx, p);
	if( err!=SQLITE_OK )
		getDatabaseLogger()->Log("Error binding double to DatabaseQuery  Stmt: ["+stmt_str+"]", LL_ERROR);
	++curr_idx;
}

void DatabaseQuery::Bind(long long int p)
{
	int err=sqlite3_bind_int64(ps, curr_idx, p);
	if( err!=SQLITE_OK )
		getDatabaseLogger()->Log("Error binding int64 to DatabaseQuery  Stmt: ["+stmt_str+"]", LL_ERROR);
	++curr_idx;
}

#if defined(_WIN64) || defined(_LP64)
void DatabaseQuery::Bind(size_t p)
{
	Bind((long long int)p);
}
#endif

void DatabaseQuery::Reset(void)
{
	sqlite3_reset(ps);
	//sqlite3_clear_bindings(ps);
	curr_idx=1;
}

bool DatabaseQuery::Write(int timeoutms)
{
	return Execute(timeoutms);
}

bool DatabaseQuery::Execute(int timeoutms)
{
	//getDatabaseLogger()->Log("Write: "+stmt_str);

	int tries=60; //10min
	if(timeoutms>=0)
	{
		sqlite3_busy_timeout(db->getDatabase(), timeoutms);
	}
	int err=sqlite3_step(ps);
	while( err==SQLITE_IOERR_BLOCKED 
			|| err==SQLITE_BUSY 
			|| err==SQLITE_PROTOCOL 
			|| err==SQLITE_ROW 
			|| err==SQLITE_LOCKED )
	{
		if(err==SQLITE_IOERR_BLOCKED)
		{
			getDatabaseLogger()->Log("SQlite is blocked!", LL_ERROR);
		}
		if(err==SQLITE_BUSY 
			|| err==SQLITE_IOERR_BLOCKED
			|| err==SQLITE_PROTOCOL )
		{
			if(timeoutms>=0)
			{
				break;
			}
			else
			{
				sqlite3_reset(ps);
				--tries;
				if(tries==-1)
				{
				  getDatabaseLogger()->Log("SQLITE: Long running query  Stmt: ["+stmt_str+"]", LL_ERROR);
				}
				else if(tries>=0)
				{
				    getDatabaseLogger()->Log("SQLITE_BUSY in DatabaseQuery::Execute  Stmt: ["+stmt_str+"]", LL_INFO);
				}
			}
		}
		else if(err==SQLITE_LOCKED)
		{
			getDatabaseLogger()->Log("DEADLOCK in DatabaseQuery::Execute  Stmt: ["+stmt_str+"]", LL_ERROR);
			std::this_thread::sleep_for(1s);
			if(timeoutms>=0)
			{
				timeoutms-=1000;

				if(timeoutms<=0)
					break;
			}
		}
		err=sqlite3_step(ps);
	}

	if(timeoutms>=0)
	{
		sqlite3_busy_timeout(db->getDatabase(), c_sqlite_busy_timeout_default);
	}

	//getDatabaseLogger()->Log("Write done: "+stmt_str);
	if( err!=SQLITE_DONE )
	{
		if(timeoutms<0)
		{
			getDatabaseLogger()->Log("Error in DatabaseQuery::Execute - "+(std::string)sqlite3_errmsg(db->getDatabase()) +"  Stmt: ["+stmt_str+"]", LL_ERROR);
		}
		return false;
	}

	return true;
}

void DatabaseQuery::setupStepping(int *timeoutms)
{
	if(timeoutms!=NULL && *timeoutms>=0)
	{
		sqlite3_busy_timeout(db->getDatabase(), *timeoutms);
	}
}

void DatabaseQuery::shutdownStepping(int err, int *timeoutms)
{
	if(timeoutms!=NULL && *timeoutms>=0)
	{
		sqlite3_busy_timeout(db->getDatabase(), c_sqlite_busy_timeout_default);
	}

	if (err == SQLITE_ROW)
	{
		sqlite3_reset(ps);
	}

	if(timeoutms!=NULL)
	{
		if(err!=SQLITE_DONE)
		{
			*timeoutms=1;
		}
		else
		{
			*timeoutms=0;
		}
	}
}

db_results DatabaseQuery::Read(int *timeoutms)
{
	int err;
	db_results rows;
	int tries=60; //10min

	setupStepping(timeoutms);

	db_single_result res;
	do
	{
		bool reset=false;
		err=step(&res, timeoutms, tries, reset);
		if(reset)
		{
			rows.clear();
		}
		if(err==SQLITE_ROW)
		{
			rows.push_back(res);
			res.clear();
		}
	}
	while(resultOkay(err));

	shutdownStepping(err, timeoutms);

	return rows;
}

bool DatabaseQuery::resultOkay(int rc)
{
	return  rc==SQLITE_BUSY ||
			rc==SQLITE_PROTOCOL ||
			rc==SQLITE_ROW ||
			rc==SQLITE_IOERR_BLOCKED;
}

std::string sqlgen::DatabaseQuery::ustring_sqlite3_column_name(int N)
{
	const char* c_name = sqlite3_column_name(ps, N);
	if (c_name == NULL)
	{
		return std::string();
	}

	return std::string(c_name);
}

int DatabaseQuery::step(db_single_result* res, int *timeoutms, int& tries, bool& reset)
{
	int err=sqlite3_step(ps);
	if( resultOkay(err) )
	{
		if( err==SQLITE_BUSY 
			|| err==SQLITE_PROTOCOL
			|| err==SQLITE_IOERR_BLOCKED )
		{
			if(timeoutms!=NULL && *timeoutms>=0)
			{
				return SQLITE_ABORT;
			}
			else
			{
				sqlite3_reset(ps);
				reset=true;
				--tries;
				if(tries==-1)
				{
				  getDatabaseLogger()->Log("SQLITE: Long runnning query  Stmt: ["+stmt_str+"]", LL_ERROR);
				}
				else
				{
					getDatabaseLogger()->Log("SQLITE_BUSY in DatabaseQuery::Read  Stmt: ["+stmt_str+"]", LL_INFO);
				}
			}
		}
		else if( err==SQLITE_ROW )
		{
			if (res != nullptr)
			{
				int column = 0;
				std::string column_name;
				while (!(column_name = ustring_sqlite3_column_name(column)).empty())
				{
					const void* data;
					int data_size;
					if (sqlite3_column_type(ps, column) == SQLITE_BLOB)
					{
						data = sqlite3_column_blob(ps, column);
						data_size = sqlite3_column_bytes(ps, column);
					}
					else
					{
						data = sqlite3_column_text(ps, column);
						data_size = sqlite3_column_bytes(ps, column);
					}
					std::string datastr(reinterpret_cast<const char*>(data), reinterpret_cast<const char*>(data) + data_size);
					res->insert(std::pair<std::string, std::string>(column_name, datastr));
					++column;
				}
			}
		}
		else
		{
			std::this_thread::sleep_for(1s);
			if(timeoutms!=NULL && *timeoutms>=0)
			{
				*timeoutms-=1000;

				if(*timeoutms<=0)
				{
					return SQLITE_ABORT;
				}
			}
		}
	}
	return err;
}

DatabaseCursor* DatabaseQuery::Cursor(int *timeoutms)
{
	if(!cursor)
	{
		cursor= std::make_unique<DatabaseCursor>(this, timeoutms);
	}
	else
	{
		if (!cursor->reset())
			return nullptr;
	}

	return cursor.get();
}

std::string DatabaseQuery::getStatement(void)
{
	return stmt_str;
}

std::string DatabaseQuery::getErrMsg(void)
{
	return std::string(sqlite3_errmsg(db->getDatabase()));
}