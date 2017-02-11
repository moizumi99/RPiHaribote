#include "bootpack.h"
extern void start_MMU(uint32_t base, uint32_t flag);

void init_gdtidt(void)
{
	// set up initial page
	//	uint32_t *section_table, *page_table; // moved to bootpack.h
	uint32_t i, j;
	uint8_t  ap, domain, c, b;
	uint32_t flag;

	section_table_os = (unsigned char *) 0x200000; // section table starts from 0x200000, 4*4K bytes
	page_table_os = (unsigned char *) 0x204000; // page table size = 512 * 256 * 4 byte

	generate_section_table(section_table_os);
	//	generate_page_table(section_table_adr, 0, 512, page_table_adr, 0x00000000, 0b01, 1, 1);
	// ap=01 for SVC only area, ap=11 for usr area
	flag = 1; // 1: MMU enabled, 0: MMU disabled
	start_MMU((uint32_t) section_table_os, flag);
}

void generate_section_table(unsigned char *section_table_adr)
{
	int i, ap, b, c;
	uint32_t *section_table;
	uint32_t physical_adr;
	// 4k x 4 bytes (16 kb) of memory must be secured for section_table
	// memory map:
	//   0-512M: SVC mode only (OS)
	//   512M-2048M: SVC (Peripheral, no cache)
	//   2048-: reserved for USR
	// ap=01 for SVC only area, ap=11 for usr area, ap = 0 for reserved area
	section_table = (uint32_t *) section_table_adr;
	for(i=0; i<4096; i++) {
		if (i<512) {
			ap = 0b01;
			c = 0;
			b = 0;
		} else if (i<2048) {
			ap = 0b01;
			c = 0;
			b = 0;
		} else {
			ap = 0b00;
			c = 0;
			b = 0;
		}
		physical_adr = i<<20; // physical address = virtual address
		section_table[i] = physical_adr | (ap<<10) | (c<<3) | (b<<2) | (0b00010); // 0b10 is for section table
	}
}

void generate_page_table(unsigned char *section_table_adr, int section_st, int sections, unsigned char *page_table_adr, unsigned char *physical_adr, int ap, int b, int c)
{
	// page_table must be able to hold 256 x sections x 4 bytes for page table
	// the memory area for pages must be secured prior to calling this function
	// ap=01 for SVC only area, ap=11 for usr area
	int i, j;
	uint32_t *section_table, *page_table;
	char *page_adr, *target_adr, *virtual_adr;

	// default: no mapping to physical memory
	section_table = (uint32_t *) section_table_adr;
	for(i = 0; i < sections; i++) {
		page_adr = page_table_adr + i*256*4; // each section (1M) has 256 pages (each page entry has 4 byte information)
		section_table[i+section_st] = ((uint32_t) page_adr) | 0b00001; // 0b01 is for small page table
		for(j=0; j<256; j++) {
		    target_adr    = (char *) (physical_adr + (i<<20) + (j<<12));
			page_table    = (uint32_t *) page_adr;
		    page_table[j] = ((uint32_t) target_adr) | (ap<<10) | (ap<<8) | (ap<<6) | (ap<<4) | (c<<3) | (b<<2) | 0b10;
		}
	}
	
}

int setpages(unsigned char *section_table_adr, unsigned char *va_st, unsigned char *pa_st, uint32_t size, char ap)
{
	uint32_t section;
	uint32_t vir_adr, phy_adr, page_adr;
	uint32_t i, sec_index;
	uint32_t page;
	uint32_t *section_table;
	section_table = (uint32_t *) section_table_adr;
	for(i = 0; i<(size+4095); i+=4096) {
		vir_adr = (uint32_t) (va_st + i); // each page is 4k bytes
		phy_adr = (uint32_t) (pa_st + i); // each page is 4k bytes
		sec_index = vir_adr >> 20;
		section = section_table[sec_index];
		if ((section & 0b11) != 0b01) {
			printf("section %08x is not for small page (08x)\n", sec_index, section);
			return -1;
		}
		page_adr = section & 0xFFFFFC00; // remove lower 10 bits
		page_adr += ((vir_adr>>12) & 0xFF)<<2; // 12th to 19th bits only
		page = *((uint32_t *) page_adr);
		if ((page & 0b11) != 0b10) {
			printf("page %08x is not for small page (08x)\n", page_adr, page);
			return -1;
		}
		page &= 0x0000000F; // ap3=ap2=ap1=ap0=0b11
		page |= (phy_adr & 0xFFFFF000);
		page |= ((ap<<10) | (ap<<8) | (ap<<6) | (ap<<4));
		*((uint32_t *) page_adr) = page;
	}
}


