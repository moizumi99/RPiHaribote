#include "bootpack.h"
#include <stdio.h>
#include <string.h>
#define APP_ADR 0x80000000

extern char *rdedata;
extern long long fatadr, rdeadr, usradr;
extern uint32_t sectorsPerFAT;
extern uint32_t _cons_shtctl;
extern uint32_t _task_a_fifo;
extern void start_app(uint32_t pc, uint32_t sp);
int setpages(unsigned char *section_table_adr, unsigned char *va_st, unsigned char *pa_st, uint32_t size, char ap);
extern uint32_t _task_a_nihongo;

void console_task(struct SHEET *sheet, unsigned int memtotal)
{
	struct TASK *task = (struct TASK *)task_now(); // RPi
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	int i;
	uint16_t *fat = (uint16_t *) memman_alloc_4k(memman, sectorsPerFAT*512); // RPi
	struct CONSOLE cons;
	struct FILEHANDLE fhandle[8];
	char cmdline[30];
	unsigned char *nihongo = (unsigned char *) _task_a_nihongo;
	
	cons.sht = sheet;
	cons.cur_x =  8;
	cons.cur_y = 28;
	cons.cur_c = -1;
	task->cons = &cons;
	task->cmdline = cmdline;

	if (cons.sht != 0) {
		cons.timer = timer_alloc();
		timer_init(cons.timer, &task->fifo, 1);
		timer_settime(cons.timer, 50);
	}
	file_readfat(fat, fatadr);
	for (i = 0; i < 8; i++) {
		fhandle[i].buf = 0;	/* 未使用マーク */
	}
	task->fhandle = fhandle;
	task->fat = fat;
	if (nihongo[4096] != 0xff) {	/* 日本語フォントファイルを読み込めたか？ */
		task->langmode = 1;
	} else {
		task->langmode = 0;
	}
	task->langbyte1 = 0;

	/* プロンプト表示 */
	cons_putchar(&cons, '>', 1);

	for (;;) {
		io_cli();
		if (fifo32_status(&task->fifo) == 0) {
			task_sleep(task);
			io_sti();
		} else {
			i = fifo32_get(&task->fifo);
			io_sti();
			if (i <= 1 && cons.sht != 0) {  /* カーソル用タイマ */
				if (i != 0) {
					timer_init(cons.timer, &task->fifo, 0);  /* 次は0を */
					if (cons.cur_c >= 0) {
						cons.cur_c = COL8_FFFFFF;
					}
				} else {
					timer_init(cons.timer, &task->fifo, 1); /* 次は1を */
					if (cons.cur_c >= 0) {
						cons.cur_c = COL8_000000;
					}
				}
				timer_settime(cons.timer, 50);
			}
			if (i == 2) {
				cons.cur_c = COL8_FFFFFF;
			}
			if (i == 3) {
				if (cons.sht != 0) {
					boxfill8(cons.sht->buf, cons.sht->bxsize, COL8_000000,
						cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);
				}
				cons.cur_c = -1;
			}
			if (i == 4) { /* "x" button on the console is clicked */
				cmd_exit(&cons, fat);
			}
			if (256 <= i && i <= 511) { /* キーボードデータ（タスクA経由） */
				if (i == 8 + 256) {
					/* バックスペース */
					if (cons.cur_x > 16) {
						/* カーソルをスペースで消してから、カーソルを1つ戻す */
						cons_putchar(&cons, ' ', 0);
						cons.cur_x -= 8;
					}
				} else if (i == 10 + 256) {
					/* Enter */
					/* remove the cursor with space and then new line */
					cons_putchar(&cons, ' ', 0);
					cmdline[cons.cur_x / 8 - 2] = 0;
					cons_newline(&cons);
					cons_runcmd(cmdline, &cons, fat, memtotal);	/* コマンド実行 */
					if (cons.sht == 0) {
						cmd_exit(&cons, fat);
					}
					/* vvg\Š */
					cons_putchar(&cons, '>', 1);
				} else {
					/* 一般文字 */
					if (cons.cur_x < 240) {
						/* 一文字表示してから、カーソルを1つ進める */
						cmdline[cons.cur_x / 8 - 2] = i - 256;
						cons_putchar(&cons, i-256, 1);
					}
				}
			}
			/* カーソル再表示 */
			if (cons.sht != 0) {
				if (cons.cur_c >= 0) {
					boxfill8(cons.sht->buf, cons.sht->bxsize, cons.cur_c, 
						cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);
				}
				sheet_refresh(cons.sht, cons.cur_x, cons.cur_y, cons.cur_x + 8, cons.cur_y + 16);
			}
		}
	}
}

