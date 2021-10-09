#pragma once
#include <string>

namespace sqlgen
{
	class Database;

	void sqlgen_main(Database& db, std::string& cppfile, std::string& headerfile);
}