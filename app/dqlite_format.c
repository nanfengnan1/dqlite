#include <stddef.h>

#include "dqlite_format.h"

void dqlite_format_init(struct dqlite_format *format)
{
	if (format == NULL)
		return;

	format->mode = DQLITE_UNDEF_MODE;
	format->show_header = false;
}
