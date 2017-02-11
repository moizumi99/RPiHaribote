/*
  This program is based on the information on 
  https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
*/

#include "mailbox.h"
#include "mylib.h"

#define flushcache() asm volatile \
		("mcr p15, #0, %[zero], c7, c14, #0" : : [zero] "r" (0) )
/*
#define dmb() asm volatile \
		("mcr p15, #0, %[zero], c7, c10, #5" : : [zero] "r" (0) )

#define dsb() asm volatile \
		("mcr p15, #0, %[zero], c7, c10, #4" : : [zero] "r" (0) )
*/

int mailbox_write(uint32_t value, uint32_t channel) {
	volatile uint32_t sta;
	volatile uint32_t cmd;

	//	printf("Write start. Channel: %0d\n", channel);
	//	printf("Write Value: 0x%08x\n", value);
	if ((value & 0x0000000F) != 0) {
		return -1;
	}
	if ((channel & 0xFFFFFFF0) != 0) {
		return -1;
	}
	
	cmd = 0;
	do {
		//		dmb();
		sta = *( (volatile uint32_t *) MB_STATUS);
	} while ((sta & 0x80000000) != 0);
	//	printf("status: 0x%08x\n", sta);
	//	dmb();
	cmd = value | channel;
	*((volatile uint32_t *) MB_WRITE) = cmd;
	//	printf("Command: 0x%08x is written to mail box\n", cmd);
	
	return 0;
}

 
int mailbox_read(uint32_t channel, uint32_t *value) {
	uint32_t mail, sta;

	//	printf("Read Start. Channel: %d\n", channel);
	if ((channel & 0xFFFFFFF0) != 0) {
		printf("Channel %d is wrong\n", (int) channel);
		return -1;
	}
	do {
		do {
			//			dmb();
			sta = *( (volatile uint32_t *) MB_STATUS);
		} while (sta & 0x40000000);
		//		printf("status: 0x%08x\n", sta); 
		mail = *( (volatile uint32_t *) MB_READ);
		//		printf("mail: 0x%08x\n", mail);
	} while ((mail & 0x0000000F) != channel);

	//	dmb();
	*value =  mail & 0xFFFFFFF0;
	return 0;
}

int mailbox_getClockRate(int id)
{
	// id: 1: emmc, 2: uart, 3: arm, 4: core, 5: v3d
	//     6: h264, 7: ISP, 8: SDRAM, 9: PIXEL, a: PWM
	int i;
	uint32_t addr;
	uint32_t mail;
	uint32_t pt[8192] __attribute__((aligned(16)));
	int pt_index;

	for(i=0; i<8192; i++) {
		pt[i]=0;
	}
	pt[         0] = 12; // placeholder for the total size
	pt[         1] =  0; // Request code: process request
	pt_index=2;
	pt[pt_index++] = 0x00030002; // tag identifeir = 0x00030002: get clock rate)
	pt[pt_index++] = 8; // value buffer size in bytes ((1 entry)*4)
	pt[pt_index++] = 0; // 1bit request/response (0/1), 31bit value length in bytes
	pt[pt_index++] = 1; // id = 2
	pt[pt_index++] = 0; // return value 2
	pt[pt_index++] = 0; // stop tag
	pt[0] = pt_index*4;

	addr = ((unsigned int) pt) + 0x40000000;
	//	printf("mailbox addr: 0x%08x\n", (unsigned int) addr);
	//	for(i=0; i<*((uint32_t *) pt)/4; i++) {
	//		printf("%08x: %08x\n", (unsigned char *) (pt+i),  *((uint32_t *) (pt+i)));
	//	}
	//	printf("request to channel 8\n");
	mailbox_write(addr, 8);
	//	printf("read from channel 8\n");
	mailbox_read(8, &mail);
	//	printf("mailbox return value: 0x%08x\n", (unsigned int) mail);
	//	for(i=0; i<*((uint32_t *) mail)/4; i++) {
	//		printf("%08x: %08x\n", (unsigned char *) (mail+4*i),  *((uint32_t *) (mail+4*i)));
	//	}
	if (*((volatile unsigned int *) (mail+4)) == 0x80000000) {
		//		printf("Get clock rate (ID=%d) successful\n", *((volatile unsigned int *) (mail+5*4)));
		//		printf("Clock rate: %d\n", *((volatile unsigned int *) (mail+6*4)));
		return *((volatile unsigned int *) (mail+6*4));
	} else {
		printf("Error: ID=%d\n", *((volatile unsigned int *) (mail+5*4)));
		printf("response: 0x%08x\n", *((volatile unsigned int *) (mail+6*4)));
	}
	return -1;
}

