/**
 *  / ____|                          \ \    / /        | | |
 * | (___   ___  ___ _   _ _ __ ___   \ \  / /_ _ _   _| | |_
 *  \___ \ / _ \/ __| | | | '__/ _ \   \ \/ / _` | | | | | __|
 *  ____) |  __/ (__| |_| | | |  __/    \  / (_| | |_| | | |_
 * |_____/ \___|\___|\__,_|_|  \___|     \/ \__,_|\__,_|_|\__|
 *
 * This header file does contain all ioctl function definitions
 */

#ifndef SECVAULT_IOCTL
#define SECVAULT_IOCTL

#include "secvault_inc.h" //types which are needed for these operations

/*
 * ioctl request for getting the size of the sv_data device, size is returned
 * as positive value
 */
extern long ioctl_get_size(unsigned long arg, struct secvault_data *vault);

/*
 * ioctl request to change the key, arg must be the userspace pointer
 * to the key
 */
extern long ioctl_change_key(unsigned long arg, struct secvault_data *vault);

/*
 * ioctl request to change the size of a sv_data vault, arg must be a valid
 * size
 */
extern long ioctl_change_size(unsigned long arg, struct secvault_data *vault);

/*
 * ioctl request to clean the vault
 */
extern long ioctl_clean(struct secvault_data *vault);

/*
 * ioctl request to select a vault for more manipulatins, arg must be a valid
 * id otherwise ENODEV is retruned
 */
extern long ioctl_select_vault(struct ctl_priv_data *data, unsigned long arg);

/*
 * ioctl request for the deletion of the vault, this also zeros out the
 * data block
 */
extern long ioctl_delete_vault(struct secvault_data *vault, unsigned int id);

#endif
