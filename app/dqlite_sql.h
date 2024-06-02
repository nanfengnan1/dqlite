#ifndef _DQLITE_SQL_H__
#define _DQLITE_SQL_H__

#include "dqlite_global.h"
#include <sqlite3.h>

struct dqlite_static_table {
	char *name;
};


int dqlite_inner_exec_sql(struct client_proto *client, const char *sql);
int dqlite_inner_query_sql(struct client_proto *client, const char *sql, struct rows *rows);

#endif
