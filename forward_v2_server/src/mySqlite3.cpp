//
// Created by czx on 19-3-22.
//

#include "../include/mySqlite3.h"
//
// Created by deepglint on 19-3-8.
//

enum class tbl_field
{
    usr_id = 0,
    usr_name,
    psw,
    forbid,
    start_date,
    end_date
};
mySqlite3::mySqlite3()
{
    server = "localhost";
    user = "root";
    password = "7755120";
    database = "gfw.db";
    conn=nullptr;
}
mySqlite3::~mySqlite3()
{
}
int mySqlite3::connect()
{
    int rc=sqlite3_open(database.c_str(), &conn);
    if( rc )
    {
        printf("Can't open database: %s\n", sqlite3_errmsg(conn));
        return 0;
    }
    else
    {
        printf("Opened database successfully\n");
        return 1;
    }
}

int mySqlite3::query_expire(int usr_id)
{
    int ret;
    std::string sql;
    sql="select * from usr_tale  where usr_id=";
    sql+=std::to_string(usr_id);
    connect();

    return 1;
}