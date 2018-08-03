/**
 *  / ____|                          \ \    / /        | | |
 * | (___   ___  ___ _   _ _ __ ___   \ \  / /_ _ _   _| | |_
 *  \___ \ / _ \/ __| | | | '__/ _ \   \ \/ / _` | | | | | __|
 *  ____) |  __/ (__| |_| | | |  __/    \  / (_| | |_| | | |_
 * |_____/ \___|\___|\__,_|_|  \___|     \/ \__,_|\__,_|_|\__|
 *
 * This file contains some functions which can create and delete
 * vaults or search them in the internal linked list
 */

/* This is not the main file of the kernel module */
#define __NO_VERSION__

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/semaphore.h>
#include <asm/uaccess.h>

#include "../secvault.h"
#include "secvault_misc.h"
#include "secvault_fops.h"

/* === global structures === */

struct semaphore vault_lock;
struct sv_ctl_device sv_ctl;
struct vault_device_node vaults = {
	.id = -1,
	.vault = NULL,
	.next = NULL,
};

int create_secvault_ctl_device(void)
{
	int ret = 0;

	sv_ctl.cdev = cdev_alloc();
	if (unlikely(sv_ctl.cdev == NULL))
		return -ENOMEM;

	/* Initalize char device */
	cdev_init(sv_ctl.cdev, &secctl_fops);

	sv_ctl.class = class_create(THIS_MODULE, VAULT_CLASS);
	if (unlikely(IS_ERR(sv_ctl.class))) {
		pr_alert(NAME "Failed to create class, error code: %ld!\n"
		       , PTR_ERR(sv_ctl.class));

		cdev_del(sv_ctl.cdev);
		return PTR_ERR(sv_ctl.class);
	}

	/* Create device */
	sv_ctl.device = device_create(sv_ctl.class
					, NULL
					, MKDEV(MAJOR_DEV_NUM, 253)
					, NULL
					, "sv_ctl");

	if (unlikely(IS_ERR(sv_ctl.device))) {
		pr_alert(NAME "Failed to create device, error code: %ld!\n"
			, PTR_ERR(sv_ctl.device));

		class_destroy(sv_ctl.class);
		cdev_del(sv_ctl.cdev);
		return PTR_ERR(sv_ctl.device);
	}

	ret = cdev_add(sv_ctl.cdev, MKDEV(MAJOR_DEV_NUM, 253), 1);
	if (unlikely(ret < 0)) {
		dev_alert(sv_ctl.device
			, "Failed to add cdev, error code: %d!\n"
			, ret);

		device_del(sv_ctl.device);
		class_destroy(sv_ctl.class);
		cdev_del(sv_ctl.cdev);
		return ret;
	}

	if (unlikely(debug_param != 0))
		dev_info(sv_ctl.device, "Device created successful\n");

	return SUCCESS;
}

int create_secvault_device(struct secvault_data *vault
				 , const struct file_operations *fops
				 , const int minor
				 , const char *name)
{
	int ret = SUCCESS;

	/* we can't init any device if sv_ctl was not initalized yet */
	BUG_ON(sv_ctl.class == NULL);

	/* Allocate char device */
	vault->cdev = cdev_alloc();
	if (unlikely(vault->cdev == NULL)) {
		pr_alert(NAME "Unable to register %s device!\n"
		       , DEV_SECVAULT_CTL);

		return -ENOMEM;
	}

	/* Initalize char device */
	cdev_init(vault->cdev, fops);

	/* Create device */
	vault->device = device_create(sv_ctl.class
					, NULL
					, MKDEV(MAJOR_DEV_NUM, minor)
					, NULL
					, name
					, minor);

	if (unlikely(IS_ERR(vault->device))) {
		pr_alert(NAME
			"Failed to create device, error code: %ld!\n"
			, PTR_ERR(vault->device));

		cdev_del(vault->cdev);
		return PTR_ERR(vault->device);
	}

	/* init semaphore */
	sema_init(&vault->lock, 1);

	/* Register char device in /dev (userspace might want to use it now) */
	ret = cdev_add(vault->cdev, MKDEV(MAJOR_DEV_NUM, minor), 1);
	if (unlikely(ret < 0)) {
		dev_alert(vault->device
			, "Failed to add cdev, error code: %d!\n"
			, ret);

		device_del(vault->device);
		cdev_del(vault->cdev);
		return ret;
	}

	if (unlikely(debug_param != 0))
		dev_info(vault->device, "Device created successful\n");

	return SUCCESS;
}