void cons_putchar(struct CONSOLE *cons, int chr, char move)
{
	char s[2];
	s[0] = chr;
	s[1] = 0;
	if (s[0] == 0x09) {	/* タブ */
		for (;;) {
			if (cons->sht != 0) {
				putfonts8_asc_sht(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, " ", 1);
			}
			cons->cur_x += 8;
			if (cons->cur_x == 8 + 240) {
				cons_newline(cons);
			}
			if (((cons->cur_x - 8) & 0x1f) == 0) {
				break;	/* 32で割り切れたらbreak */
			}
		}
	} else if (s[0] == 0x0a) {	/* 改行 */
		cons_newline(cons);
	} else if (s[0] == 0x0d) {	/* 復帰 */
		/* とりあえずなにもしない */
	} else {	/* 普通の文字 */
		if (cons->sht != 0) {
			putfonts8_asc_sht(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, s, 1);
		}
		if (move != 0) {
			/* moveが0のときはカーソルを進めない */
			cons->cur_x += 8;
			if (cons->cur_x == 8 + 240) {
				cons_newline(cons);
			}
		}
	}
	return;
}

void cons_newline(struct CONSOLE *cons)
{
	int x, y;
	struct SHEET *sheet = cons->sht;
	struct TASK *task = (struct TASK *) task_now();
	if (cons->cur_y < 28 + 112) {
		cons->cur_y += 16; /* 次の行へ */
	} else {
		/* スクロール */
		if (sheet != 0) {
			for (y = 28; y < 28 + 112; y++) {
				for (x = 8; x < 8 + 240; x++) {
					sheet->buf[x + y * sheet->bxsize] = sheet->buf[x + (y + 16) * sheet->bxsize];
				}
			}
			for (y = 28 + 112; y < 28 + 128; y++) {
				for (x = 8; x < 8 + 240; x++) {
					sheet->buf[x + y * sheet->bxsize] = COL8_000000;
				}
			}
			sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
		}
	}
	cons->cur_x = 8;
	if (task->langmode == 1 && task->langbyte1 != 0) {
		cons->cur_x = 16;
	}
	return;
}

void cons_putstr0(struct CONSOLE *cons, char *s)
{
	for (; *s != 0; s++) {
		cons_putchar(cons, *s, 1);
	}
	return;
}

void cons_putstr1(struct CONSOLE *cons, char *s, int l)
{
	int i;
	for (i = 0; i < l; i++) {
		cons_putchar(cons, s[i], 1);
	}
	return;
}


void cons_runcmd(char *cmdline, struct CONSOLE *cons, uint16_t *fat, unsigned int memtotal)
{
	if (strcmp(cmdline, "mem") == 0 && cons->sht != 0) {
		cmd_mem(cons, memtotal);
	} else if (strcmp(cmdline, "cls") == 0 && cons->sht != 0) {
		cmd_cls(cons);
	} else if (strcmp(cmdline, "dir") == 0 && cons->sht != 0) {
		cmd_dir(cons);
	} else if (strcmp(cmdline, "exit")==0) {
		cmd_exit(cons, fat);
	} else if (strncmp(cmdline, "start ", 6)==0) {
		cmd_start(cons, cmdline, memtotal);
	} else if (strncmp(cmdline, "ncst ", 5)==0) {
		cmd_ncst(cons, cmdline, memtotal);
	} else if (strncmp(cmdline, "langmode ", 9) == 0) {
		cmd_langmode(cons, cmdline);
	} else if (cmdline[0] != 0) {
		if (cmd_app(cons, fat, cmdline) == 0) {
			/* not a command, not an application, and not empty */
			cons_putstr0(cons, "Bad command.\n\n");
		}
	}
	return;
}

