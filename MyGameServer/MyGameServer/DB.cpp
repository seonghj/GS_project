#include "DB.h"

bool DB::Connection()
{
    printf("MySQL Ver. %s\n", mysql_get_client_info());

    if (mysql_init(&conn) == NULL)
        printf("mysql_init() error\n");

    connection = mysql_real_connect(&conn, DB_HOST, DB_USER
        , DB_PW, DB_NAME, 3306, (const char*)NULL, 0);

    mysql_query(connection, "set session character_set_connection=euckr;");
    mysql_query(connection, "set session character_set_results=euckr;");
    mysql_query(connection, "set session character_set_client=euckr;");

    if (connection == NULL)
    {
        printf("%d: %s\n", mysql_errno(&conn), mysql_error(&conn));
        return 0;
    }
    else
    {
        printf("DB connected\n");
        return 1;
    }
}

bool DB::Send_Query(char* query)
{
    int state = mysql_query(connection, query);
    if (state != 0)
    {
        printf("MySQL query error : %s\n", mysql_error(&conn));
        return 0;
    }
    else
        return 1;
}

bool DB::Connection_ODBC()
{
    // 환경 구성
    if (SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv)
        != SQL_SUCCESS)
        return false;
    // 버전 정보 설정
    if (SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, SQL_IS_INTEGER)
        != SQL_SUCCESS)
        return false;
    if (SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc)
        != SQL_SUCCESS)
        return false;
    // 접속
    SQLSetConnectAttr(hDbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);
    if (SQLConnect(hDbc, (SQLWCHAR*)L"skyfall", SQL_NTS
        , (SQLWCHAR*)L"root", SQL_NTS
        , (SQLWCHAR*)L"tjdwo@1034", SQL_NTS)
        != SQL_SUCCESS) {
        std::cout << "DB error!\n";
        return false;
    }
    if (SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt)
        != SQL_SUCCESS){
        std::cout << "DB error!\n";
        return false;
    }
    std::cout << "DB connected!\n";
    return true;
}

void DB::Disconnection_ODBC()
{
    if (hStmt)
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    if (hDbc)
        SQLDisconnect(hDbc);
    if (hDbc)
        SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
    if (hEnv)
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
}

bool DB::Search_ID(char* id)
{
    wchar_t query1[512] = L"SELECT isLogin FROM GS_termproject.userinfo WHERE ID = '";
    wchar_t wcID[20];

    SQLLEN len = 0;
    bool isLogin = 0;

    MultiByteToWideChar(CP_ACP, 0, id, -1, wcID, sizeof(id));

    wcscat_s(query1, wcID);
    wcscat_s(query1, L"'");

#ifdef Test_DB 
    wprintf(L"%s\n", query);
#endif

    // ID 검색
    if (SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt)
        != SQL_SUCCESS)
        return false;

    if (SQLExecDirect(hStmt, (SQLWCHAR*)query1, SQL_NTS)
        != SQL_SUCCESS) {
        printf("Search_ID Query invaild\n");
        return false;
    }
    SQLBindCol(hStmt, 1, SQL_C_TINYINT, &isLogin, sizeof(bool), &len);
    if (SQLFetch(hStmt) == SQL_NO_DATA) return false;
    if (hStmt) SQLCloseCursor(hStmt);

    if (isLogin == true) return false;
    return false;
}

bool DB::Insert_ID(char* id)
{
    wchar_t query[512] = L"insert into GS_termproject.userinfo VALUES ('";
    wchar_t wcID[20];

    MultiByteToWideChar(CP_ACP, 0, id, -1, wcID, sizeof(id));

    wcscat_s(query, wcID);
    wcscat_s(query, L"', '0')");

#ifdef Test_DB 
    wprintf(L"%s\n", query);
#endif

    if (SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt)
        != SQL_SUCCESS)
        return false;

    if (SQLExecDirect(hStmt, (SQLWCHAR*)query, SQL_NTS)
        != SQL_SUCCESS) {
        printf("Insert_ID Query invaild\n");
        return false;
    }

    if (hStmt) SQLCloseCursor(hStmt);

    return true;
}

bool DB::Logout_player(char* id)
{
    wchar_t query[512] = L"UPDATE GS_termproject.UserInfo SET isLogin = 0 WHERE ID = '";
    wchar_t wcID[20];

    MultiByteToWideChar(CP_ACP, 0, id, -1, wcID, sizeof(id));
    wcscat_s(query, wcID);
    wcscat_s(query, L"'");

#ifdef Test_DB 
    wprintf(L"%s\n", query);
#endif

    if (SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt)
        != SQL_SUCCESS)
        return false;

    if (SQLExecDirect(hStmt, (SQLWCHAR*)query, SQL_NTS)
        != SQL_SUCCESS) {
        printf("Query invaild\n");
        return false;
    }

    if (hStmt) SQLCloseCursor(hStmt);

    return true;
}