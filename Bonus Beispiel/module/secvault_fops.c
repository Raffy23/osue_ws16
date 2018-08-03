/**
 *  / ____|                          \ \    / /        | | |
 * | (___   ___  ___ _   _ _ __ ___   \ \  / /_ _ _   _| | |_
 *  \___ \ / _ \/ __| | | | '__/ _ \   \ \/ / _` | | | | | __|
 *  ____) |  __/ (__| |_| | | |  __/    \  / (_| | |_| | | |_
 * |_____/ \___|\___|\__,_|_|  \___|     \/ \__,_|\__,_|_|\__|
 */
#define __NO_VERSION__

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#include "../secvault.h"
#include "secvault_inc.h"
#include "secvault_fops.h"
#include "secvault_ioctl.h"
#include "secvault_misc.h"

/* === Global Structures === */

const struct file_operations secvault_fops = {
	.owner =    THIS_MODULE,
	.llseek =   secvault_llseek,
	.read =     secvault_read,
	.write =    secvault_write,
	.unlocked_ioctl =    secvault_unlocked_ioctl,
	.open =     secvault_open,
	.release =  secvault_release,
};

const struct file_operations secctl_fops = {
	.owner =    THIS_MODULE,
	.llseek =   NULL,
	.read =     NULL,
	.write =    NULL,
	.unlocked_ioctl =    sectl_unlocked_ioctl,
	.open =     secctl_open,
	.release =  secctl_release,
};

/* === Function implementation === */

loff_t secvault_llseek(struct file *fp, loff_t off, int mode)
{
	struct vault_priv_data *vault = fp->private_data;

	if (unlikely(debug_param != 0))
		dev_info(vault->shared->device, "llseek(%d)\n", mode);

	if (unlikely(down_interruptible(&vault->shared->lock) != 0)) {
		up(&vault->shared->lock);
		return -EINTR;
	}

	switch (mode) {
	case 0: // SEEK_SET
		if (unlikely(off > vault->shared->size || off < 0))
			goto clean_up_error;

		vault->pos = off;
		break;

	case 1: // SEEK_CUR
		if (unlikely(vault->pos + off < 0
		  || vault->pos + off > vault->shared->size))
			goto clean_up_error;

		vault->pos += off;
		break;

	case 2: // SEEK_END
		if (unlikely(off > 0 || vault->shared->size+off < 0))
			goto clean_up_error;

		vault->pos += off;
		break;

	default: // Not supported seek operations
		goto clean_up_error;

	}

	up(&vault->shared->lock);
	return vault->pos;

clean_up_error:
	up(&vault->shared->lock);
	return -EINVAL;
}

ssize_t secvault_read(struct file *fp
			    , char __user *buffer
			    , size_t count
			    , loff_t *offset)
{
	struct vault_priv_data *vault = fp->private_data;
	ssize_t bytes_read = 0;

	/* Debug output */
	if (unlikely(debug_param != 0))
		dev_info(vault->shared->device, "Read %lu Bytes\n", count);

	/* resource locking */
	if (unlikely(down_interruptible(&vault->shared->lock) != 0)) {
		up(&vault->shared->lock);
		return -EINTR;
	}

	/* check if device was initalized properly */
	if (unlikely(vault->shared->data == NULL
	  || vault->shared->keyLength == 0)) {

		up(&vault->shared->lock);

		if (debug_param != 0)
			dev_info(vault->shared->device
				, "Error: Key=%d data=%p"
				, vault->shared->keyLength
				, vault->shared->data);

		return -ENODEV;
	}

	/* check if we are in a valid range */
	if (unlikely(vault->pos >= vault->shared->size)) {
		up(&vault->shared->lock);

		if (debug_param != 0)
			dev_info(vault->shared->device
				, "Error: Overflow: %d / %d Byte"
				, vault->pos
				, vault->shared->size);

		return -ENOSPC;//OVERFLOW;
	}

	/* do actual data read */
	for (; bytes_read < count
	   && vault->pos < vault->shared->size
	     ; bytes_read++) {
		const uint8_t keyPos = vault->pos % vault->shared->keyLength;
		const uint8_t data = vault->shared->data[vault->pos] ^
					vault->shared->key[keyPos];

		/* copy buffer to user, if it fails return with result */
		if (put_user(data, buffer+bytes_read) == -EFAULT)
			goto clean_exit;

		vault->pos++;
	}

clean_exit:
	/* Debug output */
	if (unlikely(debug_param != 0))
		dev_info(vault->shared->device
			, "Actually read %lu Bytes\n"
			, bytes_read);

	up(&vault->shared->lock);
	if (unlikely(bytes_read == 0 && count > 0))
		return -EFAULT;

	return bytes_read;
}

