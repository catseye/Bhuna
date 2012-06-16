CC?=gcc

#CFLAGS+=-DNO_AR_STACK
#CFLAGS+=-DPOOL_VALUES 
CFLAGS+=-DHASH_CONSING
CFLAGS+=-DINLINE_BUILTINS
CFLAGS+=-Wall -ansi

#CFLAGS+=-g -DDEBUG
CFLAGS+=-DNDEBUG
CFLAGS+=-Os -finline-functions

CFLAGS+=-Ilib

LIBS=-ldl

#CFLAGS+=-pg
NOPROFILE=yes

STATIC=
#STATIC=-static

#----------------------------------------

LIB=lib/libbhuna.a
BIN=bhuna

all: $(LIB) $(BIN)

strip: $(BIN)
	strip $(BIN)
	ls -lah $(BIN)

clean:
	rm -f lib/*.o driver/*.o $(LIB) $(BIN)

#----------------------------------------

LIBOBJS=	lib/report.o \
	lib/utf8.o lib/scan.o lib/parse.o \
	lib/symbol.o lib/ast.o \
	lib/type.o \
	lib/mem.o lib/pool.o lib/gc.o \
	lib/list.o lib/atom.o lib/buffer.o lib/closure.o lib/dict.o lib/value.o \
	lib/activation.o \
	lib/icode.o \
	lib/gen.o lib/vm.o \
	lib/process.o \
	lib/builtin.o \
	lib/trace.o

$(LIB): $(LIBOBJS)
	ar rc $(LIB) $(LIBOBJS)
	ranlib $(LIB)

#----------------------------------------

BINOBJS=driver/main.o

$(BIN): $(BINOBJS) $(LIB)
	$(CC) $(CFLAGS) $(BINOBJS) $(STATIC) -Llib -lbhuna $(LIBS) -o $(BIN)
