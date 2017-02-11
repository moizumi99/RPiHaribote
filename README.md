# RPiHaribote
Raspberry Pi version of Haribote OS

What's this:
  This is Raspberry Pi porting of Haribote OS

What is Haribote?:
  Haribote is an OS for educational purpose released 
by Hidemi Kawai in 2006 for educational purpose for x86. The uniqueness 
of the OS was that it was released as a form of a book that explains the 
every line of the codes.

What can it do?
  It is an educational purpose OS, so it does very little. Still it comes with the followin functions
  - Provides basic GUI
  - Support input from mouse and keyboard
  - Provide basic console function with a few commands
  - Read text file
  - Run application in protected memory space
  - Provides pre-emptive multi-tasking environment
  - Displays Japanese test as well as English
  - Plays music with buzzer function
  - Provides some level of protection to OS
  - Provides basic APIs for applications, such as file read, graphic drawing, keyboard input, etc.
  - Comes with some applications such as text file viewer, MML player, an invader-like game, etc.

Where can you get the original Haribote OS?:
  You can download it from here
  https://book.mynavi.jp/supportsite/detail/4839919844.html
  The support web page is here
  http://oswiki.osask.jp/

What has been changed?
  - Basically, all assembler codes are written from scratch to adapt to ARM architecture.
  - Compiler tool-chain is moved to gcc, instead of Haribote's unique tool chain.
  - All the peripheral access has been rebuilt from scratch. Mouse/keyboard support is 
  realized by CSUD, which is take from https://github.com/Chadderz121/csud. Timer support 
  is written from scratch. Graphic initialization is written from scratch. Beep sound is 
  enabled using PWM at very low frequency.
  - Instead of the segmentation used in the original Haribote, paging is used to realize 
  - protection and multi-tasking
  - Multi-tasking related codes are all re-built from scratch
  - UART support and JTAG support is added
  - Files are read from SDCARD instead of floppy disk
  - FAT16 is supported instead of FAT12
  - Bootloader is removed.

Did you write everything?
  I tried to preserve as much part as possible from the original Haribote OS. So, large part of the c-code is unmodified from the original. (Actually, preserving the code was tougher than re-writing in some cases.)
  The USB device driver is take from another project https://github.com/Chadderz121/csud. SDCARD support is adapated from another project. Sound support has borrowed PWM access from https://github.com/Joeboy/pixperiments/tree/master/pitracker
