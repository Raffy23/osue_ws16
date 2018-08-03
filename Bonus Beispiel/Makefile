#
#  / ____|                          \ \    / /        | | |  
# | (___   ___  ___ _   _ _ __ ___   \ \  / /_ _ _   _| | |_ 
#  \___ \ / _ \/ __| | | | '__/ _ \   \ \/ / _` | | | | | __|
#  ____) |  __/ (__| |_| | | |  __/    \  / (_| | |_| | | |_ 
# |_____/ \___|\___|\__,_|_|  \___|     \/ \__,_|\__,_|_|\__|
#

#
# Secure Vault makefile, this file can build the kernel module
# and the user space tools. 
# 


# User space stuff
USERSPACE_CC 		= gcc
USERSPACE_CFLAGS 	= -std=gnu99 -pedantic -Wall -D_XOPEN_SOURCE=500 -D_BSD_SOURCE -g
USERSPACE_LDFLAGS 	=

SVCTL_BINARY 		= svctl
SVCTL_OBJ 		= svctl.o

all: $(SVCTL_OBJ)
	cd ./module && make all
	$(USERSPACE_CC) -o $(SVCTL_BINARY) $(USERSPACE_LDFLAGS) $(SVCTL_OBJ)

all-host: $(SVCTL_OBJ)
	cd ./module && make module-host
	$(USERSPACE_CC) -o $(SVCTL_BINARY) $(USERSPACE_LDFLAGS) $(SVCTL_OBJ)

clean:
	cd ./module && make clean
	rm -f $(SVCTL_BINARY)

check:
	cd ./module && make check

check-host:
	cd ./module && make check-host

# Building the Kernel Module on the host will result in an error
module:
	cd ./module && make all

module-host:
	cd ./module && make module-host

clean-host:
	cd ./module && make clean-host
	rm -f *.o
	rm -f $(SVCTL_BINARY)

clean-code-host:
	cd ./module && make clean-code-host

clean-code:
	cd ./module && make clean-code

# Building Userspace tools like in previous Makefiles
svctl: $(SVCTL_OBJ)
	$(USERSPACE_CC) -o $(SVCTL_BINARY) $(USERSPACE_LDFLAGS) $(SVCTL_OBJ)

%.o: %.c
	$(USERSPACE_CC) $(USERSPACE_CFLAGS) -c $<
