CC=$(cross_compile)gcc
CFLAGS=-Wall -Werror -g
PREFIX=/usr/local

include_headers=\
	-I../include \
	-I../src/

LDFLAGS=$(include_headers) -ldqlite

exec=\
     dqlite_cli \
     dqlite_server

.PHONY: clean all install uninstall
all: $(exec)

dqlite_cli: linenoise.c dqlite_sql.c dqlite_format.c dqlite_cli.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) 

dqlite_server: dqlite_server.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

install:
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 0755 dqlite_cli $(DESTDIR)$(PREFIX)/bin
	install -m 0755 dqlite_server $(DESTDIR)$(PREFIX)/bin

uninstall:
	rm -fr $(DESTDIR)$(PREFIX)/bin/dqlite_cli
	rm -fr $(DESTDIR)$(PREFIX)/bin/dqlite_server

clean:
	rm -fr *.o $(exec)
