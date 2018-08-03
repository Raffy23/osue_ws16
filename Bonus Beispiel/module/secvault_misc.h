/**
 *  / ____|                          \ \    / /        | | |
 * | (___   ___  ___ _   _ _ __ ___   \ \  / /_ _ _   _| | |_
 *  \___ \ / _ \/ __| | | | '__/ _ \   \ \/ / _` | | | | | __|
 *  ____) |  __/ (__| |_| | | |  __/    \  / (_| | |_| | | |_
 * |_____/ \___|\___|\__,_|_|  \___|     \/ \__,_|\__,_|_|\__|
 *
 * This header file does contain all functions which are needed
 * somewhere in the module
 */

#ifndef SECVAULT_MISC
#define SECVAULT_MISC

#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/semaphore.h>

#include "secvault_inc.h"

/* === Structures === */

/*
 * In this structure the global sv_ctl device is stored
 */
extern struct sv_ctl_device sv_ctl;

/*
 * This structure is the head of the vault list
 */
extern struct vault_device_node vaults;

/*
 * A global lock for the vaults structure
 */
extern struct semaphore vault_lock;

/* === Functions === */

/*
 * This function mustn't be called before create_secvault_ctl_device!
 *
 * This function creates a secvault device with all the needed devices
 *  - vault in this structure all data is stored
 *  - fops a pointer to the file_operations structure which should be used
 *  - minor the minor number which should be used for the device
 *  - name a string which describes the name of the device
 * returns SUCCESS or any error code form creation
 */
extern int create_secvault_device(struct secvault_data *vault
				 , const struct file_operations *fops
				 , const int minor
				 , const char *name);

/*
 * Creates the sv_ctl device
 */
extern int create_secvault_ctl_device(void);

/*
 * Creates a new vault with the id and adds it the the vault list
 * returns SUCCESS or any other error code
 */
extern int create_new_vault(uint8_t id);

/*
 * gets the vault_device_node with the id form the vaults list,
 * if any error occurs the pointer might be an error value so it
 * must be checked with IS_ERR(...)
 */
extern struct vault_device_node *get_vault(uint8_t id);

/*
 * gets the vault_device_node wich does contain the cdev form the vaults list,
 * if any error occurs the pointer might be an error value so it
 * must be checked with IS_ERR(...)
 */
extern struct vault_device_node *get_vault_by_cdev(struct cdev *id);

/*
 * This method does clean up the data which is allocated by the sv_ctl device
 */
extern void clean_up_sv_ctl_device(void);

/*
 * This method dies clean up all the vaults
 */
extern void clean_up_sv_data(void);

/*
 * Helper method which converts the ioctl request number to a human readable
 * string
 */
extern const char *ioctl_number_to_string(unsigned long ioctl);

/*
 * validate access of the current user on the vault
 */
extern int validate_access(struct secvault_data *vault);

/*
 * Helper method to delete a node in the vaults list which the id,
 * returns SUCCESS or any other error code
 */
extern int delete_node(unsigned int id);

/*
 * Helper method which frees the data of a vault device, if force is
 * set to 1 the data will be freed even if the device is in use!
 */
extern int free_node_data(struct vault_device_node *node, int force);

#endif