int mailbox_getMemorySize()
{
	int i;
	uint32_t addr;
	uint32_t mail;
	uint32_t pt[8192] __attribute__((aligned(16)));
	int pt_index;

	for(i=0; i<8192; i++) {
		pt[i]=0;
	}
	pt[         0] = 12; // placeholder for the total size
	pt[         1] =  0; // Request code: process request
	pt_index=2;
	pt[pt_index++] = 0x00010005; // tag identifeir = 0x00030002: get clock rate)
	pt[pt_index++] = 8; // value buffer size in bytes ((1 entry)*4)
	pt[pt_index++] = 0; // 1bit request/response (0/1), 31bit value length in bytes
	pt[pt_index++] = 0; // return data (base address in bytes)
	pt[pt_index++] = 0; // return data (size in bytes)
	pt[pt_index++] = 0; // stop tag (unnecessary if initialized with zero. Just in case)
	pt[0] = pt_index*4;

	addr = ((unsigned int) pt) + 0x40000000;
	//	printf("mailbox addr: 0x%08x\n", (unsigned int) addr);
	//	for(i=0; i<*((uint32_t *) pt)/4; i++) {
	//		printf("%08x: %08x\n", (unsigned char *) (pt+i),  *((uint32_t *) (pt+i)));
	//	}
	//	printf("request to channel 8\n");
	mailbox_write(addr, 8);
	//	printf("read from channel 8\n");
	mailbox_read(8, &mail);
	//	dmb();
	//	printf("mailbox return value: 0x%08x\n", (unsigned int) mail);
	//	for(i=0; i<*((uint32_t *) mail)/4; i++) {
	//		printf("%08x: %08x\n", (unsigned char *) (mail+4*i),  *((uint32_t *) (mail+4*i)));
	//	}
	if (*((uint32_t *) (mail+20)) != 0) {
		printf("Error: ARM memory base address is not zero. %08x\n", *((uint32_t *) (mail+20)));
		//		_hangup();
	}
	if (*((uint32_t *) (mail+4)) == 0x80000000) {
		return *((uint32_t *) (mail+24));
	} else {
		return -1;
	}
}


int mailbox_emmc_clock(int id)
{
	int i;
	uint32_t addr;
	uint32_t mail;
	uint32_t pt[8192] __attribute__((aligned(16)));
	int pt_index;

	for(i=0; i<8192; i++) {
		pt[i]=0;
	}
	pt[         0] = 12; // placeholder for the total size
	pt[         1] =  0; // Request code: process request
	pt_index=2;
	pt[pt_index++] = 0x00038002; // tag identifeir = 0x00030002: get clock rate)
	pt[pt_index++] = 12; // value buffer size in bytes ((1 entry)*4)
	pt[pt_index++] = 0; // 1bit request/response (0/1), 31bit value length in bytes
	pt[pt_index++] = id; // clock id (1=emmc)
	pt[pt_index++] = 100000000; // clock =100 M MHz
	pt[pt_index++] = 0; // skip setting turbo
	pt[pt_index++] = 0; // stop tag (unnecessary if initialized with zero. Just in case)
	pt[0] = pt_index*4;

	addr = ((unsigned int) pt) + 0x40000000;
	//	printf("mailbox addr: 0x%08x\n", (unsigned int) addr);
	//	for(i=0; i<*((uint32_t *) pt)/4; i++) {
	//		printf("%08x: %08x\n", (unsigned char *) (pt+i),  *((uint32_t *) (pt+i)));
	//	}
	//	printf("request to channel 8\n");
	mailbox_write(addr, 8);
	//	printf("read from channel 8\n");
	mailbox_read(8, &mail);
	//	dmb();
	//	printf("mailbox return value: 0x%08x\n", (unsigned int) mail);
	//	for(i=0; i<*((uint32_t *) mail)/4; i++) {
	//		printf("%08x: %08x\n", (unsigned char *) (mail+4*i),  *((uint32_t *) (mail+4*i)));
	//	}
	if (*((uint32_t *) (mail+4)) == 0x80000000) {
		return 0;
	} else {
		return -1;
	}
}
