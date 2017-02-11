int api_openwin(char *buf, int xsiz, int ysiz, int col_inv, char *title);
void api_putstrwin(int win, int x, int y, int col, int len, char *str);
void api_boxfilwin(int win, int x0, int y0, int x1, int y1, int col);
void api_initmalloc(void);
char *api_malloc(int size);
char api_point(int win, int x, int y, int col);
void api_end(void);
int rand(void);

void HariMain(void)
{
	char *buf;
	int win, i, x, y;

	api_initmalloc();
	buf = api_malloc(150*100);
	win = api_openwin(buf, 150, 100, -1, "stars");
	api_boxfilwin(win, 6, 26, 143, 93, 0 /* black */);
	for(i=0; i<50; i++) {
		x = (rand() % 137) + 6;
		y = (rand() %  67) + 26;
		api_point(win+1, x, y, 3 /* yellow */);
			
	}
	api_refreshwin(win, 6, 26, 144, 94);
	api_getkey(1);
	api_end();
}
