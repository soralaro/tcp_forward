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

static int mySqlite3::callback(void *data, int argc, char **argv, char **azColName)
{
    int i;
    mySqlite3 *mysql=(mySqlite3 *)data;
    // for(i=0; i<argc; i++)
    // {
    //   printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    // }
    if(argc>6)
    {
        int forbid=atoi(argv[(int)(tbl_field::forbid)]);
        //printf("forbid=%d\n",forbid);
        if(forbid==1)
        {
            mysql->expire=1;
        }
        else
        {
            struct tm expire_time;
            strptime((const char*)(argv[(int)(tbl_field::end_date)]), "%Y-%m-%d", &expire_time);
            //strptime("2001-02-12", "%Y-%m-%d",  &expire_time);
            time_t now = time(0);// 基于当前系统的当前日期/时间
            tm *tm_now = localtime(&now);
            //printf("tm_year=%d,tm_mon=%d ,tm_yday=%d\n",expire_time.tm_year,expire_time.tm_mon,expire_time.tm_yday);
            if(tm_now->tm_year > expire_time.tm_year)
            {
                mysql->expire=1;
            }
            else if(tm_now->tm_year == expire_time.tm_year)
            {
                if (tm_now->tm_mon > expire_time.tm_mon)
                {
                    mysql->expire = 1;
                }
                else if(tm_now->tm_mon == expire_time.tm_mon)
                {
                    if (tm_now->tm_yday > expire_time.tm_yday)
                    {
                        mysql->expire = 1;
                    }
                }
            }
        }
    }
    else
    {
        mysql->expire=1;
    }

   return 0;
}

int mySqlite3::query_expire(int usr_id)
{
    int ret;
    char *zErrMsg = 0;
    std::string sql;
    int rc;

    sql="select * from usr_table  where usr_id=";
    sql+=std::to_string(usr_id);

    connect();

    /* Execute SQL statement */
    expire=0;
    rc = sqlite3_exec(conn, sql.c_str(), mySqlite3::callback, (void*)this, &zErrMsg);

    if( rc != SQLITE_OK ){
      fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
   }
   else
   {
      //fprintf(stdout, "Operation done successfully\n");
   }
   sqlite3_close(conn);
   return expire;
}