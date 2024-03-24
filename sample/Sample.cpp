#include "Sample.h"
#include "../stringtools.h"
#include <assert.h>

/**
* @-SQLGenAccess
* @func vector<User> Users::getUsers
* @return int64 id, string name, string password
* @sql
*      SELECT id, name, password FROM users
*/

/**
* @-SQLGenAccess
* @func User Users::getUserById
* @return int64 id, string name, string password
* @sql
*      SELECT id, name, password FROM users WHERE id=:id(int64)
*/

/**
* @-SQLGenAccess
* @func User Users::getUserByName
* @return int64 id, string name, string password
* @sql
*      SELECT id, name, password FROM users WHERE name=:name(string)
*/

/**
* @-SQLGenAccess
* @func int64_t Users::addUser
* @return int64_raw id
* @sql
*      INSERT INTO users (name, password) VALUES (:name(string), :password(string)) RETURNING id
*/

/**
* @-SQLGenAccess
* @func void Users::deleteUser
* @sql
*      DELETE FROM users WHERE id=:id(int64)
*/


// eof