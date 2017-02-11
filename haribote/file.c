#include "bootpack.h"
#include "mylib.h"
#include "sdcard.h"

char *rdedata;
long long fatadr;
int fatsize;
long long rdeadr;
int rdesize;
long long usradr;
int usrsize;
uint32_t sectorsPerCluster;
uint32_t sectorsPerFAT;

void init_filesystem(struct MEMMAN *memman)
{
	char carddata[512];
	uint32_t fileSystemDescriptor_0, firstSectorNumbers_0, numberOfSectors_0;
	int i, j, fatsize;
	uint32_t bytePerSector;
	uint32_t reservedSectors;
	uint32_t numberOfFATs;
	uint32_t rootEntries;
	uint32_t bigTotalSectors;

	mailbox_emmc_clock(1);
	
	sdInit();
	waitMicro(100);
	i = sdInitCard();
	waitMicro(10000);
	if (i != 0) {
		printf("ERROR ------------SDCAR initialization error------------------\n", i);
	}
	waitMicro(10000);
	/* Read MBR */
	if ((i=sdTransferBlocks((long long) 0, 1, carddata, 0)) != 0) {
		printf("Error: sdTransferBlocks: %d\n", i);
		return;
	}
	/* check the first partiion */
	//	bootDescriptor_0 = carddata[446];
	fileSystemDescriptor_0 = carddata[446+4];
	if (fileSystemDescriptor_0 != 0x06 && fileSystemDescriptor_0 != 0xb && fileSystemDescriptor_0 != 0x0c && fileSystemDescriptor_0 != 0x0e) {
		printf("Error. Only support FAT\n");
		return;
	}
	firstSectorNumbers_0 = carddata[446+8] + (carddata[446+9]<<8) + (carddata[446+10]<<16) + (carddata[446+11]<<24);
	numberOfSectors_0 = carddata[446+12] + (carddata[446+13]<<8) + (carddata[446+14]<<16) + (carddata[446+15]<<24);
	//	printf("first Sector Number: %08x\n", firstSectorNumbers_0);
	
	/* Read the BPB of the first partition */
	if ((i=sdTransferBlocks((long long) firstSectorNumbers_0*512, 1, carddata, 0)) != 0) {
		printf("Error: sdTransferBlocks (BPB): %d\n", i);
		return;
	}
	bytePerSector = carddata[11] + (carddata[12]<<8);
	sectorsPerCluster = carddata[13];
	reservedSectors = carddata[14] + (carddata[15]<<8);
	numberOfFATs = carddata[16];
	rootEntries = carddata[17] + (carddata[18]<<8);
	sectorsPerFAT = carddata[22] + (carddata[23]<<8);
	bigTotalSectors = carddata[32] + (carddata[33]<<8) + (carddata[34]<<16) + (carddata[35]<<24);
	/* printf("bytePerSector: %08x\n", bytePerSector); */
	/* printf("sectorsPerCluster: %08x\n", sectorsPerCluster); */
	/* printf("reservedSectors: %08x\n", reservedSectors); */
	/* printf("numberOfFATs: %08x\n", numberOfFATs); */
	/* printf("sectorsPerFAT: %08x\n", sectorsPerFAT); */
	/* printf("bigTotalSectors: %08x\n", bigTotalSectors); */
	if (bytePerSector!=512) {
		printf("Error: byte per sector is not 512\n");
		return;
	}
	if (numberOfFATs != 2) {
		printf("Warning: The number of FATs is not two but %d\n", numberOfFATs);
	}
	if (rootEntries != 512) {
		printf("Warning: The number of rootEntries is not 512 but %d\n", rootEntries);
	}
	
	/* read fat */
	fatadr = (firstSectorNumbers_0 + reservedSectors)*512;
	fatsize = sectorsPerFAT;
	//	printf("fatadr: %08x, fatsize: %08x\n", (unsigned) fatadr, fatsize);
	//	fatdata = (char *) memman_alloc_4k(memman, fatsize*512);
	//	if ((i = sdTransferBlocks(fatadr, fatsize, fatdata, 0))!=0) {
	//		printf("Error: sdTransferBlocks (FAT): %d\n", i);
	//		return;
	//	}
	/* read rde (Rood DIrection Entry) */
	rdeadr = fatadr + fatsize*numberOfFATs*512;
	rdesize = 32*rootEntries/512;
	//	printf("rdeadr: %08x, rdesize: %08x\n", (unsigned) rdeadr, rdesize);
	rdedata = (char *) memman_alloc_4k(memman, 512*rdesize);
	if ((i = sdTransferBlocks(rdeadr, rdesize, rdedata, 0))!=0) {
		printf("Error: sdTransferBlocks (ROOT):%d\n", i);
		return;
	}
	//	dump(rdedata, 512);
	
	usradr = rdeadr + rdesize*512;
	usrsize = bigTotalSectors - reservedSectors - numberOfFATs*sectorsPerFAT - rootEntries*32/512;
	//	printf("usradr: %08x, usrsize: %08x\n", (unsigned) usradr, usrsize);
}

