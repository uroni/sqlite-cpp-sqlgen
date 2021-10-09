/**
 * Copyright (C) Martin Raiber
 * SPDX-License-Identifier: Apache-2.0.
 */

#include <iostream>
#include <string>
#include <fstream>
#include "Database.h"
#include "sqlgen.h"
#include "stringtools.h"
#include "sqlgen_config.h"

using namespace sqlgen;

int main(int argc, char* argv[])
{
	if(argc<3)
	{
		std::cout << "SQLite SQLGen version " << SQLGEN_VERSION_MAJOR << "." << SQLGEN_VERSION_MINOR << std::endl;
		std::cout << "Usage: SQLGen [SQLite database filename] [cpp-file] ([Attached db name] [Attached db filename] ...)" << std::endl;
		return 1;
	}

	std::string sqlite_db_str=argv[1];
	std::string cppfile=argv[2];
	std::string headerfile=getuntil(".cpp", cppfile)+".h";

	std::vector<std::pair<std::string, std::string> > attach_dbs;
	for(int i=3;i+1<argc;i+=2)
	{
		attach_dbs.push_back(std::make_pair(argv[i + 1], argv[i]));
	}

	Database sqldb;
	str_map db_params;
	if (!sqldb.Open(sqlite_db_str, attach_dbs, std::string::npos, db_params))
	{
		std::cout << "Could not open sqlite db \"" << sqlite_db_str << "\"" << std::endl;
		return 2;
	}

	std::string cppfile_data=getFile(cppfile);
	std::string headerfile_data=getFile(headerfile);

	writestring(cppfile_data, cppfile+".sqlgenbackup");
	writestring(headerfile_data, headerfile+".sqlgenbackup");

	try
	{
		sqlgen::sqlgen_main(sqldb, cppfile_data, headerfile_data);
	}
	catch (std::exception& e)
	{
		std::cout << "Error: " << e.what() << std::endl;
		return 3;
	}

	writestring(cppfile_data, cppfile);
	writestring(headerfile_data, headerfile);

	std::cout << "SQLGen: Ok." << std::endl;

	return 0;
}