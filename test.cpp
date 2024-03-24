#include "test.h"
#include "Database.h"
#include "sample/SampleGen.h"
#include <iostream>

using namespace sqlgen;

int test()
{
    std::cout << "TEST" << std::endl;

    Database db("sample/samplegen.db");
    Users users(db);

    std::cout << "Users:" << std::endl;
    for(const auto& user: users.getUsers())
    {
        std::cout << "id=" << user.id << " name=" << user.name << " password=" << user.password << std::endl;
    }

    auto id = users.addUser("test", "foo");
    std::cout << "Added user test with id " << id << std::endl;

    std::cout << "Users:" << std::endl;
    for(const auto& user: users.getUsers())
    {
        std::cout << "id=" << user.id << " name=" << user.name << " password=" << user.password << std::endl;
    }

    return 0;
}