int create_new_vault(uint8_t id)
{
	struct vault_device_node *cur = &vaults;
	struct vault_device_node *new_node = NULL;
	int vret = 0;

	if (unlikely(debug_param != 0))
		pr_info(NAME "create_new_vault(%d)\n", id);

	if (down_interruptible(&vault_lock) != 0) {
		up(&vault_lock);
		return -EINTR;
	}

	/* Find last vault in List */
	while (cur->next != NULL)
		cur = cur->next;

	/* Allocate Mempory for new vault node */
	new_node = kzalloc(sizeof(struct vault_device_node),  GFP_KERNEL);
	new_node->vault = kzalloc(sizeof(struct secvault_data),  GFP_KERNEL);

	if (unlikely(new_node == NULL)) {
		up(&vault_lock);
		return -ENOMEM;
	}

	if (unlikely(new_node->vault == NULL)) {
		up(&vault_lock);

		kfree(new_node);
		return -ENOMEM;
	}

	/* Increment device counter and create the char device */
	new_node->id = id;
	vret = create_secvault_device(new_node->vault
					, &secvault_fops
					, new_node->id
					, VAULT_NAME);

	/* Check outcome of device creation */
	if (likely(vret == SUCCESS)) {
		/* Set owner UID to the vault */
		new_node->vault->user_id = get_current_user()->uid;

		/* Save vault as first device or append */
		if (unlikely(vaults.vault == NULL)) {
			vaults.vault = new_node->vault;
			vaults.id    = new_node->id;
			kfree(new_node);
		} else {
			cur->next = new_node;
		}
	} else {
		/* free memory of allocated node because of error */
		kfree(new_node->vault);
		kfree(new_node);
	}

	up(&vault_lock);
	return vret;
}

struct vault_device_node *get_vault(uint8_t id)
{
	struct vault_device_node *cur = &vaults;

	if (unlikely(debug_param != 0))
		pr_info(NAME "get_vault(%d)\n", id);

	if (unlikely(down_interruptible(&vault_lock) != 0)) {
		up(&vault_lock);
		return ERR_PTR(-EINTR);
	}

	/* root element is not set */
	if (cur->id == -1) {
		up(&vault_lock);
		return NULL;
	}

	/* Search for the vault with the id */
	while (cur != NULL && cur->id != id) {
		if (debug_param != 0)
			pr_info(NAME "get_vault(%d) cur->%u\n", id, cur->id);

		cur = cur->next;
	}

	up(&vault_lock);
	return cur;
}

struct vault_device_node *get_vault_by_cdev(struct cdev *id)
{
	struct vault_device_node *cur = &vaults;

	if (unlikely(debug_param != 0))
		pr_info(NAME "get_vault_by_cdev(%p)\n", id);

	if (down_interruptible(&vault_lock) != 0) {
		up(&vault_lock);
		return ERR_PTR(-EINTR);
	}

	/* root element is not set */
	if (cur->id == -1) {
		up(&vault_lock);
		return NULL;
	}

	/* Search for the vault which has the cdev */
	while (cur != NULL
	     && cur->vault != NULL
	     && cur->vault->cdev != id)
		cur = cur->next;

	up(&vault_lock);
	return cur;
}

