#CFLAGS+=-DNO_AR_STACK
#CFLAGS+=-DPOOL_VALUES 
CFLAGS+=-DHASH_CONSING
CFLAGS+=-DINLINE_BUILTINS
CFLAGS+=-Wall

#-I/usr/local/include

.ifndef NODEBUG
CFLAGS+=-g -DDEBUG
.else
CFLAGS+=-DNDEBUG
.endif

.ifdef OPTIMIZED
CFLAGS+=-Os -finline-functions
.endif

.ifdef PROFILED
CFLAGS+=-pg
.else
NOPROFILE=yes
.endif

.ifdef STATIC
STATIC=-static
.else
STATIC=
.endif
