#include "apilib.h"

void HariMain(void)
{
	char *buf;
	int win;

	api_initmalloc();
	buf = api_malloc(150*50);
	win = api_openwin(buf, 152, 50, -1, "hello");
	api_boxfilwin(win, 8, 36, 141, 43, 6 /* yellow */);
	api_putstrwin(win, 28, 28, 0 /* black */, 12, "hello, world");
	api_getkey(1);
	api_end();
}
