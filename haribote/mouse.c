#include "bootpack.h"
#include "mylib.h"

/* mouse */
typedef uint32_t u32;
typedef int Result;
typedef int16_t s16;
typedef uint8_t u8;

/* Keyboard.c with CSUD USB Driver */
int UsbInitialise(void);
void UsbCheckForChange();
u32 MouseCount();
Result MousePoll(u32 mouseAddress);
u32 MouseGetAddress(u32 index);
//u32 MouseGetPosition(u32 mouseAddress);
s16 MouseGetPositionX(u32 mouseAddress);
s16 MouseGetPositionY(u32 mouseAddress);
//s16 MouseGetWheel(u32 mouseAddress);
u8 MouseGetButtons(u32 mouseAddress);

void MouseUpdate();
int usbstatus=0; // 0: not enabled, 1: enabled
int MouseAddress=0;
int x_new=0, y_new=0, btn_new=0, x_old=0, y_old=0, btn_old=0;
struct FIFO32 *mousefifo;
int mousedata0;

void timer_mouse_handler(unsigned int hTimer, void *pParam, void *pContext)
{
	if (MouseAddress==0) {
		// 2 + 512 : mouse missing, need update
		fifo32_put(mousefifo, 2 + mousedata0);
		return;
	} else if (MousePoll(MouseAddress)!=0) {
		MouseAddress = 0;
		// 2 + 512 : mosue missing, need update
		fifo32_put(mousefifo, 2 + mousedata0);
		return;
	}
	x_new  = MouseGetPositionX(MouseAddress);
	y_new = MouseGetPositionY(MouseAddress);
	btn_new = MouseGetButtons(MouseAddress);
	if (x_new != x_old || y_new != y_old || btn_new != btn_old) {
		// 1 + 512 : mosue change
		fifo32_put(mousefifo, 1 + mousedata0);
	}
}

void enable_mouse(struct FIFO32 *fifo, int data0, struct MOUSE_DEC *mdec)
{
	int i;
	/* 書き込み先のFIFOバッファを記憶 */
	mousefifo = fifo;
	mousedata0 = data0;
	if (usbstatus==0) {
		if (UsbInitialise()!=0) {
			printf("Error: USB is not enabled\n");
		} else {
			usbstatus = 1;
		}
	}
	i=0;
	while(MouseAddress==0 && i++<100) {
		MouseUpdate();
	}
}

void MouseUpdate()
{
	UsbCheckForChange();
	if (MouseCount()>0) {
		MouseAddress = MouseGetAddress(0);
	}
}

// This is modified from Haribote
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat)
{
	if (dat==2) {
		// mouse missing
		MouseUpdate();
		return 0;
	}
	//record diff, adjust boundary crossing
	if (x_old<-16384 && x_new>16384) {
		// left end to right end
		mdec->x = (x_new-32768) - (x_old+32768);
	} else if (x_old>16384 && x_new<-16384) {
		// right end to right end
		mdec->x = (x_new+32768) - (x_old-32768);
	} else {
		mdec->x = x_new - x_old;
	}
	if (y_old<-16384 && y_new>16384) {
		// left end to right end
		mdec->y = (y_new-32768) - (y_old+32768);
	} else if (y_old>16384 && y_new<-16384) {
		// right end to right end
		mdec->y = (y_new+32768) - (y_old-32768);
	} else {
		mdec->y = y_new - y_old;
	}
	mdec->btn = btn_new;
	x_old = x_new;
	y_old = y_new;
	btn_old = btn_new;
	return 1;
}



