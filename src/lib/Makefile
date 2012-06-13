DLIB=	libbhuna.so.0
SLIB=   libbhuna.a

OBJS=	report.o \
	utf8.o scan.o parse.o \
	symbol.o ast.o \
	type.o \
	mem.o pool.o gc.o \
	list.o atom.o buffer.o closure.o dict.o value.o \
	activation.o \
	icode.o \
	gen.o vm.o \
	process.o \
	builtin.o \
	trace.o

all: $(DLIB)

$(DLIB): $(OBJS)
	$(CC) -o$(DLIB) $(OBJECTS)

clean:
	rm -f *.o *.so.0 *.a

