#include "bootpack.h"
#include "mylib.h"

struct TASKCTL *taskctl;
struct TIMER *task_timer;

extern uint32_t next_pcb; // RPi original 
extern uint32_t _cur_pcb; // RPi original
extern void _task_switch_asm(uint32_t); // RPi original

struct TASK *task_now(void)
{
	struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];
	return tl->tasks[tl->now];
}

void task_add(struct TASK *task)
{
	struct TASKLEVEL *tl = &taskctl->level[task->level];
	tl->tasks[tl->running] = task;
	tl->running++;
	task->flags = 2; /* 動作中 */
	return;
}

void task_remove(struct TASK *task)
{
	int i;
	struct TASKLEVEL *tl = &taskctl->level[task->level];

	/* taskがどこにいるかを探す */
	for (i = 0; i < tl->running; i++) {
		if (tl->tasks[i] == task) {
			/* ここにいた */
			break;
		}
	}

	tl->running--;
	if (i < tl->now) {
		tl->now--; /* ずれるので、これもあわせておく */
	}
	if (tl->now >= tl->running) {
		/* nowがおかしな値になっていたら、修正する */
		tl->now = 0;
	}
	task->flags = 1; /* スリープ中 */

	/* ずらし */
	for (; i < tl->running; i++) {
		tl->tasks[i] = tl->tasks[i + 1];
	}

	return;
}

void task_switchsub(void)
{
	int i;
	/* 一番上のレベルを探す */
	for (i = 0; i < MAX_TASKLEVELS; i++) {
		if (taskctl->level[i].running > 0) {
			break; /* 見つかった */
		}
	}
	taskctl->now_lv = i;
	taskctl->lv_change = 0;
	return;
}

void task_idle(void)
{
	for (;;) {
		io_hlt();
	}
}

struct TASK *task_init(struct MEMMAN *memman)
{
	int i;
	struct TASK *task, *idle;
	//	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;
	taskctl = (struct TASKCTL *) memman_alloc_4k(memman, sizeof (struct TASKCTL));
	for (i = 0; i < MAX_TASKS; i++) {
		taskctl->tasks0[i].flags = 0;
		taskctl->tasks0[i].sel = (TASK_GDT0 + i) * 8;
		//		set_segmdesc(gdt + TASK_GDT0 + i, 103, (int) &taskctl->tasks0[i].tss, AR_TSS32);
	}
	for (i = 0; i < MAX_TASKLEVELS; i++) {
		taskctl->level[i].running = 0;
		taskctl->level[i].now = 0;
	}

	task = task_alloc();
	printf("OS task TSS:%08x\n", &(task->tss));   //TEST
	task->tss.sp     = 0x00400000;
	task->tss.sp_svc = 0x00380000;
	task->flags = 2;	/* 動作中マーク */
	task->priority = 2; /* 0.02秒 */
	task->level = 0;	/* 最高レベル */
	task_add(task);
	task_switchsub();	/* レベル設定 */
	//	load_tr(task->sel);
	_cur_pcb = (uint32_t) &(task->tss); // RPi only
	task_timer = timer_alloc();
	timer_settime(task_timer, task->priority);

	idle = task_alloc();
	printf("idle->tss :%08x\n", &(idle->tss));   //TEST
	idle->tss.sp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024;
	idle->tss.sp_svc = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024;
	idle->tss.pc = ((uint32_t) &task_idle) + 4;
	task_run(idle, MAX_TASKLEVELS - 1, 1);

	return task;
}

struct TASK *task_alloc(void)
{
	int i;
	struct TASK *task;
	for (i = 0; i < MAX_TASKS; i++) {
		if (taskctl->tasks0[i].flags == 0) {
			task = &taskctl->tasks0[i];
			task->flags = 1; /* 使用中マーク */
			task->tss.cpsr = 0x0000005F; /* RPi only IRQ enable, FIQ disable, System mode */
			task->tss.pc  = 0; // RPi only Other registers are set to 0 for now
			task->tss.r0  = 0; // RPi only
			task->tss.r1  = 0; // RPi only
			task->tss.r2  = 0; // RPi only
			task->tss.r3  = 0; // RPi only
			task->tss.r4  = 0; // RPi only
			task->tss.r5  = 0; // RPi only
			task->tss.r6  = 0; // RPi only
			task->tss.r7  = 0; // RPi only
			task->tss.r8  = 0; // RPi only
			task->tss.r9  = 0; // RPi only
			task->tss.r10 = 0; // RPi only
			task->tss.r11 = 0; // RPi only
			task->tss.r12 = 0; // RPi only
			task->tss.sp  = 0; // RPi only
			task->tss.lr  = 0; // RPi only
			task->tss.sp_svc = 0;
			task->tss.sp_sys = 0;
			task->tss.sp_usr = 0;
			task->tss.run_flag = 0;
			task->tss.vmem_table = (uint32_t) section_table_os;
			task->section_table = (unsigned char *) section_table_os;
			task->page_table = (unsigned char *) page_table_os;
			printf("task(%d) at 0x%08x\n", i, &task->tss);
			return task;
		}
	}
	return 0; /* もう全部使用中 */
}

void task_run(struct TASK *task, int level, int priority)
{
	if (level < 0) {
		level = task->level; /* レベルを変更しない */
	}
	if (priority > 0) {
		task->priority = priority;
	}

	if (task->flags == 2 && task->level != level) { /* 動作中のレベルの変更 */
		task_remove(task); /* これを実行するとflagsは1になるので下のifも実行される */
	}
	if (task->flags != 2) {
		/* スリープから起こされる場合 */
		task->level = level;
		task_add(task);
	}

	taskctl->lv_change = 1; /* 次回タスクスイッチのときにレベルを見直す */
	return;
}

void task_sleep(struct TASK *task)
{
	struct TASK *now_task;
	if (task->flags == 2) {
		/* 動作中だったら */
		now_task = task_now();
		task_remove(task); /* これを実行するとflagsは1になる */
		if (task == now_task) {
			/* 自分自身のスリープだったので、タスクスイッチが必要 */
			task_switchsub();
			now_task = task_now(); /* 設定後での、「現在のタスク」を教えてもらう */
			// farjmp(0, now_task->sel);
			//next_pcb = (uint32_t) &(now_task->tss); // RPi only
			_task_switch_asm((uint32_t) &(now_task->tss));
		}
	}
	return;
}

void task_switch(void)
{
	struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];
	struct TASK *new_task, *now_task = tl->tasks[tl->now];
	tl->now++;
	if (tl->now == tl->running) {
		tl->now = 0;
	}
	if (taskctl->lv_change != 0) {
		task_switchsub();
		tl = &taskctl->level[taskctl->now_lv];
	}
	new_task = tl->tasks[tl->now];
	timer_settime(task_timer, new_task->priority);
	if (new_task != now_task) {
		//farjmp(0, new_task->sel);
		next_pcb = (uint32_t) &(new_task->tss); // RPi only
	}
	return;
}
