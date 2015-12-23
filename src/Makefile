CC = gcc
LD = gcc

CFLAGS = -D LINUX -g -c 
CFLAGS += -D_DEBUG -DDUMP_RAW
LDFLAGS = -Wl,-Map -Xlinker linkermap
LIBS = -lm
STRIP = strip --strip-unneeded --remove-section .comment

OBJS = Amstrad.o Acorn.o CBM.o main.o Sample.o T64.o TAP.o UEF.o WAV.o CSW.o TZX.o Spectrum.o

all:	ubercassette

clean:
	rm *.o
	rm ubercassette

ubercassette:  $(OBJS)
		$(LD) $(LDFLAGS) $^ $(LIBS) -o $@
		cp ubercassette ../

main.o: main.c UberCassette.h
Sample.o: Sample.c Sample.h WAV.h UberCassette.h

Acorn.o: Acorn.c Acorn.h UberCassette.h WAV.h Sample.h
CBM.o: CBM.c CBM.h UberCassette.h WAV.h Sample.h
Spectrum.o: Spectrum.c WAV.h UberCassette.h CSW.h TZX.h
Amstrad.o: Amstrad.c WAV.h UberCassette.h TZX.h Spectrum.h

T64.o: T64.c T64.h UberCassette.h CBM.h
TAP.o: TAP.c TAP.h UberCassette.h CBM.h
UEF.o: UEF.c UEF.h UberCassette.h WAV.h Acorn.h
WAV.o: WAV.c WAV.h UberCassette.h
CSW.o: CSW.c UberCassette.h CSW.h
TZX.o: TZX.c TZX.h UberCassette.h