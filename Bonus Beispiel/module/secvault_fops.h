/**
 *  / ____|                          \ \    / /        | | |
 * | (___   ___  ___ _   _ _ __ ___   \ \  / /_ _ _   _| | |_
 *  \___ \ / _ \/ __| | | | '__/ _ \   \ \/ / _` | | | | | __|
 *  ____) |  __/ (__| |_| | | |  __/    \  / (_| | |_| | | |_
 * |_____/ \___|\___|\__,_|_|  \___|     \/ \__,_|\__,_|_|\__|
 *
 * This header contains all the function definitions and file
 * operation strucutres which are needed by the kernel module
 */
#ifndef SECVAULT_FOPS
#define SECVAULT_FOPS

#include <linux/fs.h>

/* === Gloabl Structures === */

/*
 * This structure does store the file operations
 * for all sv_data devices
 */
extern const struct file_operations secvault_fops;

/*
 * This structure does store the file operations
 * for the sv_ctl device (explicitly denies write /
 * read access)
 */
extern const struct file_operations secctl_fops;

/* === File operation Functions === */

extern loff_t secvault_llseek(struct file *fp, loff_t off, int mode);

extern ssize_t secvault_read(struct file *fp
			    , char __user *buffer
			    , size_t count
			    , loff_t *offset);

extern ssize_t secvault_write(struct file *fp
			     , const char __user *buffer
			     , size_t count
			     , loff_t *offset);

extern long secvault_unlocked_ioctl(struct file *fp
				   , unsigned int request
				   , unsigned long arg);

extern int secvault_open(struct inode *inode, struct file *fp);

extern int secvault_release(struct inode *inode, struct file *fp);


extern long sectl_unlocked_ioctl(struct file *fp
				, unsigned int request
				, unsigned long arg);

extern int secctl_open(struct inode *inode, struct file *fp);

extern int secctl_release(struct inode *inode, struct file *fp);

#endif
