#ifndef __DQLITE_FORMAT_H__
#define __DQLITE_FORMAT_H__

/* >= C99 support */
#include <stdbool.h>

typedef enum {
	DQLITE_CSV_MODE,
	DQLITE_COLUMN_MODE,
	DQLITE_HTML_MODE,	
	DQLITE_INSERT_MODE,	
	DQLITE_LINE_MODE,
	DQLITE_LIST_MODE,
	DQLITE_TABS_MODE,
	DQLITE_TCL_MODE,
	DQLITE_UNDEF_MODE
} dqlite_mode_type;

struct dqlite_format {
	bool show_header;
	dqlite_mode_type mode;
};

void dqlite_format_init(struct dqlite_format *format);

#endif
