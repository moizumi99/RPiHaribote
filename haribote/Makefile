OBJS_BOOTPACK = boot.o bootpack.o linker.ld uart.o mylib.o mailbox.o graphic.o int.o fifo.o memory.o timer.o keyboard.o mouse.o sheet.o timer_rp.o mtask.o bridge.o sdcard.o file.o console.o window.o asm_func.o asm_data.o sound.o

CC = arm-none-eabi-gcc
CFLAG = -O0 -g -gdwarf-2 -march=armv6zk -mtune=arm1176jzf-s -nostdlib -nostartfiles -ffreestanding -c -fno-builtin
ASM = arm-none-eabi-gcc
AFLAG = -mcpu=arm1176jzf-s -fpic -ffreestanding -c
LD = arm-none-eabi-gcc
LIBRARIES = csud

ISA = ARM6v2

default:
	make all

all: kernel.img kernel.elf Makefile kernel.asm kernel.section.txt haribote.img

haribote.img: kernel.img
	cp kernel.img haribote.img

kernel.img: kernel.elf Makefile
	arm-none-eabi-objcopy kernel.elf -O binary kernel.img

kernel.elf: Makefile linker.ld $(OBJS_BOOTPACK)
	$(LD) -static -nostdlib -T linker.ld $(OBJS_BOOTPACK) -o kernel.elf -L /usr/lib/gcc/arm-none-eabi/4.9.3/ -lgcc -L. $(patsubst %,-l %,$(LIBRARIES)) -Wl,-Map,kernel.map

kernel.asm: kernel.elf
	arm-none-eabi-objdump -D kernel.elf > kernel.asm

kernel.section.txt: kernel.elf
	arm-none-eabi-readelf -S kernel.elf > kernel.section.txt

boot.o: boot.S hankaku.bin Makefile
	$(ASM) $(AFLAG) boot.S -o boot.o

%.o: %.c Makefile
	$(CC) $(CFLAG) $< -o $@

.PHONY:clean
clean:
	rm -f *.o
	rm -f *.elf
	rm -f *.img
	rm -f *.out
	rm -f *.asm
	rm -f *.map
	rm -f *.obj
	rm -f *.S~
	rm -f *.c~
	rm -f *.h~
	rm -f *.asm~
	rm -f *.ld~
	rm -f *.txt~
