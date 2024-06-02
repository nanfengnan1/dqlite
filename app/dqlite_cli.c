#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

#include "linenoise.h"
#include "dqlite.h"
#include "dqlite_sql.h"
#include "dqlite_format.h"
#include "dqlite_global.h"

#define DQ_CLI_PREFIX_NUM_MAX 32
#define HISTORY_MAX_LENGTH 40

#define dqlite_cli_subfix_is_valid(str, i) (!isspace(*(str + i)) && *(str + i) != '\0')

struct dqlite_g_context g_cli_context = {
	.listen_address = "127.0.0.1",
	.listen_port = 8888,
	.database_path = DQLITE_DATABASE_PATH,
	.database_name = DQLITE_DATABASE_NAME
};

typedef enum {
	DQLITE_PRAGMA_HELP,
	DQLITE_PRAGMA_HEADER,
	DQLITE_PARGMA_MODE,
	DQLITE_PRAGMA_END
} dqlite_pragma_t;

const char *dqlite_pragma[] = {
	".help",
	".header(s) ON|OFF",
	".mode MODE"
};

const char *dqlite_pragma_desc[] = {
    "                 Show this message",
    "     Turn display of headers on or off",
    "            Set output mode where MODE is one of:\n"
    "                            csv      Comma-separated values\n"
    "                            column   Left-aligned columns.  (See .width)\n"
    "                            html     HTML <table> code\n"
    "                            insert   SQL insert statements for TABLE\n"
    "                            line     One value per line\n"
    "                            list     Values delimited by .separator string\n"
    "                            tabs     Tab-separated values\n"
    "                            tcl      TCL list elements\n",
};


const char *dqlite_cli_prefix[26][DQ_CLI_PREFIX_NUM_MAX] = {
	/* ALTER, ALTER TABLE*/
	{
		"alter",
		"alter table"
	},
	/* B */
	{
		"begin"
	},
	/* C */
	{
		"create",
		"create table",
		"create trigger",
		"create index",
		"create unique",
		"commit"
	},
	/* D */
	{
		"delete",
		"delete from",
		"drop",
		"drop table"
	},
	/* E */
	{
		"exit",
		"end"
	},
	/* F */
	{
	},
	/* G */
	{
	},
	/* H */
	{
		"history"
	},
	/* I */
	{
		"insert",
		"insert into"
	},
	/* J */
	{
	},
	/* K */
	{
	},
	/* L */
	{
	},
	/* M */
	{
	},
	/* N */
	{
	},
	/* O */
	{
	},
	/* P */
	{
	},
	/* Q */
	{
		"quit"
	},
	/* R */
	{
	},
	/* S */
	{
		"select",
	},
	/* T */
	{
	},
	/* U */
	{
		"update",
	},
	/* V */
	{
	},
	/* W */
	{
	},
	/* X */
	{
	},
	/* Y */
	{
	},
	/* Z */
	{
	}		
};

void dqlite_cli_completion(const char *buf, linenoiseCompletions *lc)
{
	int index = -1;	
	int buf_len;
	
	if (buf[0] >= 'A' && buf[0] <= 'Z')
		index = buf[0] - 'A';

	if (buf[0] >= 'a' && buf[0] <= 'z')
		index = buf[0] - 'a';

	buf_len = strlen(buf);

	if (index >= 0) {

		int i;

		for (i = 0; i < DQ_CLI_PREFIX_NUM_MAX; i++) {

			if (NULL == dqlite_cli_prefix[index][i])
				break;

			if (0 == strncasecmp(dqlite_cli_prefix[index][i], buf, buf_len)) {
				
				linenoiseAddCompletion(lc, dqlite_cli_prefix[index][i]);
				break;
			}
		}
	}
}

