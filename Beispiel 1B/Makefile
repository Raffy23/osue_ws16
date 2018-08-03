CC 	= gcc
CFLAGS 	= -std=c99 -pedantic -Wall -D_XOPEN_SOURCE=500 -D_BSD_SOURCE -g
LDFLAGS = -DENDEBUG

BINARY_SERVER  = server
OBJ_SERVER     = server.o
BINARY_CLIENT  = client
OBJ_CLIENT     = client.o

.PHONY: clean all

all: $(OBJ_SERVER) $(OBJ_CLIENT)
	gcc -o $(BINARY_SERVER) $(OBJ_SERVER)
	gcc -o $(BINARY_CLIENT) $(OBJ_CLIENT)

clean:
	rm -f *.o *.a $(BINARY_SERVER) $(BINARY_CLIENT)

server: $(OBJ_SERVER)
	gcc -o $(BINARY_SERVER) $(OBJ_SERVER)

client: $(OBJ_CLIENT)
	gcc -o $(BINARY_CLIENT) $(OBJ_CLIENT)

%.o: %.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c $<


