#-----------------------------------------------------------------------------

FLAGS      = -Wall -O2
LIBS       = -lm -lncurses
FILES = COPYING README makefile *.cc *.c *.h
VERSION = Apr2006

#-----------------------------------------------------------------------------

all:		mfsk_test mfsk_tx mfsk_symb mfsk_rx mfsk_trx rate_check peakrms addnoise addcarr

mfsk_symb:	mfsk_symb.cc struc.h minimize.h firgen.h
		g++ -o $@ $(FLAGS) mfsk_symb.cc $(LIBS)

mfsk_test:	mfsk_test.cc mfsk.h struc.h fht.h fft.h buffer.h cmpx.h gray.h noise.h
		g++ -o $@ $(FLAGS) mfsk_test.cc -lm

mfsk_tx:	mfsk_tx.cc mfsk.h sound.h rateconv.h lowpass3.h struc.h fht.h fft.h buffer.h cmpx.h gray.h noise.h stdinr.h
		g++ -o $@ $(FLAGS) mfsk_tx.cc $(LIBS)

mfsk_rx:	mfsk_rx.cc term.h mfsk.h sound.h struc.h fht.h fft.h buffer.h cmpx.h gray.h noise.h
		g++ -o $@ $(FLAGS) mfsk_rx.cc $(LIBS)

mfsk_trx:	mfsk_trx.cc term.h mfsk.h sound.h struc.h fht.h fft.h buffer.h cmpx.h gray.h noise.h
		g++ -o $@ $(FLAGS) mfsk_trx.cc $(LIBS)

rate_check:	rate_check.cc sound.h
		g++ -o $@ $(FLAGS) rate_check.cc -lm

peakrms:	peakrms.c
		gcc -o $@ $(FLAGS) peakrms.c $(LIBS)

addnoise:	addnoise.c
		gcc -o $@ $(FLAGS) addnoise.c $(LIBS)

addcarr:	addcarr.c
		gcc -o $@ $(FLAGS) addcarr.c $(LIBS)

#-----------------------------------------------------------------------------

release: $(FILES)
	test ! -d mfsk_olivia-$(VERSION) || rm -rf mfsk_olivia-$(VERSION)
	mkdir mfsk_olivia-$(VERSION)
	cp $(FILES) mfsk_olivia-$(VERSION)
	tar zcvvf mfsk_olivia-$(VERSION).tgz mfsk_olivia-$(VERSION)
	rm -rf mfsk_olivia-$(VERSION)

arch:
		tar -zcvf mfsk_olivia.tgz *.c* *.h* *.xmcd *.asc makefile

#-----------------------------------------------------------------------------