void clean_up_sv_ctl_device(void)
{
	device_del(sv_ctl.device);
	class_destroy(sv_ctl.class);
	cdev_del(sv_ctl.cdev);
	unregister_chrdev_region(MKDEV(MAJOR_DEV_NUM, MINOR_DEV_NUM), 254);
}

int delete_node(unsigned int id)
{
	struct vault_device_node *cur = &vaults;
	struct vault_device_node *prev = NULL;
	int free_data_result = 0;

	if (unlikely(down_interruptible(&vault_lock) != 0)) {
		up(&vault_lock);
		return -EINTR;
	}

	/* Search for the vault with the id */
	while (cur != NULL && cur->id != id) {
		prev = cur;
		cur = cur->next;
	}

	/* Trying to delete not created nodes */
	if (unlikely(cur == NULL)) {
		pr_warn(NAME "Unable to delete node with id=%d\n", id);

		up(&vault_lock);
		return -EINVAL;
	}

	/* Delte actual data */
	free_data_result = free_node_data(cur, 0);
	if (unlikely(free_data_result != SUCCESS)) {
		up(&vault_lock);
		return free_data_result;
	}

	/* Free actual node pointer */
	if (cur != &vaults) {
		prev->next = cur->next;
		kfree(cur);
	} else {
		if (vaults.next != NULL) {
			cur = vaults.next;

			vaults.id    = cur->id;
			vaults.vault = cur->vault;
			vaults.next  = cur->next;

			kfree(cur); //delete next node (it's now on stack)
		} else {
			vaults.id    = -1;
			vaults.vault = NULL;
			vaults.next  = NULL;
		}
	}

	up(&vault_lock);
	return SUCCESS;
}

int free_node_data(struct vault_device_node *node, int force)
{
	if (unlikely(debug_param != 0))
		pr_info(NAME "free_node_data(%p,%d)\n", node, force);

	/* Check if we should delete even if the device is open */
	if (force == 1) {
		if (node->vault->usage > 0)
			pr_warn(NAME "Deleting vault which is in usage!\n");

	} else {
		if (node->vault->usage > 0) {
			pr_warn(NAME "%s %s\n"
				, "Trying to delete a vault which"
				, "is in usage!");

			return -EBUSY; //Deny deleting used vault
		}
	}

	/* Free system devices */
	device_del(node->vault->device);
	cdev_del(node->vault->cdev);

	/* Delete vault data only if it exists */
	if (unlikely(node->vault != NULL && node->vault->data != NULL)) {
		/* securely delete data */
		(void) memset(node->vault->data
				, 0x0
				, node->vault->size);

		kfree(node->vault->data);
	}

	return SUCCESS;
}

void clean_up_sv_data(void)
{
	struct vault_device_node *cur = &vaults;

	if (unlikely(debug_param != 0))
		pr_info(NAME "clean_up_sv_data\n");

	/* should not be interruptable */
	down(&vault_lock);

	/* if we failed in the __init method we have no devices */
	if (vaults.vault == NULL)
		return;

	/* delete as long as we have vaults in our memory */
	while (cur != NULL) {
		struct vault_device_node *oc = cur;
		(void) free_node_data(oc, 1); //forcefully delete node

		cur = cur->next;

		//freeing the stack would be a **very** bad idea ...
		if (unlikely(oc != &vaults))
			kfree(oc);
	}
}

const char *ioctl_number_to_string(unsigned long ioctl)
{
	switch (ioctl) {
	case IOCTL_SELECT_VAULT: return "select_vault";
	case IOCTL_GET_SIZE: return "get_size";
	case IOCTL_CHANGE_KEY: return "change_key";
	case IOCTL_SET_SIZE: return "set_size";
	case IOCTL_CLEAN: return "clean";
	case IOCTL_EXISTS_VAULT: return "exists_vault";
	case IOCTL_CREATE: return "create_vault";
	case IOCTL_REMOVE: return "remove";
	}

	return "unknown";
}

int validate_access(struct secvault_data *vault)
{
	return vault->user_id.val == get_current_user()->uid.val;
}
