#include "dqlite.h"
#include "dqlite_sql.h"
#include "dqlite_global.h"
#include <stdint.h>
#include <sqlite3.h>

int dqlite_inner_exec_sql(struct client_proto *client, const char *sql)
{
	int ret = -1;

    uint64_t last_insert_id, rows_affected;

    if (clientSendExecSQL(client, sql, NULL, 0, NULL)) {
        return -1;
    }
	
    if (clientRecvResult(client, &last_insert_id, &rows_affected, NULL)) {
		return ret;
    }

    return 0;	
}

int dqlite_inner_query_sql(struct client_proto *client, const char *sql, struct rows *rows)
{
    int ret = -1;
	
    if (clientSendQuerySQL(client, sql, NULL, 0, NULL)) {
		return ret;
    }

    if (clientRecvRows(client, rows, NULL, NULL)) {
        return ret;
    }
	
    return 0;
}

