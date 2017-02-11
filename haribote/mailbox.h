#ifndef MAILBOX_H
#define MAILBOX_H

#include <stdint.h>
//#include <stdio.h>

#define MB_READ   0x2000B880
#define MB_STATUS 0x2000B898
#define MB_WRITE  0x2000B8A0

int mailbox_write(uint32_t value, uint32_t channel);
int mailbox_read(uint32_t channel, uint32_t *value);
int mailbox_getMemorySize();
int mailbox_getClockRate(int id);
int mailbox_emmc_clock(int id);

#endif