char *dqlite_cli_hints(const char *buf, int *color, int *bold)
{
    if (!strcasecmp(buf, "hello")) {
        *color = 35;
        *bold = 0;
        return " World";
    }

	if (!strcasecmp(buf, "create")) {
		*color = 32;
		*bold = 0;
		return " table";
	} else if (!strcasecmp(buf, "create table")) {
		*color = 32;
		*bold = 0;
		return " <table_name>";
	}
	
    return NULL;
}

int dqlite_cli_connect_server(uint32_t ip, uint16_t port)
{
	int fd;
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(ip);

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		perror(strerror(errno));
		return -1;
	}

	if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		perror(strerror(errno));
		return -1;
	}

	return fd;
}

int dqlite_cli_init()
{
	int ret = -1;
	int fd;
	uint32_t ipv4_addr;
    struct client_proto *client = &g_cli_context.client;

    if (1 != inet_pton(AF_INET, g_cli_context.listen_address, &ipv4_addr)) {
		perror(strerror(errno));
        return ret;
    }
	
    ipv4_addr = ntohl(ipv4_addr);
	
	if ((fd = dqlite_cli_connect_server(ipv4_addr, g_cli_context.listen_port)) < 0) {
		return ret;
	}
	
    memset(client, 0, sizeof(struct client_proto));
    buffer__init(&(client->read));
    buffer__init(&(client->write));
    client->fd = fd;

    if (clientSendHandshake(client, NULL) != 0)
		return ret;

    if (clientSendOpen(client, g_cli_context.database_name, NULL) != 0)
		return ret;
	
    if (clientRecvDb(client, NULL) != 0)
		return ret;
	
	return 0;
}

int dqlite_cli_print(const char *line)
{
	return 0;
}

int dqlite_cli_query_parse(struct rows *rows)
{
	int ret = -1;
	
	if (rows == NULL)
		return ret;

	return 0;
}

int dqlite_cli_exit(const char *line)
{
	return (strncasecmp("quit", line, 4) == 0 
			|| strncasecmp("exit", line, 4) == 0) ? 0: -1;
}

int dqlite_cli_exec_progma(const char *line)
{
	int ret = -1;
	int i;
	
	if (strncmp(".help", line, 5) == 0) {

		if (dqlite_cli_subfix_is_valid(line, 5))
			return ret;
		
		dqlite_pragma_t prag;

		for (prag = DQLITE_PRAGMA_HELP; prag < DQLITE_PRAGMA_END; ++prag) {
			
			printf("%s    %s\n", dqlite_pragma[prag] , dqlite_pragma_desc[prag]);
		}
	} else if (strncmp(".header", line, 7) == 0) {

		i = 7;
		
		if (line[i] == '\0')
			return ret;

		while (isspace(line[i++]));

		if (line[--i] == '\0')
			return ret;

		if (strncasecmp("on", line + i, 2) == 0) {

			if (dqlite_cli_subfix_is_valid(line, i + 2))
				return ret;
		
			g_cli_context.format.show_header = true;
		} else if (strncasecmp("off", line + i, 3) == 0) {

			if (dqlite_cli_subfix_is_valid(line, i + 3))
				return ret;
			
			g_cli_context.format.show_header = false;			
		} else
			return ret;

	} else if (strncmp(".mode", line, 5) == 0) {

		i = 5;
		
		if (line[i] == '\0')
			return ret;

		while (isspace(line[i++]));

		if (line[--i] == '\0')
			return ret;

		if (strncasecmp("csv", line + i, 3) == 0) {

			if (dqlite_cli_subfix_is_valid(line, i + 3))
				return ret;
					
			g_cli_context.format.mode = DQLITE_CSV_MODE;
		} else if (strncasecmp("column", line + i, 6) == 0) {

			if (dqlite_cli_subfix_is_valid(line, i + 6))
				return ret;
		
			g_cli_context.format.mode = DQLITE_COLUMN_MODE;
		} else if (strncasecmp("html", line + i, 4) == 0) {

			if (dqlite_cli_subfix_is_valid(line, i + 4))
				return ret;
		
			g_cli_context.format.mode = DQLITE_HTML_MODE;			
		} else if (strncasecmp("insert", line + i, 6) == 0) {

			if (dqlite_cli_subfix_is_valid(line, i + 6))
				return ret;
		
			g_cli_context.format.mode = DQLITE_INSERT_MODE;
		} else if (strncasecmp("line", line + i, 4) == 0) {

			if (dqlite_cli_subfix_is_valid(line, i + 4))
				return ret;
		
			g_cli_context.format.mode = DQLITE_LINE_MODE;
		} else if (strncasecmp("list", line + i, 4) == 0) {

			if (dqlite_cli_subfix_is_valid(line, i + 4))
				return ret;
		
			g_cli_context.format.mode = DQLITE_LIST_MODE;
		} else if (strncasecmp("tabs", line + i, 4) == 0) {

			if (dqlite_cli_subfix_is_valid(line, i + 4))
				return ret;
		
			g_cli_context.format.mode = DQLITE_TABS_MODE;
		} else if (strncasecmp("tcl", line + i, 3) == 0) {

			if (dqlite_cli_subfix_is_valid(line, i + 3))
				return ret;
		
			g_cli_context.format.mode = DQLITE_TCL_MODE;
		} else
			return ret;
	}

	return 0;
}

