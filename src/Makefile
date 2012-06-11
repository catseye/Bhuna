PROG=	bhuna
SRCS=	scan.c parse.c \
	symbol.c ast.c \
	mem.c \
	list.c atom.c buffer.c closure.c dict.c value.c \
	activation.c eval.c \
	builtin.c \
	main.c

CFLAGS+=-Wall -I/usr/local/include
.ifndef NODEBUG
CFLAGS+=-g -DDEBUG
.else
CFLAGS+=-DNDEBUG
.endif

NOMAN=	y

# DESTDIR=/usr/local/sbin
strip: bhuna
	strip bhuna
	ls -lah bhuna
	ls -la bhuna

.include <bsd.prog.mk>
