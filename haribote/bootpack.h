#ifndef BOOTPACK_H
#define BOOTPACK_H

#if !defined(__cplusplus)
#include <stdbool.h>
#endif
#if defined(__cplusplus)
extern "C" /* Use C linkage for kernel_main. */
#endif
#include <stddef.h>
#include <stdint.h>

// this struct is introduced for compatibility with Haribote
struct BOOTINFO {
	char cyls, leds, vmode, reserve;
	short scrnx, scrny;
	char *vram;
};

extern int _FrameBufferInfo;
// for USPi library
typedef void TInterruptHandler (void *pParam);
void ConnectInterrupt (unsigned nIRQ, TInterruptHandler *pHandler, void *pParam); // USPi uses USB IRQ 9
#define COLORDEPTH      8
int initializeFrameBuffer(int width, int height, int bitDepth, struct BOOTINFO *pbinfo);

/* boot.nas */ // boot.nas is replaced with boot.S
void io_cli(void); // = disable_IRQ
void io_sti(void); // = enable_IRQ
void io_hlt(void);
void io_stihlt(void); // = enable_IRQ_then_Halt
int io_in8(int port);  // return 0. Only for compatibility with Haribote
int io_out8(int port, int data); // do nothing. Only for compatibility with Haribote
int io_load_eflags(void); // = getmode()
void io_store_eflags(int eflags);  // = setmode(int)
int getmode();
void setmode(int);
void farjmp(char *adr);
void asm_end_app();

void _hangup(void);
void switchPCB(uint32_t next_pcb);

/* fifo.c */
struct FIFO32 {
	int *buf;
	int p, q, size, free, flags;
	struct TASK *task;
};
void fifo32_init(struct FIFO32 *fifo, int size, int *buf, struct TASK *task);
int fifo32_put(struct FIFO32 *fifo, int data);
int fifo32_get(struct FIFO32 *fifo);
int fifo32_status(struct FIFO32 *fifo);

/* graphic.c */
void init_palette(void);
void boxfill8(char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1);
void init_screen8(char *vram, int x, int y);
void putfont8(char *vram, int xsize, int x, int y, char c, char *font);
void putfonts8_asc(char *vram, int xsize, int x, int y, char c, char *s);
void init_mouse_cursor8(char *mouse, char bc);
void putblock8_8(char *vram, int vxsize, int pxsize, int pysize, int px0, int py0, char *buf, int bxsize);

#define COL8_000000		0
#define COL8_FF0000		1
#define COL8_00FF00		2
#define COL8_FFFF00		3
#define COL8_0000FF		4
#define COL8_FF00FF		5
#define COL8_00FFFF		6
#define COL8_FFFFFF		7
#define COL8_C6C6C6		8
#define COL8_840000		9
#define COL8_008400		10
#define COL8_848400		11
#define COL8_000084		12
#define COL8_840084		13
#define COL8_008484		14
#define COL8_848484		15
extern char hankaku[4096];


/* dsctbl.c */ // this file is not needed in RPi. instead bridge.c holds the functions
void init_gdtidt(void);

/* int.c */
void init_pic(void);
//void inthandler27(int *esp);

