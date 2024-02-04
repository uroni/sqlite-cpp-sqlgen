#include "SampleGen.h"
#include "../stringtools.h"
#include <assert.h>

/**
* @-SQLGenAccess
* @func vector<User> Users::getUsers
* @return int64 id, string name, string password
* @sql
*      SELECT id, name, password FROM users
*/
std::vector<Users::User> Users::getUsers()
{
	if(!q_getUsers)
	{
		q_getUsers=std::make_unique<sqlgen::DatabaseQuery>(db.prepare("SELECT id, name, password FROM users"));
	}
	sqlgen::db_results res=q_getUsers->read();
	std::vector<Users::User> ret;
	ret.resize(res.size());
	for(size_t i=0;i<res.size();++i)
	{
		ret[i].id=std::atoll(res[i]["id"].c_str());
		ret[i].name=res[i]["name"];
		ret[i].password=res[i]["password"];
	}
	return ret;
}

/**
* @-SQLGenAccess
* @func User Users::getUserById
* @return int64 id, string name, string password
* @sql
*      SELECT id, name, password FROM users WHERE id=:id(int64)
*/
Users::User Users::getUserById(int64_t id)
{
	if(!q_getUserById)
	{
		q_getUserById=std::make_unique<sqlgen::DatabaseQuery>(db.prepare("SELECT id, name, password FROM users WHERE id=?"));
	}
	q_getUserById->bind(id);
	sqlgen::db_results res=q_getUserById->read();
	q_getUserById->reset();
	User ret = { false, 0, "", "" };
	if(!res.empty())
	{
		ret.exists=true;
		ret.id=std::atoll(res[0]["id"].c_str());
		ret.name=res[0]["name"];
		ret.password=res[0]["password"];
	}
	return ret;
}

/**
* @-SQLGenAccess
* @func User Users::getUserByName
* @return int64 id, string name, string password
* @sql
*      SELECT id, name, password FROM users WHERE name=:name(string)
*/
Users::User Users::getUserByName(const std::string& name)
{
	if(!q_getUserByName)
	{
		q_getUserByName=std::make_unique<sqlgen::DatabaseQuery>(db.prepare("SELECT id, name, password FROM users WHERE name=?"));
	}
	q_getUserByName->bind(name);
	sqlgen::db_results res=q_getUserByName->read();
	q_getUserByName->reset();
	User ret = { false, 0, "", "" };
	if(!res.empty())
	{
		ret.exists=true;
		ret.id=std::atoll(res[0]["id"].c_str());
		ret.name=res[0]["name"];
		ret.password=res[0]["password"];
	}
	return ret;
}

/**
* @-SQLGenAccess
* @func int64_t Users::addUser
* @return int64_raw id
* @sql
*      INSERT INTO users (name, password) VALUES (:name(string), :password(string)) RETURNING id
*/
int64_t Users::addUser(const std::string& name, const std::string& password)
{
	if(!q_addUser)
	{
		q_addUser=std::make_unique<sqlgen::DatabaseQuery>(db.prepare("INSERT INTO users (name, password) VALUES (?, ?) RETURNING id"));
	}
	q_addUser->bind(name);
	q_addUser->bind(password);
	sqlgen::db_results res=q_addUser->read();
	q_addUser->reset();
	assert(!res.empty());
	return std::atoll(res.at(0)["id"].c_str());
}

/**
* @-SQLGenAccess
* @func void Users::deleteUser
* @sql
*      DELETE FROM users WHERE id=:id(int64)
*/
void Users::deleteUser(int64_t id)
{
	if(!q_deleteUser)
	{
		q_deleteUser=std::make_unique<sqlgen::DatabaseQuery>(db.prepare("DELETE FROM users WHERE id=?"));
	}
	q_deleteUser->bind(id);
	q_deleteUser->write();
	q_deleteUser->reset();
}


// eof