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
	if(!q_getUsers.prepared())
	{
		q_getUsers=db.prepare("SELECT id, name, password FROM users");
	}
	auto& cursor=q_getUsers.cursor();
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
	if(!q_getUserById.prepared())
	{
		q_getUserById=db.prepare("SELECT id, name, password FROM users WHERE id=?");
	}
	q_getUserById.bind(id);
	auto& cursor=q_getUserById.cursor();
	User ret = { false, 0, "", "" };
	if(cursor.next())
	{
		ret.exists=true;
		cursor.get(0, ret.id);
		cursor.get(1, ret.name);
		cursor.get(2, ret.password);
	}
	q_getUserById.reset();
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
	if(!q_getUserByName.prepared())
	{
		q_getUserByName=db.prepare("SELECT id, name, password FROM users WHERE name=?");
	}
	q_getUserByName.bind(name);
	auto& cursor=q_getUserByName.cursor();
	User ret = { false, 0, "", "" };
	if(cursor.next())
	{
		ret.exists=true;
		cursor.get(0, ret.id);
		cursor.get(1, ret.name);
		cursor.get(2, ret.password);
	}
	q_getUserByName.reset();
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
	if(!q_addUser.prepared())
	{
		q_addUser=db.prepare("INSERT INTO users (name, password) VALUES (?, ?) RETURNING id");
	}
	q_addUser.bind(name);
	q_addUser.bind(password);
	auto& cursor=q_addUser.cursor();
	assert(cursor.next());
	int64_t ret;
	cursor.get("id", ret);
	q_addUser.reset();
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
	if(!q_deleteUser.prepared())
	{
		q_deleteUser=db.prepare("DELETE FROM users WHERE id=?");
	}
	q_deleteUser.bind(id);
	q_deleteUser.write();
	q_deleteUser.reset();
}


// eof