void cmd_mem(struct CONSOLE *cons, unsigned int memtotal)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	char s[60];
	sprintf(s, "total   %dMB\nfree %dKB\n\n", memtotal / (1024 * 1024), memman_total(memman) / 1024);
	cons_putstr0(cons, s);
	return;
}

void cmd_cls(struct CONSOLE *cons)
{
	int x, y;
	struct SHEET *sheet = cons->sht;
	for (y = 28; y < 28 + 128; y++) {
		for (x = 8; x < 8 + 240; x++) {
			sheet->buf[x + y * sheet->bxsize] = COL8_000000;
		}
	}
	sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
	cons->cur_y = 28;
	return;
}

void cmd_dir(struct CONSOLE *cons)
{
	volatile struct FILEINFO *finfo = (volatile struct FILEINFO *) (rdedata); // RPi only
	int i, j;
	char s[30];
	
	for (i = 0; i <512; i++) {
		if (finfo[i].name[0] == 0x00) {
			break;
		}
		if (finfo[i].name[0] != 0xe5) {
			if ((finfo[i].type & 0x18) == 0) {
				sprintf(s, "filename.ext   %7d\n", finfo[i].size);
				for (j = 0; j < 8; j++) {
					s[j] = finfo[i].name[j];
				}
				s[ 9] = finfo[i].ext[0];
				s[10] = finfo[i].ext[1];
				s[11] = finfo[i].ext[2];
				cons_putstr0(cons, s);
			}
		}
	}
	cons_newline(cons);
	return;
}

void cmd_exit(struct CONSOLE *cons, uint16_t *fat)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct TASK *task = (struct TASK *) task_now();
	struct SHTCTL *shtctl = (struct SHTCTL *) _cons_shtctl;
	struct FIFO32 *fifo = (struct FIFO32 *) _task_a_fifo;
	if (cons->sht != 0) {
		timer_cancel(cons->timer);
	}
	memman_free_4k(memman, (int) fat, sectorsPerFAT*512);
	io_cli();
	if (cons->sht != 0) {
		fifo32_put(fifo, cons->sht - shtctl->sheets0 + 768);	/* 768〜1023 */
	} else {
		fifo32_put(fifo, task - taskctl->tasks0 + 1024); /* 1024 - 2023 */
	}
	io_sti();
	for (;;) {
		task_sleep(task);
	}
}

void cmd_start(struct CONSOLE *cons, char *cmdline, int memtotal)
{
	struct SHTCTL *shtctl = (struct SHTCTL *) _cons_shtctl;
	struct SHEET *sht = open_console(shtctl, memtotal);
	struct FIFO32 *fifo = &sht->task->fifo;
	int i;
	sheet_slide(sht, 32, 4);
	sheet_updown(sht, shtctl->top);
	/* コマンドラインに入力された文字列を、一文字ずつ新しいコンソールに入力 */
	for (i = 6; cmdline[i] != 0; i++) {
		fifo32_put(fifo, cmdline[i] + 256);
	}
	fifo32_put(fifo, 10 + 256);	/* Enter */
	cons_newline(cons);
	return;
}

void cmd_ncst(struct CONSOLE *cons, char *cmdline, int memtotal)
{
	struct TASK *task = open_constask(0, memtotal);
	struct FIFO32 *fifo = &task->fifo;
	int i;
	/* コマンドラインに入力された文字列を、一文字ずつ新しいコンソールに入力 */
	for (i = 5; cmdline[i] != 0; i++) {
		fifo32_put(fifo, cmdline[i] + 256);
	}
	fifo32_put(fifo, 10 + 256);	/* Enter */
	cons_newline(cons);
	return;
}

