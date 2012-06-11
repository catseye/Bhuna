CFLAGS+=-I../lib

.include "../Makefile.inc"

all: bhuna

bhuna: main.o
	gcc $(CFLAGS) main.o $(STATIC) -L../lib -lbhuna -o bhuna

OBJS=	../lib/report.o \
	../lib/scan.o ../lib/parse.o \
	../lib/symbol.o ../lib/ast.o \
	../lib/type.o \
	../lib/mem.o ../lib/pool.o ../lib/gc.o \
	../lib/list.o ../lib/atom.o ../lib/buffer.o ../lib/closure.o ../lib/dict.o ../lib/value.o \
	../lib/activation.o \
	../lib/icode.o \
	../lib/gen.o ../lib/vm.o \
	../lib/process.o \
	../lib/builtin.o \
	../lib/trace.o \
	../lib/utf8.o

static: main.o $(OBJS)
	gcc $(CFLAGS) main.o $(OBJS) -o bhuna
	strip bhuna
	ls -lah bhuna

main.o: main.c
	gcc $(CFLAGS) -c main.c -o main.o

clean:
	rm -f bhuna *.o *.so

strip: bhuna
	strip bhuna
	ls -lah bhuna
