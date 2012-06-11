LIB=	bhuna

SRCS=	report.c \
	utf8.c scan.c parse.c \
	symbol.c ast.c \
	type.c \
	mem.c pool.c gc.c \
	list.c atom.c buffer.c closure.c dict.c value.c \
	activation.c \
	icode.c \
	gen.c vm.c \
	process.c \
	builtin.c \
	trace.c

NOMAN=	y

# DESTDIR=/usr/local/sbin
strip: libbhuna.so.0 libbhuna.a
	strip libbhuna.so.0
	ls -lah libbhuna.so.0

SHLIB_MAJOR=0
USELIBDIR=/usr/local/lib
USESHLIBDIR=/usr/local/lib

.include <bsd.lib.mk>
