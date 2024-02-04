/**
 * Copyright (C) Martin Raiber
 * SPDX-License-Identifier: Apache-2.0.
 */

#include "DatabaseCursor.h"
#include "DatabaseQuery.h"
#include "sqlite/sqlite3.h"
#include "DatabaseLogger.h"
#include <utility>

using namespace sqlgen;

DatabaseCursor::DatabaseCursor(DatabaseQuery* query, int timeoutms)
	: query(query), tries(60), timeoutms(timeoutms),
	lastErr(SQLITE_OK), _has_error(false), is_shutdown(false)
{
	query->setupStepping(timeoutms);

#ifdef LOG_READ_QUERIES
	active_query=new ScopedAddActiveQuery(query);
#endif
}

DatabaseCursor::~DatabaseCursor(void)
{
	shutdown();
}

bool DatabaseCursor::reset()
{
	tries = 60;
	lastErr = SQLITE_OK;
	_has_error = false;
	is_shutdown = false;

	query->setupStepping(timeoutms);

#ifdef LOG_READ_QUERIES
	active_query = new ScopedAddActiveQuery(query);
#endif
	return true;
}

bool DatabaseCursor::next(db_single_result &res)
{
	res.clear();
	do
	{
		bool reset=false;
		lastErr=query->step(res, timeoutms, tries, reset);
		//TODO handle reset (should not happen in WAL mode)
		if(lastErr==SQLITE_ROW)
		{
			return true;
		}
	}
	while(query->resultOkay(lastErr));

	if(lastErr!=SQLITE_DONE)
	{
		getDatabaseLogger()->Log("SQL Error: "+query->getErrMsg()+ " Stmt: ["+query->getStatement()+"]", LL_ERROR);
		_has_error=true;
	}

	return false;
}

bool DatabaseCursor::hasError(void)
{
	return _has_error;
}

void DatabaseCursor::shutdown()
{
	if (!is_shutdown)
	{
		is_shutdown = true;
		query->shutdownStepping(lastErr, timeoutms);

#ifdef LOG_READ_QUERIES
		delete active_query;
#endif
	}
}