uint32_t v2p(uint32_t section_table_adr, uint32_t vadr)
{
	uint32_t *section_table, *page_table;
	uint32_t table_index, section_index, page_table_index, page_offset;
	uint32_t result;

	table_index = vadr >> 20;
	section_table = (uint32_t *) ((section_table_adr & 0xFFFFC000) | (table_index << 2));

	if (*section_table & 0b11 == 0b10) {
		section_index = vadr & 0x000fffff;
		result = (*section_table & 0xFFF00000) | section_index;
	} else if ((*section_table & 0b11) == 0b01) {
		page_table_index = (vadr >> 12) & 0xFF;
		page_table = (uint32_t *) (((*section_table) & 0xFFFFFC00) | (page_table_index<<2)) ;
		page_offset = vadr & 0xFFF;
		result = (*page_table & 0xFFFFF000) | page_offset;
	} else if ((*section_table & 0b11) == 0b11) {
		page_table_index = (vadr >> 10) & 0x3FF;
		page_table = (uint32_t *) (((*section_table) & 0xFFFFF000) | (page_table_index<<2)) ;
		page_offset = vadr & 0x3FF;
		result = (*page_table & 0xFFFFFC00) | page_offset;
	} else {
		result = 0;
	}
	return result;
}


// return 0. Only for compatibility with Haribote source code
int io_in8(int port)
{
	return 0;
}  

// do nothing. Only for compatibility with Haribote source code
int io_out8(int port, int data)
{
	return 0;
} 

/* void init_bss(void) */
/* { */
/* 	extern void *__bss_start; */
/* 	extern void *__bss_end; */

/* 	volatile unsigned int *p; */
/* 	unsigned int *start = (unsigned int *)&__bss_start; */
/* 	unsigned int *end = (unsigned int *)&__bss_end; */

/* 	for(p = start; p < end; p++) { */
/* 		*p = 0x00; */
/* 	} */
/* } */


// Original function for Raspberry Pi
int initializeFrameBuffer(int width, int height, int bitDepth, struct BOOTINFO *pbinfo)
{
	volatile char * fbInfo;
	uint32_t addr;
	uint32_t mail;
	
	fbInfo = (char *) &_FrameBufferInfo;	
	//	printf("FrameBufferInfo: %08x\n", (unsigned int) fbInfo);
	*((uint32_t *) (fbInfo   )) = width;
	*((uint32_t *) (fbInfo+4 )) = height;
	*((uint32_t *) (fbInfo+8 )) = width;
	*((uint32_t *) (fbInfo+12)) = height;
	*((uint32_t *) (fbInfo+20)) = bitDepth;
	*((uint32_t *) (fbInfo+24)) = 0;
	*((uint32_t *) (fbInfo+28)) = 0;

	addr = ((unsigned int) fbInfo) + 0x40000000;
	mailbox_write(addr, 1);
	mailbox_read(1, &mail);
	if (mail != 0) {
		/* Error */
		printf("Error! Mail is not 0 but 0x%08X\n", (unsigned int) mail);
		printf("Frame initialization error.\n");
		return -1;
	}
	printf("Frame initialization successful\n");
	pbinfo->scrnx = (short) (*((uint32_t *) (fbInfo   )));
	pbinfo->scrny = (short) (*((uint32_t *) (fbInfo+4 )));
	pbinfo->vram = (char *) (*((uint32_t *) (fbInfo+32)));
	printf("xsize: %4d, ysize: %4d\n", pbinfo->scrnx, pbinfo->scrny);
	printf("virtual width: %4d, ", (short) (*((uint32_t *) (fbInfo+8 ))));
	printf("virtual height: %4d\n", (short) (*((uint32_t *) (fbInfo+12))));
	printf("GPU - Pitch: %4d\n", (short) (*((uint32_t *) (fbInfo+16))));
	printf("Bit depth: %4d\n", (short) (*((uint32_t *) (fbInfo+20))));
	printf("X: %4d, ", (short) (*((uint32_t *) (fbInfo+24))));
	printf("Y: %4d\n", (short) (*((uint32_t *) (fbInfo+28))));
	printf("vram: %08x\n", (unsigned int) pbinfo->vram);
	printf("Size: %08x\n", (unsigned int) (*((uint32_t *) (fbInfo+36))));
    return (int) fbInfo;
}

inline void io_sti()
{
	asm volatile("cpsie i");
}

inline void io_cli()
{
	asm volatile("cpsid i");
}

