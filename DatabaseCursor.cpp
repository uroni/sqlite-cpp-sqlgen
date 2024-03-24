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
}

DatabaseCursor::~DatabaseCursor(void)
{
}

bool DatabaseCursor::reset()
{
	tries = 60;
	lastErr = SQLITE_OK;
	_has_error = false;
	is_shutdown = false;

	query->setupStepping(timeoutms);
	return true;
}

bool DatabaseCursor::next(db_single_result* res)
{
	if (res != nullptr)
		res->clear();

	do
	{
		bool reset = false;
		lastErr = query->step(res, timeoutms, tries, reset);
		//TODO handle reset (should not happen in WAL mode)
		if (lastErr == SQLITE_ROW)
		{
			return true;
		}
	} while (query->resultOkay(lastErr));

	if (lastErr != SQLITE_DONE)
	{
		getDatabaseLogger()->Log("SQL Error: " + query->getErrMsg() + " Stmt: [" + query->getStatement() + "]", LL_ERROR);
		_has_error = true;
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
	}
}

bool DatabaseCursor::get(int col, std::string& v)
{
	sqlite3_stmt* ps = query->getSQliteStmt();
	int col_type = sqlite3_column_type(ps, col);
	const void* data;
	int data_size;
	if (col_type == SQLITE_BLOB)
	{
		data = sqlite3_column_blob(ps, col);
		data_size = sqlite3_column_bytes(ps, col);
	}
	else
	{
		data = sqlite3_column_text(ps, col);
		data_size = sqlite3_column_bytes(ps, col);
	}
	if (data == 0)
	{
		v.clear();
		return false;
	}
	v.assign(reinterpret_cast<const char*>(data), reinterpret_cast<const char*>(data) + data_size);
	return col_type == SQLITE_BLOB || col_type == SQLITE_TEXT;
}

bool DatabaseCursor::get(int col, int& v)
{
	sqlite3_stmt* ps = query->getSQliteStmt();
	int col_type = sqlite3_column_type(ps, col);
	v = sqlite3_column_int(ps, col);
	return col_type == SQLITE_INTEGER;
}

bool DatabaseCursor::get(int col, int64_t& v)
{
	sqlite3_stmt* ps = query->getSQliteStmt();
	int col_type = sqlite3_column_type(ps, col);
	v = sqlite3_column_int64(ps, col);
	return col_type == SQLITE_INTEGER;
}

bool DatabaseCursor::get(int col, double& v)
{
	sqlite3_stmt* ps = query->getSQliteStmt();
	int col_type = sqlite3_column_type(ps, col);
	v = sqlite3_column_double(ps, col);
	return col_type == SQLITE_FLOAT;
}

void DatabaseCursor::init_col_names()
{
	int column = 0;
	std::string column_name;
	while (!(column_name = query->ustring_sqlite3_column_name(column)).empty())
	{
		col_names[column_name] = column;
		++column;
	}
}

int DatabaseCursor::get_col_idx(const std::string& col)
{
	if (col_names.empty())
		init_col_names();

	auto col_it = col_names.find(col);
	if (col_it == col_names.end())
		return -1;

	return col_it->second;
}

bool DatabaseCursor::get(const std::string& col, std::string& v)
{
	int col_idx = get_col_idx(col);
	if (col_idx < 0)
		return false;

	return get(col_idx, v);
}

bool DatabaseCursor::get(const std::string& col, int& v)
{
	int col_idx = get_col_idx(col);
	if (col_idx < 0)
		return false;

	return get(col_idx, v);
}

bool DatabaseCursor::get(const std::string& col, int64_t& v)
{
	int col_idx = get_col_idx(col);
	if (col_idx < 0)
		return false;

	return get(col_idx, v);
}

bool DatabaseCursor::get(const std::string& col, double& v)
{
	int col_idx = get_col_idx(col);
	if (col_idx < 0)
		return false;

	return get(col_idx, v);
}