int dqlite_cli_exec_sql(const char *line)
{
	int ret, i;
    struct rows rows;
    struct row *row, *next;

	ret = -1;

	if (strncasecmp(".", line, 1) == 0) {
		
		if (dqlite_cli_exec_progma(line) < 0)
			return ret;
		
	} else if (strncasecmp("select", line, 6) == 0) {
	
		if (dqlite_inner_query_sql(&g_cli_context.client, line, &rows) < 0) {
			return ret;
		}

        if (rows.column_names != NULL) {
    		for (i = 0; i < rows.column_count; ++i) {
    			printf("%s ", rows.column_names[i]);
    		}
    		printf("\n---------------------------------------------------------------------\n");
    	}

    	for (row = rows.next; row != NULL; row = next) {
    		next = row->next;
    		for (i = 0; i < rows.column_count; ++i) {
    			switch (row->values[i].type) {
    			    case SQLITE_INTEGER:
    			        printf("%ld ", row->values[i].integer);
    			        break;
			        case SQLITE_FLOAT:
    			        printf("%lf ", row->values[i].float_);
    			        break;
			        case SQLITE_TEXT:
        		        printf("'%s' ", row->values[i].text);
            			break;
			        case SQLITE_BLOB:
    			        break;
			        case SQLITE_NULL:
			            printf("NULL ");
    			        break;
            		default:
            		    break;
            	}
    		}
			
    		printf("\n");
    	}
	
		clientCloseRows(&rows);
	} else {
		dqlite_inner_exec_sql(&g_cli_context.client, line);
	}

	return 0;
}

#define SINGLE_MACHINE

int main(int argc, char **argv) {
    char *line;

	if (dqlite_cli_init() < 0) {
		printf("cli client init failure\n");
		return -1;
	}

	dqlite_format_init(&g_cli_context.format);

    linenoiseSetCompletionCallback(dqlite_cli_completion);
    linenoiseSetHintsCallback(dqlite_cli_hints);

	/* Load the history at startup */
    linenoiseHistoryLoad("history.txt");

	// linenoiseHistorySetMaxLen(HISTORY_MAX_LENGTH);

#ifdef SINGLE_MACHINE
    while((line = linenoise("dqlite> ")) != NULL) {
        if (line == NULL) break;

		linenoiseHistoryAdd(line);
		linenoiseHistorySave("history.txt");

		if (!dqlite_cli_exit(line)) {
			free(line);
			break;
		}

		if (dqlite_cli_exec_sql(line) < 0)
			printf("dqlite sql execute failure\n");

        free(line);
    }
#endif
    return 0;
}
