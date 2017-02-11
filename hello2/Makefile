APP = hello2
ASM = arm-none-eabi-gcc

default: ../bin/$(APP).out

../bin/$(APP).out: $(APP).out
	cp $(APP).out ../bin

$(APP).out: $(APP).S Makefile ../applinker.ld
	$(ASM) -mcpu=arm1176jzf-s -nostdlib -fpie -T ../applinker.ld $(APP).S -o $(APP).out
	arm-none-eabi-objdump -D $(APP).out > $(APP).asm

clean:
	rm -f *.out
	rm -f *.asm
	rm -f *.S~
