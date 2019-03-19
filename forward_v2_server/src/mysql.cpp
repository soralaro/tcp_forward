//
// Created by deepglint on 19-3-8.
//

#include "mysql.h"
enum class tbl_field
{
    usr_id = 0,
    usr_name,
    psw,
    forbid,
    start_date,
    end_date
};
mySQL::mySQL()
{
    server = "localhost";
    user = "root";
    password = "7755120";
    database = "gfw";
    conn=nullptr;
    res=nullptr;
}
mySQL::~mySQL()
{
    mysql_free_result(res);
    mysql_close(conn);
}
int mySQL::connect()
{
    conn = mysql_init(nullptr);

    if (!mysql_real_connect(conn, server.c_str(),user.c_str(), password.c_str(), database.c_str(), 0, NULL, 0))
    {
        fprintf(stderr, "%s\n", mysql_error(conn));
        return -1;
    }
    else
    {
        printf("mysql_real_connect sucessfully\n");
    }
}

int mySQL::query_expire(int usr_id)
{
    int ret;
    std::string sql;
    sql="select * from usr_tale  where usr_id=";
    sql+=std::to_string(usr_id);
    connect();
    if(!mysql_query(conn,sql.c_str()))
    {
        res=mysql_store_result(conn);
        if(res!=nullptr) {
            unsigned int num_fields;
            MYSQL_FIELD *fields;
            num_fields = mysql_num_fields(res);
            fields = mysql_fetch_fields(res);
            // for (int i = 0; i < num_fields; i++)
            // {
            //     printf("Field %u is %s\n", i, fields[i].name);
            // }
            MYSQL_ROW column;   //数据行的列
            column = mysql_fetch_row(res);
            if(column==nullptr)
            {
                mysql_free_result(res);
                mysql_close(conn);
                return 1;
            }
            //printf("usr_id=%s,usr_name=%s,psw=%s,forbid=%s,start_date=%s,end_date=%s \n",column[0],column[1],column[2],column[3],column[4],column[5]);
            int forbid=atoi(column[(int)(tbl_field::forbid)]);
            if(forbid==1)
            {
                mysql_free_result(res);
                mysql_close(conn);
                return 1;
            }
            struct tm expire_time;
            strptime((const char*)(column[(int)(tbl_field::end_date)]), "%Y-%m-%d", &expire_time);
            time_t now = time(0);// 基于当前系统的当前日期/时间
            tm *tm_now = localtime(&now);
            int expire=0;
            if(tm_now->tm_year > expire_time.tm_year)
            {
                expire=1;
            }
            else if(tm_now->tm_year == expire_time.tm_year)
            {
                if (tm_now->tm_mon > expire_time.tm_mon)
                {
                    expire = 1;
                }
                else if(tm_now->tm_mon == expire_time.tm_mon)
                {
                    if (tm_now->tm_yday > expire_time.tm_yday)
                    {
                        expire = 1;
                    }
                }
            }
            mysql_free_result(res);
            mysql_close(conn);
            return expire;
        }
    }
    mysql_close(conn);
    return 1;
}