//
// Created by deepglint on 19-3-8.
//

#ifndef GFW_MYSQL_H
#define GFW_MYSQL_H
#include <mysql/mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>


class mySQL
{
public:
    mySQL();
    ~mySQL();
    int connect();
    int query_expire(int usr_id);
private:
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;
    std::string server;
    std::string user;
    std::string password;
    std::string database;
};

#endif //GFW_MYSQL_H
