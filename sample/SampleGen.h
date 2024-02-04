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
	std::unique_ptr<sqlgen::DatabaseQuery> q_getUsers;
	std::unique_ptr<sqlgen::DatabaseQuery> q_getUserById;
	std::unique_ptr<sqlgen::DatabaseQuery> q_getUserByName;
	std::unique_ptr<sqlgen::DatabaseQuery> q_addUser;
	std::unique_ptr<sqlgen::DatabaseQuery> q_deleteUser;
	//@-SQLGenVariablesEnd
};