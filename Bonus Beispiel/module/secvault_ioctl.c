/**
 *  / ____|                          \ \    / /        | | |
 * | (___   ___  ___ _   _ _ __ ___   \ \  / /_ _ _   _| | |_
 *  \___ \ / _ \/ __| | | | '__/ _ \   \ \/ / _` | | | | | __|
 *  ____) |  __/ (__| |_| | | |  __/    \  / (_| | |_| | | |_
 * |_____/ \___|\___|\__,_|_|  \___|     \/ \__,_|\__,_|_|\__|
 *
 * This file does contain all the ioctl functions which are provided
 * by the secure vault kernel module
 */
#define __NO_VERSION__

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#include "../secvault.h"
#include "secvault_ioctl.h"
#include "secvault_misc.h"

long ioctl_get_size(unsigned long arg, struct secvault_data *vault)
{
	int ret = 0;

	if (unlikely(debug_param != 0))
		dev_info(vault->device, "ioctl_get_size(%lu)\n", arg);

	if (unlikely(validate_access(vault) == 0))
		return -EACCES;

	ret = put_user(vault->size, (unsigned int *)(arg));
	return ret;
}

long ioctl_change_key(unsigned long arg, struct secvault_data *vault)
{
	uint8_t *new_data = NULL;
	char new_key[10];
	int ret = 0;

	if (unlikely(debug_param != 0))
		dev_info(vault->device, "ioctl_change_key(%lu)\n", arg);

	if (unlikely(validate_access(vault) == 0))
		return -EACCES;

	vault->keyLength = KEY_SIZE;	//This does not work with variable keys
	ret = copy_from_user(new_key, (char *)(arg), sizeof(char) * KEY_SIZE);

	if (unlikely(ret != SUCCESS))
		return -EFAULT; //Userspace fucked up the memory

	// re-encrypt the data with new key
	new_data = kcalloc(vault->size, sizeof(char), GFP_KERNEL);
	if (unlikely(new_data == NULL))
		return -ENOMEM;

	for (int i = 0; i < vault->size; i++) {
		const uint8_t keyPos = i % vault->keyLength;
		const uint8_t data = vault->data[i] ^ vault->key[keyPos];

		new_data[i] = data ^ new_key[keyPos];
	}

	kfree(vault->data);
	vault->data = new_data;

	return SUCCESS;
}

long ioctl_change_size(unsigned long arg, struct secvault_data *vault)
{
	const unsigned int new_size = arg;
	uint8_t *new_data = NULL;

	if (unlikely(debug_param != 0))
		dev_info(vault->device, "ioctl_change_size(%lu)\n", arg);

	if (unlikely(validate_access(vault) == 0))
		return -EACCES;

	if (unlikely(new_size <= 0))
		return -EINVAL;

	new_data = kcalloc(new_size, sizeof(char), GFP_KERNEL);

	if (unlikely(new_data == NULL))
		return -ENOMEM;

	(void) memcpy(new_data, vault->data, vault->size);
	kfree(vault->data);

	vault->data = new_data;
	vault->size = new_size;

	return SUCCESS;
}

long ioctl_delete_vault(struct secvault_data *vault, unsigned int id)
{
	if (unlikely(debug_param != 0))
		dev_info(vault->device, "ioctl_delete_vault(%u)\n", id);

	if (unlikely(validate_access(vault) == 0))
		return -EACCES;

	return delete_node(id);
}

long ioctl_select_vault(struct ctl_priv_data *data
			      , unsigned long arg)
{
	if (unlikely(debug_param != 0))
		dev_info(sv_ctl.device, "ioctl_select_vault(%lu)\n", arg);

	data->target = arg;
	return SUCCESS;
}

long ioctl_clean(struct secvault_data *vault)
{
	if (unlikely(debug_param != 0))
		dev_info(vault->device, "ioctl_clean()\n");

	if (validate_access(vault) == 0)
		return -EACCES;

	if (vault->keyLength <= 0)
		return -EINVAL;

	/* //Overwrite with 'data' zeros instead of memory zeros
	 * for (int i = 0; i < vault->size; i++) {
	 *	const uint8_t keyPos = i % vault->keyLength;
	 *
	 *	vault->data[i] = vault->key[keyPos];
	 *}
	 */

	(void) memset(vault->data, 0x0, vault->size);
	return SUCCESS;
}