/*
typedef struct MasterBootRecode {
    char    checkRoutionOnx86[446];
    struct {
        char    bootDescriptor;
        char    firstPartitionSector[3];
        char    fileSystemDescriptor;
        char    lastPartitionSector[3];
        char    firstSectorNumbers[4];
        char    numberOfSectors[4];
    }   partitionTable[4];
    char    sig[2];
}   MBRecode;
*/


/*
    struct FAT16BPB_t {
        Byte    jmpOpeCode[3];          // 00, 01, 02
        Byte    OEMName[8];             // 03, 04, 05, 06, 07, 08, 09, 10
        Byte    bytesPerSector[2];      // 11, 12
        Byte    sectorsPerCluster;      // 13
        Byte    reservedSectors[2];     // 14, 15
        Byte    numberOfFATs;           // 16
        Byte    rootEntries[2];         // 17, 18
        Byte    totalSectors[2];        // 19, 20
        Byte    mediaDescriptor;        // 21
        Byte    sectorsPerFAT[2];       // 22, 23
        Byte    sectorsPerTrack[2];     // 24, 25
        Byte    heads[2];               // 26, 27
        Byte    hiddenSectors[4];       // 28, 29, 30, 31
        Byte    bigTotalSectors[4];     // 32, 33, 34, 35
        Byte    driveNumber;            // 36
        Byte    unused;                 // 37
        Byte    extBootSignature;       // 38
        Byte    serialNumber[4];        // 39, 40, 41, 42
        Byte    volumeLabel[11];        // 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53
        Byte    fileSystemType[8];      // 54, 55, 56, 57, 58, 59, 60, 61
        Byte    loadProgramCode[448];   // 62 to 509
        Byte    sig[2];                 // 510, 511
    }    fat16;
*/


void file_readfat(uint16_t *fat, long long img)
{
	uint32_t e;
	int resp;
	e = io_load_eflags();
	io_cli();
	printf("fatadr: %08x, fatsize: %08x\n", (unsigned) img, sectorsPerFAT);
	resp = sdTransferBlocks(img, sectorsPerFAT, (char *) fat, 0);
	io_store_eflags(e);
	if (resp!=0) {
		printf("Error: sdTransferBlocks (FAT): %d\n", resp);
		return;
	}
}

void file_loadfile(int clustno, int size, char *buf, uint16_t *fat, long long img)
{
	int resp;
	long long adr;
	uint32_t e;

	for(;;) {
		adr = usradr + (clustno-2)*sectorsPerCluster*512;
		if (size <= 512*sectorsPerCluster) {
			e = io_load_eflags();
			io_cli();
			resp=sdTransferBlocks(adr, (size+511)/512, buf, 0);
			io_store_eflags(e);
			if (resp!=0) {
				printf("Error: sdTransferBlocks (load file 1): %d\n", resp);
				return;
			}
			break;
		}
		// read first cluster
		e = io_load_eflags();
		io_cli();
		resp=sdTransferBlocks(adr, sectorsPerCluster, buf, 0);
		io_store_eflags(e);
		if (resp!=0) {
			printf("Error: sdTransferBlocks (load file 2): %d\n", resp);
			return;
		}
		size -= 512*sectorsPerCluster;
		buf += 512*sectorsPerCluster;
		printf("current clust no: %04x\n", clustno);
		clustno = fat[clustno];
		printf("next clust no   : %04x\n", clustno);
	}
	return;
}

struct FILEINFO *file_search(char *name, struct FILEINFO *finfo, int max)
{
	int i, j;
	char s[12];
	for (j = 0; j < 11; j++) {
		s[j] = ' ';
	}
	j = 0;
	for (i = 0; name[i] != 0; i++) {
		if (j >= 11) { return 0; /* 見つからなかった */ }
		if (name[i] == '.' && j <= 8) {
			j = 8;
		} else {
			s[j] = name[i];
			if ('a' <= s[j] && s[j] <= 'z') {
				/* 小文字は大文字に直す */
				s[j] -= 0x20;
			} 
			j++;
		}
	}
	for (i = 0; i < max; ) {
		if (finfo[i].name[0] == 0x00) {
			break;
		}
		if ((finfo[i].type & 0x18) == 0) {
			for (j = 0; j < 8; j++) {
				if (finfo[i].name[j] != s[j]) {
					goto next;
				}
			}
			for (j=0; j<3; j++) {
				if (finfo[i].ext[j] != s[8+j]) {
					goto next;
				}
			}
			return finfo + i; /* ファイルが見つかった */
		}
next:
		i++;
	}
	return 0; /* 見つからなかった */
}

		