#define INTERRUPT_BASE					(0x0000B000)
#define INTERRUPT_IRQ_BASIC_PENDING		((vu32_t *)PHY_PERI_ADDR(INTERRUPT_BASE + 0x200))
#define INTERRUPT_IRQ_PENDING1			((vu32_t *)PHY_PERI_ADDR(INTERRUPT_BASE + 0x204))
#define INTERRUPT_IRQ_PENDING2			((vu32_t *)PHY_PERI_ADDR(INTERRUPT_BASE + 0x208))
#define INTERRUPT_FIQ_CTRL				((vu32_t *)PHY_PERI_ADDR(INTERRUPT_BASE + 0x20C))
#define INTERRUPT_ENABLE_IRQS1			((vu32_t *)PHY_PERI_ADDR(INTERRUPT_BASE + 0x210))
#define INTERRUPT_ENABLE_IRQS2			((vu32_t *)PHY_PERI_ADDR(INTERRUPT_BASE + 0x214))
#define INTERRUPT_ENABLE_BASIC_IRQS		((vu32_t *)PHY_PERI_ADDR(INTERRUPT_BASE + 0x218))
#define INTERRUPT_DISABLE_IRQS1			((vu32_t *)PHY_PERI_ADDR(INTERRUPT_BASE + 0x21C))
#define INTERRUPT_DISABLE_IRQS2			((vu32_t *)PHY_PERI_ADDR(INTERRUPT_BASE + 0x220))
#define INTERRUPT_DISABLE_BASIC_IRQS	((vu32_t *)PHY_PERI_ADDR(INTERRUPT_BASE + 0x224))

#define PIC0_ICW1		0x0020
#define PIC0_OCW2		0x0020
#define PIC0_IMR		0x0021
#define PIC0_ICW2		0x0021
#define PIC0_ICW3		0x0021
#define PIC0_ICW4		0x0021
#define PIC1_ICW1		0x00a0
#define PIC1_OCW2		0x00a0
#define PIC1_IMR		0x00a1
#define PIC1_ICW2		0x00a1
#define PIC1_ICW3		0x00a1
#define PIC1_ICW4		0x00a1

void interrupt_test(void); // test function
unsigned int hold_IRQ();
void unhold_IRQ(unsigned int irq_status);

/* keyboard.c */
void init_keyboard(struct FIFO32 *fifo, int data0);
void KeyboardUpdate();

/* mouse.c */
struct MOUSE_POS {
	int x, y, btn;
};

struct MOUSE_DEC {
	unsigned char buf[3], phase; // unnecessary for RPi ver
	int x, y, btn;
};
void enable_mouse(struct FIFO32 *fifo, int data0, struct MOUSE_DEC *mdec);
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat);;

/* memory.c */
#define MEMMAN_FREES		4090	/* これで約32KB */
#define MEMMAN_ADDR			0x003c0000
struct FREEINFO {	/* あき情報 */
	unsigned int addr, size;
};
struct MEMMAN {		/* メモリ管理 */
	unsigned int frees, maxfrees, lostsize, losts;
	struct FREEINFO free[MEMMAN_FREES];
};
unsigned int memtest(unsigned int start, unsigned int end);
void memman_init(struct MEMMAN *man);
unsigned int memman_total(struct MEMMAN *man);
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size);
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size);
unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size);
int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size);
//for debug
void memman_monitor(struct MEMMAN *man);
// for compatibility with USPi
void *malloc (unsigned nSize);		// result must be 4-byte aligned
void free (void *pBlock);

/* sheet.c */
#define MAX_SHEETS		256
struct SHEET {
	unsigned char *buf;
	int bxsize, bysize, vx0, vy0, col_inv, height, flags;
	struct SHTCTL *ctl;
	struct TASK *task;
};
struct SHTCTL {
	unsigned char *vram, *map;
	int xsize, ysize, top;
	struct SHEET *sheets[MAX_SHEETS];
	struct SHEET sheets0[MAX_SHEETS];
};
struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize);
struct SHEET *sheet_alloc(struct SHTCTL *ctl);
void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv);
void sheet_updown(struct SHEET *sht, int height);
void sheet_refresh(struct SHEET *sht, int bx0, int by0, int bx1, int by1);
void sheet_slide(struct SHEET *sht, int vx0, int vy0);
void sheet_free(struct SHEET *sht);

/* timer.c */
#define MAX_TIMER		500
struct TIMER {
	struct TIMER *next;
	unsigned int timeout;
	char flags, flags2;
	struct FIFO32 *fifo;
	int data;
};
struct TIMERCTL {
	unsigned int count, next, using;
	struct TIMER *t0;
	struct TIMER timers0[MAX_TIMER];
};
extern struct TIMERCTL timerctl;
void init_pit(void);
struct TIMER *timer_alloc(void);
void timer_free(struct TIMER *timer);
void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data);
void timer_settime(struct TIMER *timer, unsigned int timeout);
void inthandler20(int *esp);

