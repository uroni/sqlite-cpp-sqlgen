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
    //@-SQLGenFunctionsEnd

private:
    //@-SQLGenVariablesBegin
    //@-SQLGenVariablesEnd
};