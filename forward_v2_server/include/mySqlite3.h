//
// Created by czx on 19-3-22.
//

#ifndef GFW_SQLITE3_H
#define GFW_SQLITE3_H
#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
class mySqlite3
{
public:
    mySqlite3();
    ~mySqlite3();
    int connect();
    int query_expire(int usr_id,int &dev_limit);
private:
	static int callback(void *data, int argc, char **argv, char **azColName);
    sqlite3 *conn;
    std::string server;
    std::string user;
    std::string password;
    std::string database;
    std::mutex mutex_;
    std::condition_variable cond_;
    int expire ;
    char dev;

};
#endif //GFW_SQLITE3_H
