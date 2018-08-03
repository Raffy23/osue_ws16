CC 	= gcc
CFLAGS 	= -std=c99 -pedantic -Wall -D_XOPEN_SOURCE=500 -g -D_BSD_SOURCE
LDFLAGS =

BINARY  = ispalindrom
OBJ     = ispalindrom.o

.PHONY: clean all

all: $(OBJ)
	gcc -o $(BINARY) $(OBJ)

clean:
	rm -f *.o *.a $(BINARY)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

run: all
	./$(BINARY)