/* mtask.c */
#define MAX_TASKS		1000	/* 最大タスク数 */
#define TASK_GDT0		3		/* TSSをGDTの何番から割り当てるのか */
#define MAX_TASKS_LV	100
#define MAX_TASKLEVELS	10
struct TSS32 {
	uint32_t cpsr,     pc,         r0,     r1;
	uint32_t r2,       r3,         r4,     r5;
	uint32_t r6,       r7,         r8,     r9;
	uint32_t r10,      r11,        r12,    sp;
	uint32_t lr,       sp_svc,     sp_sys, sp_usr;
	uint32_t run_flag, vmem_table;
};
struct TASK {
	int sel, flags; /* selはGDTの番号のこと */
	int level, priority;
	struct FIFO32 fifo;
	struct TSS32 tss;
	struct CONSOLE *cons;
	int cons_stack, cons_stack_svc;
	struct FILEHANDLE *fhandle;
	uint32_t *fat;
	char *cmdline;
	unsigned char langmode, langbyte1;
	unsigned char *section_table, *page_table; // RPi
};

struct FILEHANDLE {
	char *buf;
	int size;
	int pos;
};

struct TASKLEVEL {
	int running; /* 動作しているタスクの数 */
	int now; /* 現在動作しているタスクがどれだか分かるようにするための変数 */
	struct TASK *tasks[MAX_TASKS_LV];
};
struct TASKCTL {
	int now_lv; /* 現在動作中のレベル */
	char lv_change; /* 次回タスクスイッチのときに、レベルも変えたほうがいいかどうか */
	struct TASKLEVEL level[MAX_TASKLEVELS];
	struct TASK tasks0[MAX_TASKS];
};
extern struct TASKCTL *taskctl;
extern struct TIMER *task_timer;
struct TASK *task_init(struct MEMMAN *memman);
struct TASK *task_alloc(void);
void task_run(struct TASK *task, int level, int priority);
void task_switch(void);
void task_sleep(struct TASK *task);

/* window.c */
void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char act);
void putfonts8_asc_sht(struct SHEET *sht, int x, int y, int c, int b, char *s, int l);
void make_textbox8(struct SHEET *sht, int x0, int y0, int sx, int sy, int c);
void make_wtitle8(unsigned char *buf, int xsize, char *title, char act);
void change_wtitle8(struct SHEET *sht, char act);

/* console.c */
struct CONSOLE {
	struct SHEET *sht;
	int cur_x, cur_y, cur_c;
	struct TIMER *timer;
};
void console_task(struct SHEET *sheet, unsigned int memtotal);
void cons_putchar(struct CONSOLE *cons, int chr, char move);
void cons_newline(struct CONSOLE *cons);
void cons_putstr0(struct CONSOLE *cons, char *s);
void cons_putstr1(struct CONSOLE *cons, char *s, int l);
void cons_runcmd(char *cmdline, struct CONSOLE *cons, uint32_t *fat, unsigned int memtotal);
void cmd_mem(struct CONSOLE *cons, unsigned int memtotal);
void cmd_cls(struct CONSOLE *cons);
void cmd_dir(struct CONSOLE *cons);
void cmd_type(struct CONSOLE *cons, uint32_t *fat, char *cmdline);
void cmd_exit(struct CONSOLE *cons, uint32_t *fat);
void cmd_start(struct CONSOLE *cons, char *cmdline, int memtotal);
void cmd_ncst(struct CONSOLE *cons, char *cmdline, int memtotal);
void cmd_langmode(struct CONSOLE *cons, char *cmdline);
int cmd_app(struct CONSOLE *cons, uint32_t *fat, char *cmdline);
int inthandler0d(uint32_t, uint32_t *, uint32_t);
int inthandler0e(uint32_t, uint32_t *, uint32_t);
int inthandler0f();
void invalidate_tlbs();
void hrb_api_linewin(struct SHEET *sht, int x0, int y0, int x1, int y1, int col);

