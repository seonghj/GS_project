#pragma once
#include "stdafx.h"
#include "sql.h"
#include "sqlext.h"
#include <mysql.h>

#define DB_HOST "127.0.0.1"
#define DB_USER "root"
#define DB_PW "tjdwo@1034"
#define DB_NAME "GS_termproject"

class DB
{
public:
	MYSQL* connection = NULL;
	MYSQL conn;
	MYSQL_RES* sql_result;
	MYSQL_ROW sql_row;
	char query[255];
	bool isRun = false;

	bool Connection();
	bool Send_Query(char* query);

	SQLHENV hEnv;
	SQLHDBC hDbc;
	SQLHSTMT hStmt = 0;

	SQLCHAR* name = (SQLCHAR*)"GS_termproject";
	SQLCHAR* user = (SQLCHAR*)"root";
	SQLCHAR* pw = (SQLCHAR*)"tjdwo@1034";

	bool Connection_ODBC();
	void Disconnection_ODBC();
	bool Search_ID(char* id);
	bool Insert_ID(char* id);
	bool Logout_player(char* id);
};