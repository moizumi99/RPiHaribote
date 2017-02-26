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
	uint32_t bigSectorsPerFAT;
	uint32_t extFlag;
	uint32_t FS_version, rootDirStrtClus, FSInfoSec, bkUpBootSec;
	uint32_t driveNumber, extBootSignature, serialNumber, volumeLabel;
	char sig_0, sig_1;
	
	uint32_t *fatdata;

	//	mailbox_emmc_clock(1);
	
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
	//	printf("MBR\n");
	//	dump(carddata, 512);
	/* check the first partiion */
	//	bootDescriptor_0 = carddata[446];
	fileSystemDescriptor_0 = carddata[446+4];
	//	printf("File System Descriptor %02x\n", fileSystemDescriptor_0);
	if (fileSystemDescriptor_0 != 0x04 &&fileSystemDescriptor_0 != 0x06 && fileSystemDescriptor_0 != 0xb && fileSystemDescriptor_0 != 0x0c && fileSystemDescriptor_0 != 0x0e) {
		// 01: FAT12 (not supported)
		// 04: FAT16 (<32MB)
		// 05: Extended DOS (not supported)
		// 06: FAT16 (>=32MB)
		// 0b: FAT32 (>2GB)
		// 0c: FAT32 (Int 32h)
		// 0e: FAT16 (Int 32h)
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
	//	printf("BPB\n");
	//	dump(carddata, 512);
	bytePerSector = carddata[11] + (carddata[12]<<8);
	sectorsPerCluster = carddata[13];
	reservedSectors = carddata[14] + (carddata[15]<<8);
	numberOfFATs = carddata[16];
	rootEntries = carddata[17] + (carddata[18]<<8);
	sectorsPerFAT = carddata[22] + (carddata[23]<<8);
	bigTotalSectors = carddata[32] + (carddata[33]<<8) + (carddata[34]<<16) + (carddata[35]<<24);
	sig_0 = carddata[510];
	sig_1 = carddata[511];
	//	printf("bytePerSector: %08x\n", bytePerSector);
	//	printf("sectorsPerCluster: %08x\n", sectorsPerCluster);
	//	printf("reservedSectors: %08x\n", reservedSectors);
	//	printf("numberOfFATs: %08x\n", numberOfFATs);
	//	printf("sectorsPerFAT: %08x\n", sectorsPerFAT);
	//	printf("bigTotalSectors: %08x\n", bigTotalSectors);
	FAT_type = 0;
	if (sectorsPerFAT == 0) {
		FAT_type = 1; //FAT32
		// todo: check the number of clusters http://elm-chan.org/docs/fat.html#fsinfo
		// read FAT32 BPB
		bigSectorsPerFAT = carddata[36] + (carddata[37]<<8) + (carddata[38]<<16) + (carddata[39]<<24);
		sectorsPerFAT = bigSectorsPerFAT;
		extFlag = carddata[40] + (carddata[41]<<8);
		FS_version = carddata[42] + (carddata[43]<<8);
		rootDirStrtClus = carddata[44] + (carddata[45]<<8) + (carddata[46]<<16) + (carddata[47]<<24);
		FSInfoSec = carddata[48] + (carddata[49]<<8);
		bkUpBootSec = carddata[50] + (carddata[51]<<8);
		//		printf("bigSectorsPerFAT: %08x\n", bigSectorsPerFAT);
		//		printf("extFlag: %04x\n", extFlag);
		//		printf("FS Version: %04x\n", FS_version);
		//		printf("rootDirStrtClus: %08x\n", rootDirStrtClus);
		//		printf("FSInfoSec: %04x\n", FSInfoSec);
		//		printf("bkUpBootSec: %04x\n", bkUpBootSec);
	}
	if (bytePerSector!=512) {
		printf("Error: byte per sector is not 512\n");
		return;
	}
	if (numberOfFATs != 2) {
		printf("Warning: The number of FATs is not two but %d\n", numberOfFATs);
	}
	//	if (rootEntries != 512) {
	//		printf("Warning: The number of rootEntries is not 512 but %d\n", rootEntries);
	//	}
	if (sig_0 != 0x55 || sig_1 != 0xaa) {
		printf("Error: signature is not 55 AA but %02X%02X\n", sig_0, sig_1);
		return;
	}
	
	/* read fat */
	fatadr = (firstSectorNumbers_0 + reservedSectors)*512;
	fatsize = sectorsPerFAT;
	fat_bytesize = (FAT_type==0) ? fatsize*512*2 : fatsize*512;
	//	printf("fatadr: %08x, fatsize: %08x\n", (unsigned) fatadr, fatsize);
	fatdata = (uint32_t *) memman_alloc_4k(memman, fat_bytesize);
	file_readfat(memman, fatdata, fatadr);
	//	printf("FAT\n");
	//	dump((char *) fatdata, 512);
	/* read rde (Rood DIrection Entry) */
	rdeadr = fatadr + fatsize*numberOfFATs*bytePerSector;
	rdesize = 32*rootEntries/bytePerSector;
	usradr = rdeadr + rdesize*512;
	usrsize = bigTotalSectors - reservedSectors - numberOfFATs*sectorsPerFAT - rootEntries*32/bytePerSector;
	//	printf("rdeadr: %08x, rdesize: %08x\n", (unsigned) rdeadr, rdesize);
	//	printf("usradr: %08x, usrsize: %08x\n", (unsigned) usradr, usrsize);
	
	if (FAT_type==0) {
		rdedata = (char *) memman_alloc_4k(memman, 512*rdesize);
		if ((i = sdTransferBlocks(rdeadr, rdesize, rdedata, 0))!=0) {
			printf("Error: sdTransferBlocks (ROOT):%d\n", i);
			return;
		}
	} else {
		rdedata = (char *) memman_alloc_4k(memman, 512*32);
		file_loadfile(rootDirStrtClus, 32*512, rdedata, fatdata, usradr);
	}
	//	printf("RDE\n");
	//	dump(rdedata, 512*8);
	
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

/*    FAT32 BPB 
    struct FAT32BPB_t {
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
        // extension for FAT32
        Byte    bigSectorsPerFAT[4];    // 36, 37, 38, 39
        Byte    extFlags[2];            // 40, 41
        Byte    FS_Version[2];          // 42, 43
        Byte    rootDirStrtClus[4];     // 44, 45, 46, 47
        Byte    FSInfoSec[2];           // 48, 49
        Byte    bkUpBootSec[2];         // 50, 51
        Byte    reserved[12];           // 52-63
        Byte    driveNumber;            // 64
        Byte    unused;                 // 65
        Byte    extBootSignature;       // 66
        Byte    serialNumber[4];        // 67, 68, 69, 70
        Byte    volumeLabel[11];        // 71 - 80
        Byte    fileSystemType[8];      // 81 - 88
        Byte    loadProgramCode[420];   // 89 - 509
        Byte    sig[2];                 // 510, 512
    }    fat32;
}   BPBlock;
*/

void file_readfat(struct MEMMAN *memman, uint32_t *fat, long long img)
{
	uint32_t e;
	int resp;
	char *fat_data;
	uint16_t *fat16;
	uint32_t i;

	fat_data = (char *) memman_alloc_4k(memman, sectorsPerFAT*512);
	e = io_load_eflags();
	io_cli();
	//	printf("fatadr: %08x, fatsize: %08x\n", (unsigned) img, sectorsPerFAT);
	resp = sdTransferBlocks(img, sectorsPerFAT, fat_data, 0);
	io_store_eflags(e);
	if (resp!=0) {
		printf("Error: sdTransferBlocks (FAT): %d\n", resp);
		return;
	}
	if (FAT_type==0) {
		fat16 = (uint16_t *) fat_data;
		for(i=0; i<sectorsPerFAT*512/2; i++) {
			fat[i] = fat16[i];
		}
	} else {
		memCopy((char *)fat, fat_data, sectorsPerFAT*512);
	}
}

void file_loadfile(int clustno, int size, char *buf, uint32_t *fat, long long img)
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
		//		printf("current clust no: %04x\n", clustno);
		clustno = fat[clustno];
		//		printf("next clust no   : %04x\n", clustno);
		if (FAT_type==0 && clustno>=0xfff8) {
			return;
		} else if (clustno >= 0x0ffffff8) {
			return;
		}
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

		
