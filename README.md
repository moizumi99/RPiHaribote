# RPiHaribote
Raspberry Pi version of Haribote OS

What's this:
  This is Raspberry Pi porting of Haribote OS. This runs on Raspberry Pi B+ Rev 2.0.
  The link to original Haribote OS support page is here: http://hrb.osask.jp/
  Details are in Wiki https://github.com/moizumi99/RPiHaribote/wiki
  
How to run:
  - Copy your Raspberry Pi Raspbian OS boot directory to a new SD CARD. Only the FAT16 region that includes kernel.img need to be copied. Format needs to be FAT16.
  - Then, overwrite kernel.img in the new SDCARD with the one in /bin directory.
  - Copy "xxx.out" files to the same  directory. Copy "xxx.org" files as well.
  - Insert the SDCARD to your Raspberry PI B+, connect USB keryboard and mouse (please see the limitation section). And boot.

Keys
  - Shift + F2 : create a new console
  - tab: Switch console
  - F11: Move up the window

Commands
  - mem: displays memory size and free size
  - dir: shows root directory
  - start: start an application. wait for completion
  - ncst: start an application, without waiting for completion
  - langmode [0/1/2]: change language mode (0=English, 1=Japanse SJIS, 2=Japanese EUC)
  - exit: exit console
  - cls: clear console

Bundled applications
  - type <filename>: display a text file content
  - mmlplay <xxx.org>: play mml data
  - tview <filename>: text viewer (see limitation)
  - invader: a game which doesn't look too far from the old invader game
  - bball: Show s a graphic demo
  - calc <equation>: calculates equation
  - there are some other small demo or debugging applications
  
Limitation:
  - As CSUD is used for mouse and keyboard, this software only supports keyboard and mouse that is supported by CSUD. So far, I found wired keyboards and mice have high chance of being recognized
  - SDCARD.c is tested with limited nubmer of cards.
  - This version of Haribote has application compatibility with the original Haribote OS at c-code source level. The graphic viewer application that was bundled with the original Haribote is not included because of the assembler code.
  - The extraction of compressed data in the original Haribote OS is not implemented yet.
  - The support of ELF format used in the application is very rudimentary. Newly compiled application may not work. I am working on this.
  - Only basic functionality of FAT16/32 is supported. For example, only the root level is visible.
  - Only tested with my Raspberry PI B+ rev2.0 and Raspberry Pi Zero. It won't run on Pi2 or Pi3. Future extension is planned.
