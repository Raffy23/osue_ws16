CC 	= gcc
CFLAGS 	= -std=c99 -pedantic -Wall -D_XOPEN_SOURCE=500 -D_BSD_SOURCE -g
LDFLAGS = -pthread -lrt

BINARY_SERVER  = bankserver
OBJ_SERVER     = server.o
BINARY_CLIENT  = bankclient
OBJ_CLIENT     = client.o
OBJ_SHARED     = shared.o

.PHONY: clean all


all: $(OBJ_SERVER) $(OBJ_CLIENT)
	gcc -o $(BINARY_SERVER) $(LDFLAGS) $(OBJ_SERVER)
	gcc -o $(BINARY_CLIENT) $(LDFLAGS) $(OBJ_CLIENT)
	
clean:
	rm -f *.o *.a
	rm -f $(BINARY_SERVER)
	rm -f $(BINARY_CLIENT)
	echo "Clearing shared memory and lockfiles if they exist!"
	rm -f /dev/shm/sem.1526280*
	rm -f /dev/shm/1526280*
	
server: $(OBJ_SERVER)
	gcc -o $(BINARY_SERVER) $(LDFLAGS) $(OBJ_SERVER)

client: $(OBJ_CLIENT)
	gcc -o $(BINARY_CLIENT) $(LDFLAGS) $(OBJ_CLIENT)

%.o: %.c
	$(CC) $(CFLAGS) -c $<


