#
#  / ____|                          \ \    / /        | | |
# | (___   ___  ___ _   _ _ __ ___   \ \  / /_ _ _   _| | |_
#  \___ \ / _ \/ __| | | | '__/ _ \   \ \/ / _` | | | | | __|
#  ____) |  __/ (__| |_| | | |  __/    \  / (_| | |_| | | |_
# |_____/ \___|\___|\__,_|_|  \___|     \/ \__,_|\__,_|_|\__|
#

#
# Makefile for the Secure Vault kernel module, this file supports
# building inside the user-mode linux system and outside of it with
# a custom kernel location
#

KERNEL_DIR_HOST = "/opt/user-mode-linux/linux-4.8/linux-4.8/"
EXTRA_CFLAGS += -std=gnu99

ccflags-y += -DDEBUG

# Main .o file of the Kernel Module
obj-m += secvault.o

# All the .o file which are compiled into the secvault.o file
secvault-objs += secvault_fops.o secvault_misc.o secvault_ioctl.o secvault_main.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) ARCH=um V=1 modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) ARCH=um V=1 clean

# Building the Kernel Module on the host will result in an error
module-host:
	make -C $(KERNEL_DIR_HOST) M=$(PWD) ARCH=um V=1 modules

clean-host:
	make -C $(KERNEL_DIR_HOST) M=$(PWD) ARCH=um V=1 clean

#Coding Style checking: 
check:
	perl /lib/module/$(shell uname -r)/build/scripts/checkpatch.pl --no-tree --file *.h
	perl /lib/module/$(shell uname -r)/build/scripts/checkpatch.pl --no-tree --file *.c

check-host:
	perl $(KERNEL_DIR_HOST)/scripts/checkpatch.pl --no-tree --file *.h
	perl $(KERNEL_DIR_HOST)/scripts/checkpatch.pl --no-tree --file *.c

clean-code:
	perl /lib/module/$(shell uname -r)/build/scripts/cleanfile *.h
	perl /lib/module/$(shell uname -r)/build/scripts/cleanfile *.c

clean-code-host:
	perl $(KERNEL_DIR_HOST)/scripts/cleanfile *.h
	perl $(KERNEL_DIR_HOST)/scripts/cleanfile *.c
