default:
	make all

all:
	make -C haribote
	make -C apilib
	make -C beepdown
	make -C color
	make -C color2
	make -C hello2
	make -C hello3
	make -C hello4
	make -C hello5
	make -C lines
	make -C noodle
	make -C star1
	make -C stars
	make -C stars2
	make -C walk
	make -C winhelo
	make -C winhelo2
	make -C winhelo3
	make -C typeipl
	make -C type
	make -C iroha
	make -C chklang
	make -C notrec
	make -C bball
	make -C invader
	make -C calc
	make -C tview
	make -C mmlplay
	cp nihongo/nihongo.fnt bin/
	cp euc.txt bin/
	cp mmldata/*.org bin/

clean:
	make -C haribote clean
	make -C apilib clean
	make -C beepdown clean
	make -C color clean
	make -C color2 clean
	make -C hello2 clean
	make -C hello3 clean
	make -C hello4 clean
	make -C hello5 clean
	make -C lines clean
	make -C noodle clean
	make -C star1 clean
	make -C stars clean
	make -C stars2 clean
	make -C walk clean
	make -C winhelo clean
	make -C winhelo2 clean
	make -C winhelo3 clean
	make -C typeipl clean
	make -C type clean
	make -C iroha clean
	make -C chklang clean
	make -C notrec clean
	make -C bball clean
	make -C invader clean
	make -C calc clean
	make -C tview clean
	make -C mmlplay clean
	rm	-f bin/*.*

