CC=gcc

CFLAGS=-g -Wall

EXE=cf-sh
OBJECTS=main.o

all: $(EXE) clean

.SUFFIXES: .c .o
.c.o:
	$(CC) $(CFLAGS) -c $*.c

$(EXE): $(OBJECTS)
	$(CC) -g -o $(EXE) $(OBJECTS)

	
clean:
	rm -f *.o
