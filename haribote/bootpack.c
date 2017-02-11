//#include <stdlib.h>
#include "bootpack.h"
#include "mylib.h" // RPi only
#include "mailbox.h" // RPi only

void keywin_off(struct SHEET *key_win);
void keywin_on(struct SHEET *key_win);
void close_console(struct SHEET *sht);
void close_constask(struct TASK *task);

/* RPi only */
struct BOOTINFO binfo0; // RPi only
void printMessage(char *s);
extern uint32_t _cur_pcb;
extern uint32_t _next_pcb;
extern uint32_t next_pcb;
char message[1024];
void init_filesystem(struct MEMMAN *memman);
extern uint32_t _cons_shtctl;
uint32_t _task_a_fifo;
uint32_t _task_a_nihongo;
extern uint32_t sectorsPerFAT;
extern long long fatadr, usradr;
extern char *rdedata;

extern void beep(int);

/* RPi only end */

void HariMain(void)
{
	struct BOOTINFO *binfo;  // RPi
	struct SHTCTL *shtctl;
	char s[40];
	struct FIFO32 fifo;
	int fifobuf[128], *cons_fifo[2];
	int mx, my, i, new_mx = -1, new_my = 0, new_wx = 0x7fffffff, new_wy = 0;
	unsigned int memtotal;
	struct MOUSE_DEC mdec;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	unsigned char *buf_back, buf_mouse[256], *buf_cons[2];
	struct SHEET *sht_back, *sht_mouse;
	struct TASK *task_a, *task_cons[2], *task;
	int j, x, y, mmx = -1, mmy = -1, mmx2 = 0;
	struct SHEET *sht = 0, *key_win, *sht2;
	uint16_t *fat;
	unsigned char *nihongo;
	struct FILEINFO *finfo;
	extern char hankaku[4096];

	init_gdtidt();
	uart_init();
	init_timer();
	
	next_pcb = 0; // RPi only
	init_pic(); 
	//	io_sti();  // TEST
	fifo32_init(&fifo, 128, fifobuf, 0);
	_task_a_fifo = (uint32_t) &fifo;
	init_pit();
	init_keyboard(&fifo, 256);
	enable_mouse(&fifo, 512, &mdec);
	io_out8(PIC0_IMR, 0xf8); // dummy
	io_out8(PIC1_IMR, 0xef); // dummy

	memtotal = memtest(0x00400000, 0xbfffffff);
	memman_init(memman);
	//memman_free(memman, 0x00001000, 0x0009e000); /* 0x00001000 - 0x0009efff */
	memman_free(memman, 0x00400000, memtotal - 0x00400000);
	//	memman_monitor(memman);  // TEST

	init_filesystem(memman);
	//	io_sti();
	
	binfo = &binfo0; // RPi only, instead of ADR_BOOTINFO
	initializeFrameBuffer(800, 480, COLORDEPTH, binfo); // RPi only
	init_palette();
	shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
	task_a = task_init(memman);
	fifo.task = task_a;
	task_run(task_a, 1, 2);
	_cons_shtctl = (uint32_t) shtctl;
	task_a->langmode = 0;

	/* sht_back */
	sht_back  = sheet_alloc(shtctl);
	buf_back  = (unsigned char *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
	sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1);
	init_screen8(buf_back, binfo->scrnx, binfo->scrny);

	/* sht_cons */
	key_win = open_console(shtctl, memtotal);
	
	/* sht_mouse */
	sht_mouse = sheet_alloc(shtctl);
	sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
	init_mouse_cursor8(buf_mouse, 99);
	mx = (binfo->scrnx - 16) / 2; /* 画面中央になるように座標計算 */
	my = (binfo->scrny - 28 - 16) / 2;

	sheet_slide(sht_back, 0, 0);
	sheet_slide(key_win, 32,  4);
	sheet_slide(sht_mouse, mx, my);
	sheet_updown(sht_back,     0);
	sheet_updown(key_win,  1);
	sheet_updown(sht_mouse,    2);
	keywin_on(key_win);

	interrupt_test();  // RPi only this is not test routine
	io_sti();
	
	/* nihongo.fntの読み込み */
	nihongo = (unsigned char *) memman_alloc_4k(memman, 16 * 256 + 32 * 94 * 47);
	fat = (uint16_t *) memman_alloc_4k(memman, sectorsPerFAT*512);
	file_readfat(fat, fatadr);
	finfo = file_search("nihongo.fnt", (struct FILEINFO *) rdedata, 512);
	if (finfo != 0) {
		file_loadfile(finfo->clustno, finfo->size, nihongo, fat, usradr);
	} else {
		for (i = 0; i < 16 * 256; i++) {
			nihongo[i] = hankaku[i]; /* フォントがなかったので半角部分をコピー */
		}
		for (i = 16 * 256; i < 16 * 256 + 32 * 94 * 47; i++) {
			nihongo[i] = 0xff; /* フォントがなかったので全角部分を0xffで埋め尽くす */
		}
	}
	_task_a_nihongo = (int) nihongo;
	memman_free_4k(memman, (int) fat, sectorsPerFAT*512);
	
	for(;;) {
		io_cli();
		printMessage(message);  // TEST
		if (fifo32_status(&fifo) == 0) {
			/* FIFOがからっぽになったので、保留している描画があれば実行する */
			if (new_mx >= 0) {
				io_sti();
				sheet_slide(sht_mouse, new_mx, new_my);
				new_mx = -1;
			} else if (new_wx != 0x7fffffff) {
				io_sti();
				sheet_slide(sht, new_wx, new_wy);
				new_wx = 0x7fffffff;
			} else {
				task_sleep(task_a);
				io_sti();
			}
		} else {
			i = fifo32_get(&fifo); 
			io_sti();
			if (key_win != 0 && key_win->flags == 0) {	/* ウィンドウが閉じられた */
				if (shtctl->top == 1) {
					key_win = 0; /* there only a mouse cursor and background */
				} else {
					key_win = shtctl->sheets[shtctl->top - 1];
					keywin_on(key_win);
				}
			}
			if (i - 256 == 0) {
				KeyboardUpdate(); // RPi
			} else if (256 <= i && i<=511) { /* keyboard data */
				if ('\a'+256 <= i && i<127 + 256) {
					s[0] = i - 256;
				} else {
					s[0] = 0;
				}
				if (s[0]!=0 && key_win != 0) {
					fifo32_put(&key_win->task->fifo, s[0]+256);
				}
				if (i == 256 + '\t' && key_win != 0) {	/* Tab */
					keywin_off(key_win);
					j = key_win->height - 1;
					if (j == 0) {
						j = shtctl->top - 1;
					}
					key_win = shtctl->sheets[j];
					keywin_on(key_win);
				}
				if (i == 256 + 254 && key_win != 0) {
					task = key_win->task;
					if (task != 0 && task->tss.run_flag != 0) {	/* Shift+F1 */
						cons_putstr0(task->cons, "\nBreak (key) :\n");
						io_cli();
						task->tss.cpsr |= 0x1F;  // move to system mode
						task->tss.pc = (uint32_t) asm_end_app+4;
						io_sti();
						task_run(task, -1, 0);
					}
				}
				if (i == 256 + 252) {   /* Shift + F2 */
					/* select the new console */
					if (key_win != 0) {
						keywin_off(key_win);
					}
					key_win = open_console(shtctl, memtotal);
					sheet_slide(key_win, 32, 4);
					sheet_updown(key_win, shtctl->top);
					keywin_on(key_win);
				}
				if (i == 256 + 253) {	/* F11 */
					sheet_updown(shtctl->sheets[1], shtctl->top - 1);
				}
			} else if (512 <= i && i<=767) { /* mouse data */
				if (mouse_decode(&mdec, i-512) != 0) {
					/* Mouse data available */
					mx += mdec.x;
					my += mdec.y;
					if (mx < 0) {
						mx = 0;
					}
					if (my < 0) {
						my = 0;
					}
					if (mx > binfo->scrnx - 1) {
						mx = binfo->scrnx - 1;
					}
					if (my > binfo->scrny - 1) {
						my = binfo->scrny - 1;
					}
					new_mx = mx;
					new_my = my;
					if ((mdec.btn & 0x01) != 0) {
						/* 左ボタンを押している */
						if (mmx < 0) {
							/* 通常モードの場合 */
							/* 上の下じきから順番にマウスが指している下じきを探す */
							for (j = shtctl->top - 1; j > 0; j--) {
								sht = shtctl->sheets[j];
								x = mx - sht->vx0;
								y = my - sht->vy0;
								if (0 <= x && x < sht->bxsize && 0 <= y && y < sht->bysize) {
									if (sht->buf[y * sht->bxsize + x] != sht->col_inv) {
										sheet_updown(sht, shtctl->top - 1);
										if (sht != key_win) {
											keywin_off(key_win);
											key_win = sht;
											keywin_on(key_win);
										}
										if (3 <= x && x < sht->bxsize - 3 && 3 <= y && y < 21) {
											mmx = mx;	/* ウィンドウ移動モードへ */
											mmy = my;
											mmx2 = sht->vx0;
											new_wy = sht->vy0;
										}
										if (sht->bxsize - 21 <= x && x < sht->bxsize - 5 && 5 <= y && y < 19) {
											/* 「×」ボタンクリック */
											if ((sht->flags & 0x10) != 0) {	/* アプリが作ったウィンドウか？ */
												task = sht->task;
												cons_putstr0(task->cons, "\nBreak(mouse) :\n");
												io_cli();	/* 強制終了処理中にタスクが変わると困るから */
												task->tss.cpsr |= 0x1F;  // move to system mode
												task->tss.pc = (uint32_t) asm_end_app+4;
												io_sti();
												task_run(task, -1, 0);
											} else { /* console */
												task = sht->task;
												sheet_updown(sht, -1); /* hide the console */
												keywin_off(key_win);
												key_win = shtctl->sheets[shtctl->top - 1];
												keywin_on(key_win);
												io_cli();
												fifo32_put(&task->fifo, 4);
												io_sti();
											}
										}
										break;
									}
								}
							}
						} else {
							/* ウィンドウ移動モードの場合 */
							x = mx - mmx;	/* マウスの移動量を計算 */
							y = my - mmy;
							new_wx = (mmx2 + x + 2) & ~3;
							new_wy = new_wy + y;
							mmy = my;	/* 移動後の座標に更新 */
						}
					} else {
						/* 左ボタンを押していない */
						mmx = -1;	/* 通常モードへ */
						if (new_wx != 0x7fffffff) {
							sheet_slide(sht, new_wx, new_wy);	/* 一度確定させる */
							new_wx = 0x7fffffff;
						}
					}
				}
			} else if (768 <= i && i <= 1023) { /* exit */
				close_console(shtctl->sheets0 + (i - 768));
			} else if (1024 <= i && i <= 2023) {
				close_constask(taskctl->tasks0 + (i - 1024));
			} else if (2024 <= i && i <= 2279) {
				sht2 = shtctl->sheets0 + (i - 2024);
				memman_free_4k(memman, (uint32_t) sht2->buf, 256*165);
				sheet_free(sht2);
			}
		}
	}
}

