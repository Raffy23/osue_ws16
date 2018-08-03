/**
 *  / ____|                          \ \    / /        | | |
 * | (___   ___  ___ _   _ _ __ ___   \ \  / /_ _ _   _| | |_
 *  \___ \ / _ \/ __| | | | '__/ _ \   \ \/ / _` | | | | | __|
 *  ____) |  __/ (__| |_| | | |  __/    \  / (_| | |_| | | |_
 * |_____/ \___|\___|\__,_|_|  \___|     \/ \__,_|\__,_|_|\__|
 *
 * Main file of the Secure Vault module.
 *
 * This module provides encrypted memory devices (sv_data) which
 * can be used by all programs to write and read. These devices
 * must first be initalized with the userspace tool svctl.
 *
 * @brief This is the main file of the secure vault module.
 * @author Raphael Ludwig (1526280)
 * @date 21. Dez 2016
 * @version 1
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/ioctl.h>
#include <linux/string.h>
#include <asm/uaccess.h>
#include <asm/errno.h>

/* This headerfile contains shared data between kernel and userspace */
#include "../secvault.h"

/* Contains type definitions*/
#include "secvault_inc.h"

/* Headers contain functionality */
#include "secvault_ioctl.h"
#include "secvault_fops.h"
#include "secvault_misc.h"

/* Needed by the Kernel for my Module */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Raphael Ludwig");
MODULE_DESCRIPTION("A kernel module which provides encrypted memory devices");
MODULE_SUPPORTED_DEVICE("sv_data")

int debug_param;
module_param_named(debug, debug_param, int, 0664);
//MODULE_PARM_DESC(debug_param, "set to 1 to increase debug output");
MODULE_PARM_DESC(debug, "set to 1 to increase debug output");

/* === Implementation === */
static int __init secvault_init(void)
{
	int ret = 0;

	pr_info(NAME "Module is starting ...\n");

	ret = register_chrdev_region(MKDEV(MAJOR_DEV_NUM, MINOR_DEV_NUM)
					, 254
					, "secure_vault");
	if (ret < 0) {
		pr_alert(NAME "Unable to register device region\n");
		return ret;
	}

	ret = create_secvault_ctl_device();
	if (ret != SUCCESS) {
		unregister_chrdev_region(MKDEV(MAJOR_DEV_NUM, MINOR_DEV_NUM)
					, 254);
		pr_alert(NAME "%s, %s %s %d\n"
			, "Unable to create control device"
			, "module start failed!"
			, "Error Code:"
			, ret);

		return ret;
	}

	sema_init(&vault_lock, 1);

	if (debug_param != 0)
		pr_info(NAME "Started in 'debug' mode!\n");

	pr_info(NAME "Initalization completed\n");
	return SUCCESS;
}

static void __exit secvault_exit(void)
{
	pr_info(NAME "Module will be unloaded ...\n");
	pr_info(NAME "All Data in vaults will be freed");

	/* Clean up all the devices we created */
	clean_up_sv_data();
	clean_up_sv_ctl_device();
}

module_init(secvault_init);
module_exit(secvault_exit);
