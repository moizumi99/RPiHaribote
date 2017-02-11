#include "apilib.h"

void HariMain(void)
{
	char buf[150*50];
	int win;
	win = api_openwin(buf, 150, 50, -1, "hello");
	api_boxfilwin(win, 8, 36, 141, 43, 3 /* yellow */);
	api_putstrwin(win, 28, 28, 0 /* black */, 12, "hello, world");
	api_getkey(1);
	api_end();
}