void keywin_off(struct SHEET *key_win)
{
	change_wtitle8(key_win, 0);
	if ((key_win->flags & 0x20) != 0) {
		fifo32_put(&key_win->task->fifo, 3); /* コンソールのカーソルOFF */
	}
	return;
}

void keywin_on(struct SHEET *key_win)
{
	change_wtitle8(key_win, 1);
	if ((key_win->flags & 0x20) != 0) {
		fifo32_put(&key_win->task->fifo, 2); /* コンソールのカーソルON */
	}
	return;
}

struct TASK *open_constask(struct SHEET *sht, unsigned int memtotal)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct TASK *task = task_alloc();
	int	*cons_fifo = (int *) memman_alloc_4k(memman, 128*4);
	//	printf("task->tss: 0x%08x\n", i, &(task->tss)); // TEST
	task->cons_stack = memman_alloc_4k(memman, 64 * 1024);
	task->tss.sp = task->cons_stack + 64*1024; // RPi
	task->cons_stack_svc = memman_alloc_4k(memman, 64 * 1024);  // RPi
	task->tss.sp_svc = task->cons_stack_svc + 64 * 1024;  // RPi
	task->tss.pc = (uint32_t) (&console_task + 4);	// RPi
	task->tss.r0 = (uint32_t) sht;	// RPi
	task->tss.r1 = (uint32_t) memtotal;  // RPi
	// virtual memory // RPi
	task->section_table = (unsigned char *) memman_alloc_4k(memman, 8*4096); // RPi
	task->page_table = (unsigned char *) memman_alloc_4k(memman, 64*256*4);  // RPi allocate 64MB space for application
	generate_section_table(task->section_table);
	generate_page_table(task->section_table, 2048, 64, task->page_table, (unsigned char *) 0x80000000, 0b00, 1, 1);
	task->tss.vmem_table = (uint32_t) task->section_table;
	task_run(task, 2, 2); /* level=2, priority=2 */
	fifo32_init(&task->fifo, 128, cons_fifo, task);
	return task;
}

