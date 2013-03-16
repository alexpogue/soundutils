all: sndcat sndinfo

sndcat: sndcat.o fileUtils.o cs229Utils.o fileReader.o waveUtils.o
	gcc sndcat.o fileUtils.o cs229Utils.o fileReader.o waveUtils.o errorPrinter.o -o sndcat

sndinfo: sndinfo.o fileUtils.o fileReader.o errorPrinter.o waveUtils.o cs229Utils.o
	gcc sndinfo.o fileUtils.o fileReader.o errorPrinter.o waveUtils.o cs229Utils.o -o sndinfo

sndcat.o: sndcat.c fileUtils.h
	gcc -g -Wall -pedantic -c sndcat.c

sndinfo.o: sndinfo.c fileUtils.h fileTypes.h readError.h errorPrinter.h
	gcc -g -Wall -pedantic -c sndinfo.c

fileUtils.o: fileUtils.c fileUtils.h fileReader.h fileTypes.h waveUtils.h readError.h cs229Utils.h writeError.h
	gcc -g -Wall -pedantic -c fileUtils.c

fileReader.o: fileReader.c fileReader.h readError.h
	gcc -g -Wall -pedantic -c fileReader.c

errorPrinter.o: errorPrinter.c errorPrinter.h
	gcc -g -Wall -pedantic -c errorPrinter.c

waveUtils.o: waveUtils.c waveUtils.h errorPrinter.h readError.h writeError.h
	gcc -g -Wall -pedantic -c waveUtils.c

cs229Utils.o: cs229Utils.c cs229Utils.h fileReader.h fileTypes.h readError.h writeError.h
	gcc -g -Wall -pedantic -c cs229Utils.c

clean:
	rm *.o
