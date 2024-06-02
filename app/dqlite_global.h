#ifndef __DQLITE_GLOBAL_H__
#define __DQLITE_GLOBAL_H__
#include "dqlite.h"
#include "server.h"
#include "dqlite_format.h"
#include <stdint.h>

#define DQLITE_DATABASE_PATH "/mnt/database"
#define DQLITE_DATABASE_NAME "dqlitedb"

struct dqlite_g_context {
	char 					listen_address[129];   /* format: "172.16.0.1" */
	uint16_t 				listen_port;           /* host order */
	char 					database_path[128];
	char 					database_name[128];

	struct dqlite_format    format;
	struct client_proto     client;
	struct dqlite_server   *server;
};

#endif
