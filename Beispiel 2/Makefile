CC 	= gcc
CFLAGS 	= -std=c99 -pedantic -Wall -D_XOPEN_SOURCE=500 -D_BSD_SOURCE -g
LDFLAGS = -DENDEBUG

BINARY  = dsort
OBJ     = dsort.o

.PHONY: clean all


all: $(OBJ)
	gcc -o $(BINARY) $(OBJ)

clean:
	rm -f *.o *.a

dsort: $(OBJ)
	gcc -o $(BINARY) $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c $<


