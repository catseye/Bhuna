PROG=	bhuna
SRCS=	scan.c parse.c \
	symbol.c ast.c \
	mem.c \
	list.c atom.c buffer.c closure.c dict.c value.c \
	activation.c eval.c \
	gen.c vm.c \
	builtin.c \
	main.c

#	gen.c pool.c \
#CFLAGS+=-DPOOL_VALUES 
CFLAGS+=-O2
CFLAGS+=-DINLINE_BUILTINS
CFLAGS+=-Wall -I/usr/local/include
.ifndef NODEBUG
CFLAGS+=-g -DDEBUG
.else
CFLAGS+=-DNDEBUG
.endif

.ifdef PROFILED
CFLAGS+=-pg
.endif

NOMAN=	y

# DESTDIR=/usr/local/sbin
strip: bhuna
	strip bhuna
	ls -lah bhuna
	ls -la bhuna

.include <bsd.prog.mk>
