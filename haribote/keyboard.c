#include "bootpack.h"
#include "mylib.h"

/* CSUD USB Driver */
typedef uint32_t u32;
typedef uint16_t u16;
/* #ifndef __cplusplus  */
/* typedef enum {  */
/* 	false = 0,  */
/*  	true = 1,  */
/* 	} bool;  */
/* #endif  */

struct KeyboardModifiers {
	bool LeftControl : 1; // @0
	bool LeftShift : 1; // @1
	bool LeftAlt : 1; // @2
	bool LeftGui : 1; // the 'windows' key @3
	bool RightControl : 1;  // @4
	bool RightShift : 1; // @5
	bool RightAlt : 1; // 'alt gr' @6
	bool RightGui : 1; // @7
} __attribute__ ((__packed__));

int UsbInitialise(void);
void UsbCheckForChange(void);
u32 KeyboardCount();
u32 KeyboardGetAddress(u32 index);
u32 KeyboardGetKeyDownCount(u32 keyboardAddress);
u16 KeyboardGetKeyDown(u32 keyboardAddress, u32 index);
int KeyboardPoll(u32 keyboardAddress);
struct KeyboardModifiers KeyboardGetModifiers(u32 keyboardAddress);

static char KeysNormal[] = {
	0x0, 0x0, 0x0, 0x0, 'a', 'b', 'c', 'd',
	'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',
	'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
	'u', 'v', 'w', 'x', 'y', 'z', '1', '2',
	'3', '4', '5', '6', '7', '8', '9', '0',
	'\n', 0x0, '\b', '\t', ' ', '-', '=', '[',
	']', '\\', 0x0, ';',  39, '`', ',', '.',
	'/', 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, '/', '*', '-', '+',
	'\n', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', '0', '.', 0x0, 0x0, 0x0, '='};

static char KeysShift[] = {
	0x0, 0x0, 0x0, 0x0, 'A', 'B', 'C', 'D',
	'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
	'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
	'U', 'V', 'W', 'X', 'Y', 'Z', '!', '@',
	'#', '$', '%', '^', '&', '*', '(', ')',
	'\n', 0x0, '\b', '\t', ' ', '_', '+', '{',
	'}', '|', 0x0, ':', '~',  34, '<', '>',
	'?', 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, '/', '*', '-', '+',
	'\n', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', '0', '.', 0x0, 0x0, 0x0, '='};

// shared with mouse.c
extern int usbstatus; // 0: not enabled, 1: enabled

// global variables for keyboard.c
unsigned int KeyboardAddress;
unsigned int KeyboardOldDown[6];
int KeyboardGetChar();

struct FIFO32 *keyfifo;
int keydata0;
extern char message[];

void init_keyboard(struct FIFO32 *fifo, int data0)
{
	keyfifo = fifo;
	keydata0 = data0;
	if (usbstatus==0) {
		if (UsbInitialise()!=0) {
			printf("Error: USB is not enabled\n");
		} else {
			usbstatus = 1;
		}
	}
	KeyboardUpdate();
}

void KeyboardUpdate()
{
	int i;
	
	i=0;
	while(KeyboardAddress==0 && i++<100) {
		UsbCheckForChange();
		if (KeyboardCount()>0) {
			KeyboardAddress = KeyboardGetAddress(0);
		}
		if (KeyboardPoll(KeyboardAddress)!=0) {
			KeyboardAddress = 0;
		}
	}
}

void timer_kbd_handler(unsigned int hTimer, void *pParam, void *pContext)
{
	int c, j;

	if (KeyboardAddress==0) {
		fifo32_put(keyfifo, 0 + keydata0); // 0 (NULL) is for keyboard missing
		return;
	}
	for(j=0; j<6; j++) { // check for up-to 6 keys
		KeyboardOldDown[j] = KeyboardGetKeyDown(KeyboardAddress, j);
	}
	if (KeyboardPoll(KeyboardAddress)!=0) {
		fifo32_put(keyfifo, 0 + keydata0); // 0 (NULL)  is keyboard missing
		return;
	}
	if ((c = KeyboardGetChar())>=0) {
		fifo32_put(keyfifo, c+keydata0);
		//		sprintf(message, "key: %d\n", c);
		//		printf("In int handler: %s", message);
   }
}

bool KeyWasDown(u32 scanCode)
{
	int i;
	for(i=0; i<6; i++) {
		if (KeyboardOldDown[i] == scanCode) {
			return true;
		}
	}
	return false;
}

int KeyboardGetChar()
{
	unsigned int key;
	struct KeyboardModifiers KM;
	char *s;
	int j;
	int kc;

	if ((kc = KeyboardGetKeyDownCount(KeyboardAddress))==0) {
		return -1;
	}
	kc = (kc>6) ? 6 : kc;
	for(j=0; j<kc; j++) {
		if ((key = KeyboardGetKeyDown(KeyboardAddress, j))==0) {
			// no more key
			break;
		} else {
			// check if it was pressed before, and key is <104
			if (!KeyWasDown(key)) {
				KM = KeyboardGetModifiers(KeyboardAddress);
				if (key==58 && (KM.LeftShift | KM.RightShift)) {
					return 254; // send special command (kill)
				} else if (key == 68) {
					return 253; // send special command (switch window)
				} else if (key==59 && (KM.LeftShift | KM.RightShift)) {
					return 252; // send special command (open console)
				} else if (key<104) {
					if (KM.LeftShift | KM.RightShift) {
						s = KeysShift;
					} else {
						s = KeysNormal;
					}
					if (s[key] !=0) {
						return s[key];
					}
				}
			}
		}
	}
	// no appropriate key found
	return -1;
}

