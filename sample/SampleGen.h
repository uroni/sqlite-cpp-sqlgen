#pragma once

#include <vector>
#include <string>
#include "../DatabaseQuery.h"

class Users
{
    sqlgen::Database& db;
public:
    Users(sqlgen::Database& db) : db(db) {}

    //@-SQLGenFunctionsBegin
	struct User
	{
		bool exists;
		int64_t id;
		std::string name;
		std::string password;
	};


	std::vector<User> getUsers();
	User getUserById(int64_t id);
	User getUserByName(const std::string& name);
	int64_t addUser(const std::string& name, const std::string& password);
	void deleteUser(int64_t id);
	//@-SQLGenFunctionsEnd

private:
    //@-SQLGenVariablesBegin
	sqlgen::DatabaseQuery _getUsers;
	sqlgen::DatabaseQuery _getUserById;
	sqlgen::DatabaseQuery _getUserByName;
	sqlgen::DatabaseQuery _addUser;
	sqlgen::DatabaseQuery _deleteUser;
	//@-SQLGenVariablesEnd
};