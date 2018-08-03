/**
 *  / ____|                          \ \    / /        | | |
 * | (___   ___  ___ _   _ _ __ ___   \ \  / /_ _ _   _| | |_
 *  \___ \ / _ \/ __| | | | '__/ _ \   \ \/ / _` | | | | | __|
 *  ____) |  __/ (__| |_| | | |  __/    \  / (_| | |_| | | |_
 * |_____/ \___|\___|\__,_|_|  \___|     \/ \__,_|\__,_|_|\__|
 *
 * This header contains the basic structures which are needed by
 * the kernel module
 */

#ifndef SECVAULT_MODULE
#define SECVAULT_MODULE

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/semaphore.h>
#include <linux/sched.h>
#include <linux/cred.h>

#define SUCCESS 0
#define VAULT_NAME "sv_data%d"
#define VAULT_CLASS KBUILD_MODNAME
#define NAME KBUILD_MODNAME ": "

/* ==== Type Definitions === */

/*
 * This structure represents a linked list node to store devices
 */
struct vault_device_node {
	int id;					//ID of the vault
	struct secvault_data *vault;		//Vault device struct
	struct vault_device_node *next;		//Next pointer
};

/*
 * This structure does store the actual data of the secure vaults and some
 * other informations about the device
 */
struct secvault_data {
	uint8_t *data;				//Encrypted data

	kuid_t user_id;				//UID for access validation
	unsigned int size;			//Size of the data block
	unsigned int keyLength;			//Size of the key
	char key[KEY_SIZE];			//Key -> should be dynamic

	struct semaphore lock;			//Lock for the data
	unsigned int usage;			//reference counter

	struct device *device;			//Kernel device
	struct cdev *cdev;			//Kernel char device
	struct class *class;			//Kernel device class
};

/*
 * This structure contains information about the sv_ctl device
 */
struct sv_ctl_device {
	struct device *device;			//Kernel device
	struct cdev *cdev;			//Kernel char device
	struct class *class;			//Kernel device class
};

/*
 * This structure holds provate data of each open request
 */
struct vault_priv_data {
	unsigned int pos;			//Position in file
	struct secvault_data *shared;		//Shared data
};

/*
 * Private data for the sv_ctl device
 */
struct ctl_priv_data {
	uint8_t target;				//Stores target device id
};

extern int debug_param;				//Debug parameter

#endif