void cmd_langmode(struct CONSOLE *cons, char *cmdline)
{
	struct TASK *task = (struct TASK *) task_now();
	unsigned char mode = cmdline[9] - '0';
	if (mode <= 2) {
		task->langmode = mode;
	} else {
		cons_putstr0(cons, "mode number error (0:EN, 1: SJIS, 2:EUC).\n");
	}
	cons_newline(cons);
	return;
}

int cmd_app(struct CONSOLE *cons, uint16_t *fat, char *cmdline)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct FILEINFO *finfo;
	char name[18], *p, *q, *h;
	struct TASK *task = (struct TASK *) task_now();
	int i;
	struct SHTCTL *shtctl;
	struct SHEET *sht;
	uint32_t stack_size, heap_size;

	/* R}hC©çt@CŒð¶¬ */
	for(i=0; i<13; i++) {
		if (cmdline[i] <= ' ') {
			break;
		}
		name[i] = cmdline[i];
	}
	name[i] = 0;
	
	finfo = file_search(name, (struct FILEINFO *) rdedata, 512);
	if (finfo == 0 && name[i-1]!='.') {
		name[i    ] = '.';
		name[i + 1] = 'O';
		name[i + 2] = 'U';
		name[i + 3] = 'T';
		name[i + 4] = 0;
		finfo = file_search(name, (struct FILEINFO *) rdedata, 512);
	}
	if (finfo != 0) {
		/* file found */
		p = (char *) memman_alloc_4k(memman, finfo->size);
		printf("Application start at 0x%08x, size: 0x%008x\n", p, finfo->size);
		// application code and data are placed after APP_ADR
		file_loadfile(finfo->clustno, finfo->size, p, fat, usradr);  // RPi
		if (finfo->size >= 8 && *p==0x7F && strncmp(p+1, "ELF", 3)==0) {
			stack_size = 1024*1024*2;
			heap_size = 1024*1024*2;
			q = (char *) memman_alloc_4k(memman, stack_size);
			printf("Application stack at 0x%08x, size: 0x%008x\n", q, stack_size);
			h = (char *) memman_alloc_4k(memman, heap_size);
			printf("Application heap at 0x%08x, size: 0x%008x\n", h, heap_size);
			setpages(task->section_table, (unsigned char *) APP_ADR              , p, finfo->size, 0b11);
			setpages(task->section_table, (unsigned char *) (APP_ADR + 0x1000000), q, stack_size,  0b11);
			setpages(task->section_table, (unsigned char *) (APP_ADR + 0x2000000), h, heap_size,   0b11);
			invalidate_tlbs();
			start_app(APP_ADR+0x8000, APP_ADR+0x1000000 + stack_size);  // RPi
			shtctl = (struct SHTCTL *) _cons_shtctl;
			for (i=0; i<MAX_SHEETS; i++) {
				sht = &(shtctl->sheets0[i]);
				if ((sht->flags & 0x11) == 0x11 && sht->task == task) {
					/* found a window left open */
					sheet_free(sht);
				}
			}
			for (i = 0; i < 8; i++) {	/* クローズしてないファイルをクローズ */
				if (task->fhandle[i].buf != 0) {
					memman_free_4k(memman, (int) task->fhandle[i].buf, task->fhandle[i].size);
					task->fhandle[i].buf = 0;
				}
			}
			timer_cancelall(&task->fifo);
			setpages(task->section_table, (unsigned char *) APP_ADR            , (unsigned char *) APP_ADR           , finfo->size, 0b00);
			setpages(task->section_table, (unsigned char *) (APP_ADR+0x1000000), (unsigned char *) (APP_ADR+0x1000000), stack_size,  0b00);
			setpages(task->section_table, (unsigned char *) (APP_ADR+0x2000000), (unsigned char *) (APP_ADR+0x2000000), heap_size,   0b00);
			invalidate_tlbs();
			memman_free_4k(memman, (int) q, stack_size);
			memman_free_4k(memman, (int) h, heap_size);
			task->langbyte1 = 0;
		} else {
			cons_putstr0(cons, "Not an ELF file.\n");
		}
		memman_free_4k(memman, (int) p, finfo->size);
		cons_newline(cons);
		return 1;
	}
	return 0;
}


