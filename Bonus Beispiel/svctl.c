/*!
 * This Tool is part of the secvault kernel module and is responsible for communicating 
 * with it
 *
 * @brief Secure vault controller
 * @author Raphael Ludwig (e1526280)
 * @version 1
 * @date 22 Dez. 2016
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h> 
#include <sys/ioctl.h>

#include "secvault.h"

#define SIZE 256
#define TARGET 1

#define ARG_CREATE 'c'
#define ARG_CH_KEY 'k'
#define ARG_DELETE 'e'
#define ARG_REMOVE 'd'

#define ARG_CMD ("c:ked")

/* progamname */
static const char *progname = "svctl";

/**
 * @breif This function does parse the arguments and returns them with the pointers
 * @param argc number of arguments
 * @param argv arguments from main
 * @param arg result pointer for the request which should be made
 * @param param result pointer for the parameter for the requesr
 * @param secID result pointer for the targeted vault
 */
static void parse_arguments(int argc, char **argv, int *arg, int *param, int *secID);

/**
 * @brief prints the usage and exits the program
 */
static void print_usage(void);

/**
 * This function represents the entry point to the secure vault user space tool.
 *
 * @brief main function of the svctl tool
 * @param argc argument counter of how many arguments
 * @param argv the actual agruments for this program
 * @return EXIT_SUCCESS or EXIT_FAILURE
 */
int main(int argc,char **argv) {
	const int sv_ctl = open(DEV_SECVAULT_CTL, 0);
	if( sv_ctl < 0 ) {
		(void) fprintf( stderr , "Unable to open control device %s!\n",	DEV_SECVAULT_CTL);
		exit(EXIT_FAILURE);
	}
	
	int cmd    = 0;
	int param  = 0;
	int target = 0;

	parse_arguments(argc, argv, &cmd, &param, &target);
	
	/* select vault for operation */	
	long ret = ioctl(sv_ctl, IOCTL_SELECT_VAULT, target);
	if( ret < 0 && cmd != ARG_CREATE ) {
		perror("select_vault");
		exit(EXIT_FAILURE);
	}
	
	/* limit targets to match task description */
	if( target < 0 || target > NUM_VALUTS) {
		(void) fprintf( stderr , "Target vault must be between 0 and 3!\n");
		exit(EXIT_FAILURE);
	}

	/* switch the request argument */
	switch( cmd ) {
		case ARG_CREATE:
			if( param < MIN_SIZE || param > MAX_SIZE ) {
				(void) fprintf( stderr , "Size must be between %d and %d!\n", MIN_SIZE, MAX_SIZE);
				exit(EXIT_FAILURE);
			}
			
			ret = ioctl(sv_ctl, IOCTL_EXISTS_VAULT);
			if( ret != 0 ) {
				(void) fprintf( stdout , "Vault does not exist, creating device ...\n");
				
				ret = ioctl(sv_ctl, IOCTL_CREATE);
				if( ret < 0 ) {
					perror("create_vault");
					exit(EXIT_FAILURE);
				}
			}

			ret = ioctl(sv_ctl, IOCTL_SET_SIZE, param);
			if( ret < 0 ) {
				perror("set_size");
				exit(EXIT_FAILURE);
			}

		/* fall through for entering the key*/
		case ARG_CH_KEY:
			(void) fprintf( stdout, "Please enter a key (max size 10 chars):\n");		

			char key[KEY_SIZE] = { 0x0 };
			(void) fgets( key, KEY_SIZE, stdin );

			ret = ioctl(sv_ctl, IOCTL_CHANGE_KEY, key);
			if( ret < 0 ) {
				perror("change_key");
				exit(EXIT_FAILURE);
			}
			break;

		case ARG_DELETE:
			ret = ioctl(sv_ctl, IOCTL_CLEAN );
			if( ret < 0 ) {
				perror("clean_vault");
				exit(EXIT_FAILURE);
			}
			break;

		case ARG_REMOVE:
			ret = ioctl(sv_ctl, IOCTL_REMOVE);
			if( ret < 0 ) {
				perror("remove_vault");
				exit(EXIT_FAILURE);
			}
			break;

		default:
			(void) fprintf( stderr, "Unknown command\n");
	}	
	
	close(sv_ctl);
	return EXIT_SUCCESS;
}

static void parse_arguments(int argc, char **argv, int *arg, int *param, int *secID) {
	int opt      =  0;
	int cur_arg  = -1;
	int new_size =  0;
	
	progname = argv[0];
	while( (opt = getopt(argc, argv, ARG_CMD)) != -1) {
		switch (opt) {
			case ARG_CREATE:
				if( cur_arg == -1 ) cur_arg = ARG_CREATE;
				else print_usage();
				
				new_size = strtol(optarg, NULL, 10);
				break;
			
			case ARG_DELETE:
				if( cur_arg == -1 ) cur_arg = ARG_DELETE;
				else print_usage();			
				break;
			
			case ARG_CH_KEY:
				if( cur_arg == -1 ) cur_arg = ARG_CH_KEY;
				else print_usage();
				break;

			case ARG_REMOVE:
				if( cur_arg == -1 ) cur_arg = ARG_REMOVE;
				else print_usage();
				break;
		
			default: /* '?' */
				(void) fprintf( stderr, "Unknown Command!\n");
				print_usage();
		}
	}
	
	if( argc != optind+1 )
		print_usage();
	
	*arg   = cur_arg;
	*param = new_size;
	*secID = strtol(argv[optind], NULL, 10); 
}

static void print_usage(void) {
	(void) fprintf( stdout , "Usage: %s [-c <size>|-k|-e|-d] <secvault id>\n", progname );
	exit(EXIT_FAILURE);
}
