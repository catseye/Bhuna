PROG=	bhuna
SRCS=	report.c \
	scan.c parse.c \
	symbol.c ast.c \
	type.c \
	mem.c pool.c \
	list.c atom.c buffer.c closure.c dict.c value.c \
	activation.c eval.c \
	gen.c vm.c \
	builtin.c \
	main.c

CFLAGS+=-DPOOL_VALUES 
CFLAGS+=-DREFCOUNTING_MACROS
CFLAGS+=-DINLINE_BUILTINS
CFLAGS+=-Wall -I/usr/local/include
.ifndef NODEBUG
CFLAGS+=-g -DDEBUG
.else
CFLAGS+=-DNDEBUG
.endif

.ifdef OPTIMIZED
CFLAGS+=-O2 -finline-functions
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