int hrb_api(uint32_t r0, uint32_t r1, uint32_t r2, uint32_t r3, uint32_t r4, uint32_t r5, uint32_t r6)
// r0: func
// r0=1 -> r1: c (putchar)
// r0=1 -> r1: NA, r2: str, (putstr, null stop)
// r0=2 -> r1: NA, r2: str, r3: len (putstr, with len)
// r0=4 -> not done here (app_end)
// r0=5 -> r1: *buf, r2: x, r3: y,  r4: cursor, r5: *title, return: win (make window)
// r0=6 -> r1: win, r2: x, r3: y, r4: col, r5: len, r6: *str (write str to window)
// r0=7 -> r1: win, r2: x0, r3: y0, r4: x1, r5: y1, r6: col (draw rectangular to window)
// r0=8 -> r1: memman, r2: heap start address, r3: heap size (initialize memman for app)
// r0=9 -> r1: memman, r2: size, return: allocated memory pointer (malloc)
// r0=10-> r1: memman, r2: memory pointer, r3: size (free)
// r0=11-> r1: win, r2: x, r3: y, r4: c (put a dot)
// r0=12-> r1: win, r2: x0, r3: y0, r4: x1, r5: y1 (refresh)
// r0=13-> r1: win, r2: x0, r3: y0, r4: x1, r5: y1, r6: color (linewin)
// r0=14-> r1: win (closewin)
// r0=15-> r1: 0(no sleep)/1(sleep), return key code (getkey)
// r0=16-> return: timer handler (alloctimer)
// r0=17-> r1: timer, r2: data (inittimer)
// r0=18-> r1: timer, r2: timer (settimer)
// r0=19-> r1: timer (freetimer)
// r0=20-> r1: tone (beep)
// r0=21-> r1: filename, return: file handle (fileopen)
// r0=22-> r1: filehandle (file close)
// r0=23-> r1: filehandle, r2: (int) size/offset, r3: seekmode (0: from beginning, 1: from current, 2: from end), 
// r0=24-> r1: filehandle, r2: mode (0: size, 1: current position, 2: from end), return: filesize (file size/position)
// r0=25-> r1: buffer address, r2: maximum size in bytes, r3: filehandle, return: read size (file read)
// r0=26-> r1: buffer address, r2: maximum size; return length in bytes (cmdline argument)
// r0=27-> return langmode (getlang)
{
	struct TASK *task = (struct TASK *) task_now();
	struct CONSOLE *cons = task->cons;
	struct SHTCTL *shtctl = (struct SHTCTL *) _cons_shtctl;
	struct SHEET *sht;
	int i, result;
	struct FILEINFO *finfo;
	struct FILEHANDLE *fh;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	char *buf, *title, *str;
	struct FIFO32 *sys_fifo = (struct FIFO32 *) _task_a_fifo;
	/*
	printf("r0: %08x\n", r0);
	printf("r1: %08x\n", r1);
	printf("r2: %08x\n", r2);
	printf("r3: %08x\n", r3);
	printf("r4: %08x\n", r4);
	printf("r5: %08x\n", r5);
	printf("r6: %08x\n", r6);
	*/
	result = 0;
	if (r0 == 1) {
		cons_putchar(cons, r1 & 0xff, 1);
	} else if (r0 == 2) {
		cons_putstr0(cons, (char *) r2);
	} else if (r0 == 3) {
		cons_putstr1(cons, (char *) r2, r3);
	} else if (r0 == 5) {
		buf   = (char *) v2p((uint32_t) task->section_table, r1);
		title = (char *) r5;
		sht = sheet_alloc(shtctl);
		sht->task = task;
		sht->flags |= 0x10;
		sheet_setbuf(sht, buf, r2, r3, r4);
		make_window8(buf, r2, r3, title, 0);
		sheet_slide(sht, ((shtctl->xsize - ((int) r2))/2) & ~3, (shtctl->ysize - ((int) r3))/2);
		sheet_updown(sht, shtctl->top); /* same level as the mouse */
		result = (int) sht;
	} else if (r0 == 6) {
		sht = (struct SHEET *) (r1 & 0xfffffffe);
		str = (char *) r6;
		putfonts8_asc(sht->buf, sht->bxsize, r2, r3, r4, str);
		if ((r1 & 1) == 0) {
			sheet_refresh(sht, r2, r3, r2 + r5*8, r3+16);
		}
	} else if (r0 == 7) {
		sht = (struct SHEET *) (r1 & 0xfffffffe);
		boxfill8(sht->buf, sht->bxsize, r6, r2, r3, r4, r5);
		if ((r1 & 1) == 0) {
			sheet_refresh(sht, r2, r3, r4+1, r5+1);
		}
	} else if (r0 == 8) {
		memman_init((struct MEMMAN *) r1);
		r2 &= 0xfffffff0; /* align 16-byte order */
		memman_free((struct MEMMAN *) r1, r2, r3);
	} else if (r0 == 9) {
		r2 = (r2 + 0x0f) & 0xfffffff0; /* round up to 16 bytes */
	    return memman_alloc((struct MEMMAN *) r1, r2);
	} else if (r0 == 10) {
		r3 = (r3 + 0x0f) & 0xfffffff0;
		memman_free((struct MEMMAN *) r1, r2, r3);
	} else if (r0 == 11) {
		sht = (struct SHEET *) (r1 & 0xfffffffe);
		sht->buf[sht->bxsize * r3 + r2] = r4;
		if ((r1 & 1) == 0) {
			sheet_refresh(sht, r2, r3, r2+1, r3+1);
		}
	} else if (r0 == 12) {
		sht = (struct SHEET *) r1;
		sheet_refresh(sht, r2, r3, r4, r5);
	} else if (r0 == 13) {
		sht = (struct SHEET *) (r1 & 0xfffffffe);
		hrb_api_linewin(sht, r2, r3, r4, r5, r6);
		if ((r1 & 1) == 0) {
			if (r2 > r4) {
				i = r2;
				r2 = r4;
				r4 = i;
			}
			if (r3 > r5) {
				i = r3;
				r3 = r5;
				r5 = i;
			}
			sheet_refresh(sht, r2, r3, r4+1, r5+1);
		}
	} else if (r0 == 14) {
		sheet_free((struct SHEET *) r1);
	} else if (r0 == 15) {
		for(;;) {
			io_cli();
			if (fifo32_status(&task->fifo) == 0) {
				if (r1 != 0) {
					task_sleep(task);
				} else {
					io_sti();
					return -1;
				}
			}
			i = fifo32_get(&task->fifo);
			io_sti();
			if (i <= 1) {
				/* timer for cursor */
				/* no cursor during application execution (request 1)*/
				timer_init(cons->timer, &task->fifo, 1); /* request 1 */
				timer_settime(cons->timer, 50);
			}
			if (i == 2) { /* cursor on */
				cons->cur_c = COL8_FFFFFF;
			}
			if (i == 3) { /* cursor off */
				cons->cur_c = -1;
			}
			if (i == 4) { /* close console only */
				timer_cancel(cons->timer);
				io_cli();
				fifo32_put(sys_fifo, cons->sht - shtctl->sheets0 + 2024);
				cons->sht = 0;
				io_sti();
			}
			if (256 <= i) {
				return i-256;
			}
		}
	} else if (r0 == 16) {
		result = (int) timer_alloc();
		((struct TIMER *) result)->flags2 = 1; /* enable auto cancelling */
	} else if (r0 == 17) {
		timer_init((struct TIMER *) r1, &task->fifo, r2 + 256);
	} else if (r0 == 18) {
		timer_settime((struct TIMER *) r1, r2);
	} else if (r0 == 19) {
		timer_free((struct TIMER *) r1);
	} else if (r0 == 20) {
		beep(r1);
	} else if (r0 == 21) {
		for (i = 0; i < 8; i++) {
			if (task->fhandle[i].buf == 0) {
				break;
			}
		}
		fh = &task->fhandle[i];
		result = 0;
		if (i < 8) {
			finfo = file_search((char *) r1, (struct FILEINFO *) rdedata, 512);
			if (finfo != 0) {
				result = (int) fh;
				fh->buf = (char *) memman_alloc_4k(memman, finfo->size);
				fh->size = finfo->size;
				fh->pos = 0;
				file_loadfile(finfo->clustno, finfo->size, fh->buf, task->fat, usradr);
			}
		}
	} else if (r0 == 22) {
		fh = (struct FILEHANDLE *) r1;
		memman_free_4k(memman, (int) fh->buf, fh->size);
		fh->buf = 0;
	} else if (r0 == 23) {
		fh = (struct FILEHANDLE *) r1;
		if (r3 == 0) {
			fh->pos = (int) r2;
		} else if (r3 == 1) {
			fh->pos += (int) r2;
		} else if (r3 == 2) {
			fh->pos = fh->size + (int) r2;
		}
		if (fh->pos < 0) {
			fh->pos = 0;
		}
		if (fh->pos > fh->size) {
			fh->pos = fh->size;
		}
	} else if (r0 == 24) {
		fh = (struct FILEHANDLE *) r1;
		if (r2 == 0) {
			result = fh->size;
		} else if (r2 == 1) {
			result = fh->pos;
		} else if (r2 == 2) {
			result = fh->pos - fh->size;
		}
	} else if (r0 == 25) {
		fh = (struct FILEHANDLE *) r3;
		for (i = 0; i < r2; i++) {
			if (fh->pos == fh->size) {
				break;
			}
			*((char *) r1 + i) = fh->buf[fh->pos];
			fh->pos++;
		}
		result = i;
	} else if (r0 == 26) {
		i = 0;
		for(;;) {
			*((char *) r1 + i) = task->cmdline[i];
			if (task->cmdline[i] == 0) {
				break;
			}
			if (i >= r2) {
				break;
			}
			i++;
		}
		result = i;
	} else if (r0 == 27) {
		result = task->langmode;
	}
	return result;
}