ssize_t secvault_write(struct file *fp
			     , const char __user *buffer
			     , size_t count
			     , loff_t *offset)
{
	struct vault_priv_data *vault = fp->private_data;
	ssize_t bytes_wrote = 0;

	/* debug output */
	if (unlikely(debug_param != 0))
		dev_info(vault->shared->device, "Write %lu Bytes\n", count);

	/* resource locking */
	if (unlikely(down_interruptible(&vault->shared->lock) != 0)) {
		up(&vault->shared->lock);
		return -EINTR;
	}

	/* check if device was initalized properly */
	if (unlikely(vault->shared->data == NULL
	  || vault->shared->keyLength == 0)) {
		up(&vault->shared->lock);

		if (debug_param != 0)
			dev_info(vault->shared->device
				, "Error: Key=%d data=%p"
				, vault->shared->keyLength
				, vault->shared->data);

		return -ENODEV;
	}

	/* check if we are in range of the file */
	if (unlikely(vault->pos >= vault->shared->size)) {
		up(&vault->shared->lock);

		if (debug_param != 0)
			dev_info(vault->shared->device
				, "Error: Overflow: %d / %d Byte"
				, vault->pos
				, vault->shared->size);


		return -ENOSPC;
	}

	/* actual file write */
	for (; bytes_wrote < count && vault->pos < vault->shared->size
	     ; bytes_wrote++) {
		const uint8_t keyPos = vault->pos % vault->shared->keyLength;
		uint8_t dataBlock = 0;

		if (get_user(dataBlock, buffer+bytes_wrote) == -EFAULT)
			goto clean_exit;

		vault->shared->data[vault->pos] = dataBlock ^
				vault->shared->key[keyPos];
		vault->pos++;
	}

clean_exit:
	/* Debug output */
	if (unlikely(debug_param != 0))
		dev_info(vault->shared->device
			, "Actually wrote %lu Bytes\n"
			, bytes_wrote);

	up(&vault->shared->lock);
	if (unlikely(bytes_wrote == 0 && count > 0))
		return -EFAULT;

	return bytes_wrote;
}



long secvault_unlocked_ioctl(struct file *fp
				   , unsigned int request
				   , unsigned long arg)
{
	struct vault_priv_data *pdata = fp->private_data;
	struct secvault_data *vault = pdata->shared;
	long result = 0;

	/* Yes this was not even tested ... */
	pr_warn(NAME "IOCTLs for Vaults are not tested!\n");

	if (unlikely(debug_param != 0))
		dev_info(vault->device
			, "ioctl_vault(%p, %u, %lu)\n"
			, fp, request, arg);

	/* resource locking */
	if (unlikely(down_interruptible(&vault->lock) != 0)) {
		up(&vault->lock);
		return -EINTR;
	}

	switch (request) {
	case IOCTL_GET_SIZE:
		result = ioctl_get_size(arg, vault);
		break;

	case IOCTL_CHANGE_KEY:
		result = ioctl_change_key(arg, vault);
		break;

	case IOCTL_CLEAN:
		result = ioctl_clean(vault);
		break;

	case IOCTL_SET_SIZE:
		result = ioctl_change_size(arg, vault);
		break;

	default:
		result = -ENOTTY; //POSIX standard ...
	}

	up(&vault->lock);
	return result;
}

