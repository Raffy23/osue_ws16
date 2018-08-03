/*!\mainpage
 *
 *  / ____|                          \ \    / /        | | |  
 * | (___   ___  ___ _   _ _ __ ___   \ \  / /_ _ _   _| | |_ 
 *  \___ \ / _ \/ __| | | | '__/ _ \   \ \/ / _` | | | | | __|
 *  ____) |  __/ (__| |_| | | |  __/    \  / (_| | |_| | | |_ 
 * |_____/ \___|\___|\__,_|_|  \___|     \/ \__,_|\__,_|_|\__|
 *
 *
 * This header file contains the magix ioctl numbers and other
 * global constances which are shared between kernel and user
 * space.
 *
 */
#ifndef SECVAULT_HEADER
#define SECVAULT_HEADER

#include <linux/ioctl.h>

#define MAX_SIZE 		(1048577)
#define MIN_SIZE 		(0)
#define KEY_SIZE 		(10)
#define NUM_VALUTS 		(3)

#define MAJOR_DEV_NUM 		(231)
#define MINOR_DEV_NUM		(0)

#define DEV_SECVAULT_CTL 	("/dev/sv_ctl")
#define DEV_SECVAULT_BASE 	("/dev/sv_data")

#define DEV_SECVAULT_1 		(DEV_SECVAULT_BASE "0")
#define DEV_SECVAULT_2 		(DEV_SECVAULT_BASE "1")
#define DEV_SECVAULT_3 		(DEV_SECVAULT_BASE "2")
#define DEV_SECVAULT_4 		(DEV_SECVAULT_BASE "3")

/* ==== IOCTL stuff below here ==== */

// ioctl for everything
#define IOCTL_GET_SIZE     _IOR(MAJOR_DEV_NUM, 0 , unsigned int    )
#define IOCTL_CHANGE_KEY   _IOW(MAJOR_DEV_NUM, 1 , char *          )
#define IOCTL_CLEAN	   _IO (MAJOR_DEV_NUM, 2                   )
#define IOCTL_REMOVE       _IO (MAJOR_DEV_NUM, 3                   )
#define IOCTL_CREATE       _IO (MAJOR_DEV_NUM, 4                   )
#define IOCTL_SELECT_VAULT _IOW(MAJOR_DEV_NUM, 5 , unsigned int    )
#define IOCTL_SET_SIZE     _IOW(MAJOR_DEV_NUM, 6 , unsigned int    )
#define IOCTL_EXISTS_VAULT _IO (MAJOR_DEV_NUM, 7                   )

#endif