/* data abort */
int inthandler0d(uint32_t adr, uint32_t *sp, uint32_t cpsr)
{
	struct TASK *task = (struct TASK *) task_now();
	struct CONSOLE *cons = task->cons;
	char s[50];
	sprintf(s, "\nData Abort Exception at %08x\n", adr);
	uart_puts(s);
	sprintf(s, "adr: 0x%08x\n", adr);
	uart_puts(s);
	sprintf(s, "cpsr: 0x%08x\n", cpsr);
	uart_puts(s);
	sprintf(s, "r0:  0x%08x\n", *(sp+2));
	uart_puts(s);
	sprintf(s, "r1:  0x%08x\n", *(sp+3));
	uart_puts(s);
	sprintf(s, "r2:  0x%08x\n", *(sp+4));
	uart_puts(s);
	sprintf(s, "r3:  0x%08x\n", *(sp+5));
	uart_puts(s);
	sprintf(s, "r4:  0x%08x\n", *(sp+6));
	uart_puts(s);
	sprintf(s, "r5:  0x%08x\n", *(sp+7));
	uart_puts(s);
	sprintf(s, "r6:  0x%08x\n", *(sp+8));
	uart_puts(s);
	sprintf(s, "r7:  0x%08x\n", *(sp+9));
	uart_puts(s);
	sprintf(s, "r8:  0x%08x\n", *(sp+10));
	uart_puts(s);
	sprintf(s, "r9:  0x%08x\n", *(sp+11));
	uart_puts(s);
	sprintf(s, "r10: 0x%08x\n", *(sp+12));
	uart_puts(s);
	sprintf(s, "r11: 0x%08x\n", *(sp+13));
	uart_puts(s);
	sprintf(s, "r12: 0x%08x\n", *(sp+14));
	uart_puts(s);
	sprintf(s, "sp_usr:  0x%08x\n", *sp);
	uart_puts(s);
	sprintf(s, "lr_usr:  0x%08x\n", *(sp+1));
	uart_puts(s);
	sprintf(s, "lr_abt: 0x%08x\n", *(sp+15));
	uart_puts(s);
	sprintf(s, "\nData Abort Exception at %08x\n", adr);
	uart_puts(s);
	cons_putstr0(cons, s);
	//	for(;;) {}
	return 1;
}