/* file.c */
struct FILEINFO {
	uint8_t name[8], ext[3], type;
	char reserve[10];
	uint16_t time, date, clustno;
	uint32_t size;
};

void file_readfat(struct MEMMAN *memman, uint32_t *fat, long long img);
void file_loadfile(int clustno, int size, char *buf, uint32_t *fat, long long img);
struct FILEINFO *file_search(char *name, struct FILEINFO *finfo, int max);
uint32_t FAT_type;
uint32_t fat_bytesize;

/* bootpack.c */
struct TASK *open_constask(struct SHEET *sht, unsigned int memtotal);
struct SHEET *open_console(struct SHTCTL *shtctl, unsigned int memtotal);


/* -------------- Below are for RPi only -------------*/
/* bridge.c */
int initializeFrameBuffer(int width, int height, int bitDepth, struct BOOTINFO *pbinfo);
void generate_section_table(unsigned char *);
void generate_page_table(unsigned char *section_table_adr, int section_st, int sections, unsigned char *page_table_adr, unsigned char *physical_adr, int ap, int b, int c);
int setpages(unsigned char *section_table_adr, unsigned char *va_st, unsigned char *pa_st, uint32_t size, char ap);
unsigned char *section_table_os, *page_table_os;
uint32_t v2p(uint32_t section_table_adr, uint32_t vadr);

/* uart.c */ // RPi original
void uart_init();

/* timer_rp.c */ // RPi original
void init_timer(void);
void timerIRQ_handler(void *pParam);
void timer_kbd_handler(unsigned int hTimer, void *pParam, void *pContext);
void timer_mouse_handler(unsigned int hTimer, void *pParam, void *pContext);
void timer_uart_handler(unsigned int hTimer, void *pParam, void *pContext);
void timer_led_handler(unsigned int hTimer, void *pParam, void *pContext);
// for USPi library
typedef void TKernelTimerHandler (unsigned hTimer, void *pParam, void *pContext);
// for USPi library
// returns the timer handle (hTimer)
unsigned StartKernelTimer (unsigned	        nHzDelay,	// in HZ units (see "system configuration" above)
						   TKernelTimerHandler *pHandler,
						   void *pParam, void *pContext);	// handed over to the timer handler
void CancelKernelTimer (unsigned hTimer);
// Timer(ARM side) peripheral
typedef volatile unsigned int vu32_t ;
#define PHY_PERI_ADDR(x) (0x20000000 + (x))
#define TIMER_BASE			(0x0000B000)
#define TIMER_LOAD			((vu32_t *)PHY_PERI_ADDR(TIMER_BASE + 0x400))
#define TIMER_VALUE			((vu32_t *)PHY_PERI_ADDR(TIMER_BASE + 0x404))
#define TIMER_CONTROL		((vu32_t *)PHY_PERI_ADDR(TIMER_BASE + 0x408))
#define TIMER_IRQ_CLR		((vu32_t *)PHY_PERI_ADDR(TIMER_BASE + 0x40C))
#define TIMER_RAWIRQ		((vu32_t *)PHY_PERI_ADDR(TIMER_BASE + 0x410))
#define TIMER_MASKEDIRQ		((vu32_t *)PHY_PERI_ADDR(TIMER_BASE + 0x414))
#define TIMER_RELOAD		((vu32_t *)PHY_PERI_ADDR(TIMER_BASE + 0x418))
#define TIMER_PREDIVIDER	((vu32_t *)PHY_PERI_ADDR(TIMER_BASE + 0x41C))
#define TIMER_FREECOUNTER	((vu32_t *)PHY_PERI_ADDR(TIMER_BASE + 0x420))


// sound.c
void beep(int);

#endif