struct SHEET *open_console(struct SHTCTL *shtctl, unsigned int memtotal)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct SHEET *sht = sheet_alloc(shtctl);
	unsigned char *buf = (unsigned char *) memman_alloc_4k(memman, 256 * 165);
	sheet_setbuf(sht, buf, 256, 165, -1); /* §ŸFÈµ */
	make_window8(buf, 256, 165, "console", 0);
	make_textbox8(sht, 8, 28, 240, 128, COL8_000000);
	sht->task = open_constask(sht, memtotal);
	sht->flags |= 0x20; /* with cursor */
	return sht;
}

void close_constask(struct TASK *task)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	task_sleep(task);
	memman_free_4k(memman, task->cons_stack, 64*1024);
	memman_free_4k(memman, task->cons_stack_svc, 64*1024);
	memman_free_4k(memman, (int) task->fifo.buf, 128*4);
	task->flags = 0; /* task_free(task); の代わり */
	return;
}

void close_console(struct SHEET *sht)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct TASK *task = sht->task;
	memman_free_4k(memman, (uint32_t) sht->buf, 256 * 165);
	sheet_free(sht);
	close_constask(task);
	memman_free_4k(memman, (uint32_t) task->section_table, 8*4096);
	memman_free_4k(memman, (uint32_t) task->page_table, 64*256*4);
	return;
}


void printMessage(char *msg)
{
	char s[1024];
	if (*msg=0) {
		return;
	}
 	strcpy(s, msg);
	msg[0]=0;
	io_sti();
 	printf("%s", s);
	io_cli();
}


