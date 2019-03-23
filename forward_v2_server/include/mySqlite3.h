//
// Created by czx on 19-3-22.
//

#ifndef GFW_SQLITE3_H
#define GFW_SQLITE3_H
#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <string>
class mySqlite3
{
public:
    mySqlite3();
    ~mySqlite3();
    int connect();
    int query_expire(int usr_id);
private:
    sqlite3 *conn;
    std::string server;
    std::string user;
    std::string password;
    std::string database;
};
#endif //GFW_SQLITE3_H