/* Prefetch abort */
int inthandler0e(uint32_t adr, uint32_t *sp, uint32_t cpsr)
{
	struct TASK *task = (struct TASK *) task_now();
	struct CONSOLE *cons = task->cons;
	char s[50];
	sprintf(s, "\nPrefetch Abort Exception at %08x\n", adr);
	uart_puts(s);
	sprintf(s, "adr: 0x%08x\n", adr);
	uart_puts(s);
	sprintf(s, "cpsr: 0x%08x\n", cpsr);
	uart_puts(s);
	sprintf(s, "r0:  0x%08x\n", *(sp+2));
	uart_puts(s);
	sprintf(s, "r1:  0x%08x\n", *(sp+3));
	uart_puts(s);
	sprintf(s, "r2:  0x%08x\n", *(sp+4));
	uart_puts(s);
	sprintf(s, "r3:  0x%08x\n", *(sp+5));
	uart_puts(s);
	sprintf(s, "r4:  0x%08x\n", *(sp+6));
	uart_puts(s);
	sprintf(s, "r5:  0x%08x\n", *(sp+7));
	uart_puts(s);
	sprintf(s, "r6:  0x%08x\n", *(sp+8));
	uart_puts(s);
	sprintf(s, "r7:  0x%08x\n", *(sp+9));
	uart_puts(s);
	sprintf(s, "r8:  0x%08x\n", *(sp+10));
	uart_puts(s);
	sprintf(s, "r9:  0x%08x\n", *(sp+11));
	uart_puts(s);
	sprintf(s, "r10: 0x%08x\n", *(sp+12));
	uart_puts(s);
	sprintf(s, "r11: 0x%08x\n", *(sp+13));
	uart_puts(s);
	sprintf(s, "r12: 0x%08x\n", *(sp+14));
	uart_puts(s);
	sprintf(s, "sp_usr:  0x%08x\n", *sp);
	uart_puts(s);
	sprintf(s, "lr_usr:  0x%08x\n", *(sp+1));
	uart_puts(s);
	sprintf(s, "lr_abt: 0x%08x\n", *(sp+15));
	uart_puts(s);
	sprintf(s, "\nPrefetch Abort Exception at %08x\n", adr);
	uart_puts(s);
	cons_putstr0(cons, s);
	//	for(;;) {}
	return 1;
}

