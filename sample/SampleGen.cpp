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
	if(!_getUsers.prepared())
	{
		_getUsers=db.prepare("SELECT id, name, password FROM users");
	}
	auto& cursor=_getUsers.cursor();
	std::vector<Users::User> ret;
	while(cursor.next())
	{
		ret.emplace_back();
		Users::User& obj=ret.back();
		cursor.get(0, obj.id);
		cursor.get(1, obj.name);
		cursor.get(2, obj.password);
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
	if(!_getUserById.prepared())
	{
		_getUserById=db.prepare("SELECT id, name, password FROM users WHERE id=?");
	}
	_getUserById.bind(id);
	auto& cursor=_getUserById.cursor();
	User ret = { false, 0, "", "" };
	if(cursor.next())
	{
		ret.exists=true;
		cursor.get(0, ret.id);
		cursor.get(1, ret.name);
		cursor.get(2, ret.password);
	}
	_getUserById.reset();
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
	if(!_getUserByName.prepared())
	{
		_getUserByName=db.prepare("SELECT id, name, password FROM users WHERE name=?");
	}
	_getUserByName.bind(name);
	auto& cursor=_getUserByName.cursor();
	User ret = { false, 0, "", "" };
	if(cursor.next())
	{
		ret.exists=true;
		cursor.get(0, ret.id);
		cursor.get(1, ret.name);
		cursor.get(2, ret.password);
	}
	_getUserByName.reset();
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
	if(!_addUser.prepared())
	{
		_addUser=db.prepare("INSERT INTO users (name, password) VALUES (?, ?) RETURNING id");
	}
	_addUser.bind(name);
	_addUser.bind(password);
	auto& cursor=_addUser.cursor();
	assert(cursor.next());
	int64_t ret;
	cursor.get(0, ret);
	_addUser.reset();
	return ret;
}

/**
* @-SQLGenAccess
* @func void Users::deleteUser
* @sql
*      DELETE FROM users WHERE id=:id(int64)
*/
void Users::deleteUser(int64_t id)
{
	if(!_deleteUser.prepared())
	{
		_deleteUser=db.prepare("DELETE FROM users WHERE id=?");
	}
	_deleteUser.bind(id);
	_deleteUser.write();
	_deleteUser.reset();
}


// eof