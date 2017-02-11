OBJS_API =	api001.obj api002.obj api003.obj api004.obj api005.obj api006.obj \
			api007.obj api008.obj api009.obj api010.obj api011.obj api012.obj \
			api013.obj api014.obj api015.obj api016.obj api017.obj api018.obj \
			api019.obj api020.obj api021.obj api022.obj api023.obj api024.obj \
			api025.obj api026.obj api027.obj \
			applib.o

CC = arm-none-eabi-gcc
CFLAG = -O0 -g -gdwarf-2 -march=armv6zk -mtune=arm1176jzf-s -nostdlib -nostartfiles -ffreestanding -c -fno-builtin
ASM = arm-none-eabi-gcc
AFLAG = -mcpu=arm1176jzf-s -fpic -ffreestanding -c
LD = arm-none-eabi-gcc
AR = arm-none-eabi-ar

ISA = ARM6v2

all: apilib.a

apilib.a: $(OBJS_API) Makefile
	-rm -f $@
	$(AR) rc $@ $(OBJS_API) 

%.obj: %.S Makefile
	$(ASM) -mcpu=arm1176jzf-s -nostdlib -fpie -c $< -o $@

%.o: %.c Makefile
	$(CC) $(CFLAG) $< -o $@

.PHONY:clean
clean:
	rm -f *.o
	rm -f *.obj
	rm -f *.S~
