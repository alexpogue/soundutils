all: sndinfo sndcat sndchan sndmix

sndcat: sndcat.o fileUtils.o cs229Utils.o fileReader.o waveUtils.o errorPrinter.o
	gcc sndcat.o fileUtils.o cs229Utils.o fileReader.o waveUtils.o errorPrinter.o -o sndcat

sndinfo: sndinfo.o fileUtils.o fileReader.o errorPrinter.o waveUtils.o cs229Utils.o
	gcc sndinfo.o fileUtils.o fileReader.o errorPrinter.o waveUtils.o cs229Utils.o -o sndinfo

sndmix: sndmix.o fileUtils.o errorPrinter.o fileReader.o waveUtils.o cs229Utils.o
	gcc sndmix.o fileUtils.o errorPrinter.o fileReader.o waveUtils.o cs229Utils.o -o sndmix

sndchan: sndchan.o errorPrinter.o fileUtils.o fileReader.o waveUtils.o cs229Utils.o
	gcc sndchan.o errorPrinter.o fileUtils.o fileReader.o waveUtils.o cs229Utils.o -o sndchan

sndchan.o: sndchan.c errorPrinter.h 
	gcc -O3 -Wall -pedantic -c sndchan.c

sndcat.o: sndcat.c fileUtils.h
	gcc -O3 -Wall -pedantic -c sndcat.c

sndinfo.o: sndinfo.c fileUtils.h fileTypes.h readError.h errorPrinter.h
	gcc -O3 -Wall -pedantic -c sndinfo.c

sndmix.o: sndmix.c fileTypes.h fileUtils.h errorPrinter.h
	gcc -O3 -Wall -pedantic -c sndmix.c

fileUtils.o: fileUtils.c fileUtils.h fileReader.h fileTypes.h waveUtils.h readError.h cs229Utils.h writeError.h
	gcc -O3 -Wall -pedantic -c fileUtils.c

fileReader.o: fileReader.c fileReader.h readError.h
	gcc -O3 -Wall -pedantic -c fileReader.c

errorPrinter.o: errorPrinter.c errorPrinter.h
	gcc -O3 -Wall -pedantic -c errorPrinter.c

waveUtils.o: waveUtils.c waveUtils.h errorPrinter.h readError.h writeError.h
	gcc -O3 -Wall -pedantic -c waveUtils.c

cs229Utils.o: cs229Utils.c cs229Utils.h fileReader.h fileTypes.h readError.h writeError.h
	gcc -O3 -Wall -pedantic -c cs229Utils.c

clean:
	rm *.o

project.tar.gz: makefile cs229Utils.c cs229Utils.h errorPrinter.c errorPrinter.h fileReader.c fileReader.h fileTypes.h fileUtils.c fileUtils.h readError.h sndcat.c sndchan.c sndinfo.c sndmix.c waveUtils.c waveUtils.h writeError.h README
	tar -czf project.tar.gz makefile cs229Utils.c cs229Utils.h errorPrinter.c errorPrinter.h fileReader.c fileReader.h fileTypes.h fileUtils.c fileUtils.h readError.h sndcat.c sndchan.c sndinfo.c sndmix.c waveUtils.c waveUtils.h writeError.h README
