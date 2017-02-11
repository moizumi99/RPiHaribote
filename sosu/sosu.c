#include <stdio.h>
#include "apilib.h"

#define MAX 10000

void HariMain()
{
	char flag[MAX], s[8];
	int i, j;

	for (i=0; i<MAX; i++) {
		flag[i] = 0;
	}
	for (i=2; i<MAX; i++) {
		if (flag[i] == 0) {
			/* prime number */
			sprintf(s, "%d ", i);
			api_putstr0(s);
			for(j=i * 2; j<MAX; j += i) {
				flag[j] = 1;
			}
		}
	}
	api_end();
}
