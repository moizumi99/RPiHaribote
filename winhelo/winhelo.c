#include "apilib.h"

void HariMain(void)
{
	char buf[150*50];
	int win;
	win = api_openwin(buf, 150, 50, -1, "hello");
	api_getkey(1);
	api_end();
}
