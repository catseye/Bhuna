CFLAGS+=-I../lib

.include "../Makefile.inc"

all: bhuna

bhuna: main.o
	gcc main.o -L../lib -lbhuna -o bhuna

main.o: main.c
	gcc $(CFLAGS) -c main.c -o main.o

clean:
	rm -f bhuna *.o *.so

strip: bhuna
	strip bhuna
	ls -lah bhuna