/* Undefined instruction */
int inthandler0f(uint32_t adr)
{
	struct TASK *task = (struct TASK *) task_now();
	struct CONSOLE *cons = task->cons;
	char s[50];
	sprintf(s, "\nUndefined Instruction Exception at %08x\n", adr);
	uart_puts(s);
	cons_putstr0(cons, s);
	//	for(;;) {}
	return 1;

}

void hrb_api_linewin(struct SHEET *sht, int x0, int y0, int x1, int y1, int col)
{
	int i, x, y, len, dx, dy;

	dx = x1 - x0;
	dy = y1 - y0;
	x = x0 << 10;
	y = y0 << 10;
	if (dx < 0) {
		dx = - dx;
	}
	if (dy < 0) {
		dy = - dy;
	}
	if (dx >= dy) {
		len = dx + 1;
		if (x0 > x1) {
			dx = -1024;
		} else {
			dx =  1024;
		}
		if (y0 <= y1) {
			dy = ((y1 - y0 + 1) << 10) / len;
		} else {
			dy = ((y1 - y0 - 1) << 10) / len;
		}
	} else {
		len = dy + 1;
		if (y0 > y1) {
			dy = -1024;
		} else {
			dy =  1024;
		}
		if (x0 <= x1) {
			dx = ((x1 - x0 + 1) << 10) / len;
		} else {
			dx = ((x1 - x0 - 1) << 10) / len;
		}
	}

	for (i = 0; i < len; i++) {
		sht->buf[(y >> 10) * sht->bxsize + (x >> 10)] = col;
		x += dx;
		y += dy;
	}

	return;
}