long sectl_unlocked_ioctl(struct file *fp
				, unsigned int request
				, unsigned long arg)
{
	struct ctl_priv_data *svctl_data = fp->private_data;
	struct vault_device_node *node = NULL;
	long result = 0;

	/* Do somde debug output */
	if (unlikely(debug_param != 0))
		dev_info(sv_ctl.device
			, "ioctl_svctl(%s, %lu)\n"
			, ioctl_number_to_string(request)
			, arg);

	/* select only vault and do nothing else */
	if (request == IOCTL_SELECT_VAULT)
		return ioctl_select_vault(svctl_data, arg);

	/* Check if targeted vault is in range */
	if (svctl_data->target == -1)
		return -EINVAL;

	/* get node and check for any error while getting */
	node = get_vault(svctl_data->target);
	if (unlikely(IS_ERR(node))) {
		if (debug_param != 0)
			dev_info(sv_ctl.device
				, "Error in get_vault: %ld"
				, PTR_ERR(node));

		return PTR_ERR(node);
	}

	/* even more debug output */
	if (unlikely(debug_param != 0)) {
		dev_info(sv_ctl.device
			, "node = %p \ttarget = %d"
			, node, svctl_data->target);
	}

	if (unlikely(node == NULL)) {
		/*
		 * even if we targeted an invalid device
		 * we might want to create it
		 */
		if (request == IOCTL_CREATE)
			return create_new_vault(svctl_data->target);

		return -ENODEV;
	}

	/* resource locking */
	if (unlikely(down_interruptible(&node->vault->lock) != 0)) {
		up(&node->vault->lock);
		return -EINTR;
	}

	/* Now we process 'normal' ioctl requests */
	switch (request) {
	case IOCTL_GET_SIZE:
		result = ioctl_get_size(arg, node->vault);
		break;

	case IOCTL_CHANGE_KEY:
		result = ioctl_change_key(arg, node->vault);
		break;

	case IOCTL_SET_SIZE:
		result = ioctl_change_size(arg, node->vault);
		break;

	case IOCTL_CLEAN:
		result = ioctl_clean(node->vault);
		break;

	case IOCTL_EXISTS_VAULT:
		result = SUCCESS;
		break;

	case IOCTL_REMOVE:
		result = ioctl_delete_vault(node->vault, svctl_data->target);

		/* if we success vault->lock does not exist anymore! */
		if (likely(result == SUCCESS))
			return SUCCESS;
		break;

	default:
		if (unlikely(debug_param != 0)) {
			dev_info(sv_ctl.device
				, "This ioctl request it not known: %d\n"
				, request);
		}

		//POSIX says that is the error value for invalid ioctl
		result = -ENOTTY;
	}

	up(&node->vault->lock);
	return result;
}

int secvault_open(struct inode *inode, struct file *fp)
{
	struct vault_device_node *node = NULL;
	struct vault_priv_data *priv_data = NULL;

	try_module_get(THIS_MODULE);

	node = get_vault_by_cdev(inode->i_cdev);
	if (unlikely(IS_ERR(node))) { //How is that even possible?
		pr_alert("(Open Vault) No such vault was created as cdev!\n");

		module_put(THIS_MODULE);
		return PTR_ERR(node);
	}

	if (unlikely(debug_param != 0))
		dev_info(node->vault->device, "Open Vault Device\n");

	if (unlikely(validate_access(node->vault) == 0)) {
		module_put(THIS_MODULE);
		return -EACCES;
	}

	/* resource locking */
	if (unlikely(down_interruptible(&node->vault->lock) != 0)) {
		up(&node->vault->lock);
		module_put(THIS_MODULE);
		return -EINTR;
	}

	/* just make sure this stuff isn't delete while opening */
	node->vault->usage += 1;
	up(&node->vault->lock);

	priv_data = kmalloc(sizeof(struct vault_priv_data), GFP_KERNEL);
	if (unlikely(priv_data == NULL)) {
		module_put(THIS_MODULE);
		return -ENOMEM;
	}

	/* init private data structure */
	priv_data->pos = 0;
	priv_data->shared = node->vault;

	/* save to kernel data structure */
	fp->private_data = priv_data;

	return SUCCESS;
}

int secvault_release(struct inode *inode, struct file *fp)
{
	struct vault_priv_data *priv_data = fp->private_data;

	/* debug output */
	if (unlikely(debug_param != 0))
		dev_info(priv_data->shared->device, "Close Vault Device\n");

	/* resource locking */
	if (unlikely(down_interruptible(&priv_data->shared->lock) != 0)) {
		up(&priv_data->shared->lock);
		return -EINTR;
	}

	/* make sure resource can be freed at some point */
	priv_data->shared->usage -= 1;
	up(&priv_data->shared->lock);

	/* free private data of FD */
	kfree(fp->private_data);

	/* release module */
	module_put(THIS_MODULE);
	return SUCCESS;
}

int secctl_open(struct inode *inode, struct file *fp)
{
	struct ctl_priv_data *ctl_data =
		kzalloc(sizeof(struct ctl_priv_data), GFP_KERNEL);

	try_module_get(THIS_MODULE);

	if (unlikely(debug_param != 0))
		dev_info(sv_ctl.device, "Open Control Device\n");

	if (unlikely(ctl_data == NULL)) {
		module_put(THIS_MODULE);
		return -ENOMEM;
	}

	fp->private_data = ctl_data;
	return SUCCESS;
}

int secctl_release(struct inode *inode, struct file *fp)
{
	if (unlikely(debug_param != 0))
		dev_info(sv_ctl.device, "Close Control Device\n");

	kfree(fp->private_data);
	module_put(THIS_MODULE);
	return SUCCESS;
}
