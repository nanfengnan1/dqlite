#include "dqlite.h"
#include "dqlite_global.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/epoll.h>
#include <unistd.h>

struct dqlite_g_context g_context = {
	.listen_address = "127.0.0.1",
	.listen_port = 8888,
	.database_path = DQLITE_DATABASE_PATH,
	.database_name = DQLITE_DATABASE_NAME
};

int dqlite_s_exist_directory(const char *pathname)
{
	return !opendir(pathname)? -1: 0;
}

int dqlite_s_create_directory(const char *pathname)
{
	int ret = -1;
	if (pathname == NULL)
		return ret;
	
	if (mkdir (pathname, 0x766) < 0) {			
		fprintf(stderr, "%s", "dqlite server create directory failure\n");
		return ret;
	}

	return 0;
}

int dqlite_s_start_server(struct dqlite_g_context *context)
{
	int ret = -1;
	char listen_addr_pair[256];
	
	snprintf(listen_addr_pair, sizeof(listen_addr_pair), "%s:%hu%c",
			context->listen_address, context->listen_port, '\0');
	
	if (dqlite_s_exist_directory(context->database_path) < 0)
		dqlite_s_create_directory(context->database_path);

#ifdef DQLITE_DEFINE_DISTRIBUTE_MACHINE

	/* wait to support */
	if (dqlite_server_start(&context->server) != 0) {
		fprintf(stderr, "%s", "dqlite server start failure\n");
		return ret;
	}
#else
	if (dqlite_server_create(context->database_path, &context->server) != 0) {
		fprintf(stderr, "%s", "dqlite server start failure\n");
		return ret;		
	}

	context->server->refresh_period = 100;
	if (dqlite_server_set_auto_bootstrap(context->server, true) != 0) {
		fprintf(stderr, "%s", "dqlite server configure bootstrap failure\n");
		return ret;		
    }
	
	if (dqlite_server_set_address(context->server, listen_addr_pair) != 0) {
		fprintf(stderr, "%s", "dqlite server configure address:port failure\n");
		return ret;
	}

	if (dqlite_server_start(context->server) != 0) {
		fprintf(stderr, "%s", "dqlite server boot failure\n");
		return ret;
	}

	printf("create server success!");
#endif
	return 0;
}

void dqlite_server_daemon()
{
	pid_t pid;
	
	/* new session */
	if (setsid() == -1) {
		perror("setsid");
		exit(EXIT_FAILURE);
	}

	pid = fork();
	if (pid < 0) {
		perror("fork");
		exit(EXIT_FAILURE);
	}

	if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	if (chdir("/") == -1) {
		perror("chdir");
		exit(EXIT_FAILURE);
	}

	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	open("/dev/null", O_RDONLY);
	open("/dev/null", O_WRONLY);
	open("/dev/null", O_WRONLY);

	while(1) {
		
		/* code logic */
		dqlite_s_start_server(&g_context);
	}
}

int main(int argc, char *argv[])
{

	pid_t pid = fork();
	
	if (pid < 0) {
		perror("fork");
		exit(EXIT_FAILURE);
	}

	if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	dqlite_server_daemon();

	return 0;
}
