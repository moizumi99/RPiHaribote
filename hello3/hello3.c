#include "apilib.h"

void main()
{
	int i=0;
	for(i=0; i<100; i++) {
		api_putchar('H');
		api_putchar('e');
		api_putchar('l');
		api_putchar('l');
		api_putchar('o');
		api_putchar('3');
		api_putchar('\n');
	}
	api_end();
}
