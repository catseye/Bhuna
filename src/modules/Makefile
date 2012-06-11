CFLAGS+=-I../lib

.include "../Makefile.inc"

all: io.so test

test.o: test.c
	gcc $(CFLAGS) -c test.c -o test.o

test: test.o
	gcc test.o -L../lib -lbhuna -o test

io.o: io.c
	gcc $(CFLAGS) -fpic -c io.c -o io.o

io.so: io.o
	gcc -shared -nostdlib io.o -L../lib -lbhuna -o io.so

clean:
	rm -f test *.o *.so

strip: io.so
	strip io.so
	ls -lah io.so
