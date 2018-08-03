/*! \mainpage 
 * 
 * \section intro_sec Beschreibung
 * Schreiben Sie ein Programm, das die beiden Kommandos command1 und 
 * command2 ausführt, deren Ausgaben einliest und in ein gemeinsames 
 * Array speichert. Dieses Array wird dann sortiert und an das Unix-
 * Kommando <code>uniq -d</code> weitergegeben. Die Ausgabe Ihres 
 * Programmes soll also identisch sein mit jener des folgenden 
 * Shellskripts :
 * 
 * <code>
 * #!/bin/bash
 * ( $1; $2 ) | sort | uniq -d
 * </code>
 *  
 * \htmlonly
 * <embed src="dsort_td.pdf" width="100%" heigth="800px" href="dsort_td.pdf"></embed> 
 * \endhtmlonly
 *
 *
 * @brief A program which does the same as this bash script: <code>( $1; $2 ) | sort | uniq -d</code>
 * @author Raphael Ludwig (e1526280)
 * @version 1
 * @date 25 Nov. 2016
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>
#include <signal.h>

/* === Constants === */
/**
 * @brief max size of the line buffer
 */
#define BUFFER_SIZE (1024)

/* === Type Definitions === */

/**
 * @brief A structure which does hold the path to the programs from the argument list
 */
struct options {
	/**
	 * @brief Path to the first program
	 */
	char *program1;
	/**
	 * @brief Path to the second program
	 */
	char *program2;
};

/**
 * @brief This structure does hold a line of output from the programs and also a next pointer.
 */
struct record {
	/**
	 * @brief Output of a single line of the child program.
	 */
	char data[BUFFER_SIZE];
	/**
	 * @brief pointer to the next structure, is NULL if end is reached
	 */
	struct record *next;
};

/**
 * @brief This structure does hold multiple data for the child programs like file descriptors, the path for the exec and the pid
 */
struct params {
	/**
	 * @brief Array for STDIN and STDOUT file descriptors
	 */
	int fd[2];
	/**
	 * @brief this string is splited so that execvp does take it
	 */
	char *exec;
	
	/**
	 * @brief pid of the child program
	 */
	pid_t pid;
	/**
	 * @brief the data which is read by the parent from STDOUT of the child
	 */
	struct record *data;
	/**
	 * @brief number of read records
	 */
	int records;
};

/**
 * @brief A structure which does hold all the Data which is needed by the Program
 */
struct environment {
	/**
	 * @brief A structure which does hold all parameters for the first program
	 */
	struct params p1;
	/**
	 * @brief A structure which does hold all parameters for the second program
	 */
	struct params p2;
	
	/**
	 * @brief all the STDOUT data from the children mergend, does get sorted
	 */
	char **data;
	/**
	 * @brief the size of data 
	 */
	int length;
};

/* === Globals === */

/**
 * @brief The Base name of the Program which is displayed in the error message
 */
static const char *progname = "dsort";
/**
 * @brief Pointer to the Environment struct which does hold all the Data for the Program
 */
static struct environment *env;
/**
 * @brief Variable which is set to 1 by the signal_handler if SIGINT was recieved
 */
static sig_atomic_t stopSignal = 0;

/* === Prototypes === */

/**
 * @brief prases the arguments, does not return if an error occurs
 * @param opts a valid pointer to a options struct to save the program paths
 * @param argc argument counter
 * @param argv the arguments from the main method
 */
static void parse_arguments(struct options *opts,int argc,char *argv[]);

/**
 * @brief frees all resources
 */
static void free_resources(void);

/**
 * @brief prints an error message exits the program
 * @param exitcode the code with which the program should exit
 * @param fmt the format string for the output
 * @param ... arguments for the format string
 */
static void bail_out(int exitcode,const char *fmt, ...);

/**
 * @brief prints the usage to stdout
 */
static void print_usage(void);

/**
 * @brief forks and execs the other command
 * @param p a pointer to a params struct which does contain all the data for starting a child
 */
static void launch(struct params *p); 

/**
 * @brief frees all record resources
 * @param head a pointer to the head of a record linked list
 */
static void free_records(struct record *head);

/**
 * This function does collect the data from the p1 and p2 in the environment
 * struct and does save the the data from the record into the data field of 
 * the environment structure
 *
 * @brief collects all the record into the data arrays
 */
static void collect_data(void);

/**
 * @brief copies all data pointers from record to data
 * @param offset at which offset on the data array is should start
 * @param head a pointer to the head of a record linked list
 * @param size how many items form the record list should be copied
 */
static void link_collected_data(int offset,struct record *head,int size);

/**
 * @brief frees all data alloceated by the params structure
 * @param p a pointer to a structure which will be freed (all next pointers are included!)
 */
static void free_params(struct params *p);

/**
 * @brief counts how many specific chars are in the string
 * @param str a pointer to a string which should be searched
 * @param c a character which should be counted
 */
static int strcnt(const char * const str,int c);

/**
 * @brief compare function for qsort, does compare strings a1 and a2 with strcmp
 * @param a1 first string
 * @param a2 second string
 */
static int cmp_data(const void *a1,const void *a2);

/**
 * @brief prints the unique data to stdout
 */
static void print_uniq_data(void);

/**
 * @brief handles the signals
 * @param signal the signal for the signal handler 
 */
static void signal_handler(int signal);

/**
 * This function does prepare the parameters for execvp 
 * and invokes the function. It should only be called
 * by a forked child!
 *
 * @brief does the work of the forked child
 * @param p a pointer to the valid params structure
 */
static void do_child_work(struct params *p);

/**
 * This function reads the STDOUT of the forked child
 *
 * @brief does the work of the parent after the fork
 * @param p a pointer to the child params structure
 */
static void do_parent_work(struct params *p);

/**
 * @brief parses the parameters in the p structure to an argv array with the length of argc
 * @param argc the number of arguments which should be parsed from p
 * @param argv the ouput array of strings which does cointain data from the params structure
 * @param p a pointer to the params structure
 */
static void parse_to_argv(int argc,char **argv,struct params *p);

/**
 * @brief this function does install all the signal handlers which are needed by the parent
 */
static void install_signal_handlers(void);

/**
 * @brief main function of dsort
 * @param argc argument counter of how many arguments
 * @param argv the actual agruments for this program
 * @return EXIT_SUCCESS
 */
int main(int argc,char *argv[]) {
	
	struct options opts;
	parse_arguments( &opts, argc, argv );
	
	install_signal_handlers();

	/* create global memory space */
	env = (struct environment *) malloc( sizeof(struct environment) );
	if( env == NULL ) {
		bail_out(EXIT_FAILURE,"malloc");
	}
	
	memset( env , 0 , sizeof(struct environment) );
	env->p1.fd[STDIN_FILENO ] = -1;
	env->p1.fd[STDOUT_FILENO] = -1;
	env->p2.fd[STDIN_FILENO ] = -1;
	env->p2.fd[STDOUT_FILENO] = -1;
	
	env->p1.exec = strdup( opts.program1 );
	env->p2.exec = strdup( opts.program2 );

	/* each function call may more cpu time than expected (on big outputs) */
	if( stopSignal == 0 ) launch( &env->p1 );
	if( stopSignal == 0 ) launch( &env->p2 );

	if( stopSignal == 0 ) collect_data();
	if( stopSignal == 0 ) qsort( env->data, env->length, sizeof(char *), cmp_data);
	if( stopSignal == 0 ) print_uniq_data();
	
	free_resources();
	return EXIT_SUCCESS;
}

static void install_signal_handlers(void) {
	/* install signal handlers */
	struct sigaction s;
	
	s.sa_handler = signal_handler;
	s.sa_flags   = 0;
	
	if( sigfillset(&s.sa_mask) < 0 ) {
		bail_out(EXIT_FAILURE, "sigfillset");
	}

	if( sigaction(SIGINT,&s,NULL) < 0 ) {
		bail_out(EXIT_FAILURE, "sigaction");
	}

}

static int cmp_data(const void *a1,const void *a2) {
	const char **c1 = (const char**)a1;
	const char **c2 = (const char**)a2;

	return strcmp(*c1,*c2);
}

static void print_uniq_data(void) {
	char *lastprt = NULL;

	for(int i=0; i<env->length-1; i++) {
		if( strcmp(env->data[i],env->data[i+1]) == 0 ) {
			if( lastprt == NULL ||  (lastprt != NULL && strcmp(env->data[i],lastprt) != 0) ) {
				(void) fprintf( stdout , "%s" , env->data[i] );
				lastprt = env->data[i];
			}
		}
	}

}


static void collect_data(void) {
	env->length = env->p1.records + env->p2.records + 1;
	env->data = (char **) malloc( sizeof(char *) * env->length );
	memset( env->data, 0 , sizeof(char *) * env->length );

	link_collected_data(0,env->p1.data,env->p1.records);
	link_collected_data(env->p1.records,env->p2.data,env->p2.records);
}

static void link_collected_data(int offset,struct record *head,int size) {
	for(int i=offset; i-offset < size || head != NULL; i++ ) {
		env->data[i] = head->data;
		head = head->next;
	}
}

static void launch(struct params *p) {
	if( pipe( p->fd ) != 0 ) {
		bail_out(EXIT_FAILURE,"pipe");
	}

	p->pid = fork();
	if( p->pid == -1 ) {
		bail_out(EXIT_FAILURE,"fork");
	}

	if( p->pid == 0 ) {
		do_child_work(p);
	} else {
		do_parent_work(p);
	}
}

static void do_child_work(struct params *p) {
	close( p->fd[STDIN_FILENO] );
	close( STDOUT_FILENO );
	
	p->fd[STDIN_FILENO] = -1;
	if( dup2( p->fd[STDOUT_FILENO], STDOUT_FILENO ) < 0 ) {
		perror("dup2");
	}

	const int argc = strcnt(p->exec,' ') + 1;
	char *argv[ argc+1 ];

	parse_to_argv( argc, argv, p );
	argv[ argc ] = NULL;	// exec must have NULL element at last pos
	
	if( execvp(argv[0],argv) == -1 ) {
		close( p->fd[STDOUT_FILENO] );
		p->fd[STDOUT_FILENO] = -1;
		perror("exec");
	}

	assert(false);
	exit(EXIT_FAILURE); 
}

static void parse_to_argv(int argc,char **argv,struct params *p) {
	if( argc == 1 ) {
		argv[0] = p->exec;
	} else {
		int argcnt = 2;
		char *arg  = strchr(p->exec,' ');
		argv[0]    = p->exec;
		argv[1]    = arg+1;

		while( arg != NULL ) {
			arg[0] = '\0';
			arg    = strchr(arg+2,' ');
			
			if( arg != NULL ) {
				argv[argcnt] = arg+1;
				argcnt++;
			}
		}
	}
}

static void do_parent_work(struct params *p) {
	close( p->fd[STDOUT_FILENO] );
	p->fd[STDOUT_FILENO] = -1;
		
	FILE *input = fdopen( p->fd[STDIN_FILENO] , "r");
	p->data     = (struct record *) malloc( sizeof(struct record) );
	memset( p->data, 0, sizeof(struct record) );
	p->records  = 0;	

	struct record *cur = p->data;
	char *in = NULL;
	do {
		cur->next = NULL;
		in = fgets(cur->data,BUFFER_SIZE,input);
		
		if( stopSignal == 1 ) break;		

		if( in != NULL ) {
			cur->next = (struct record *) malloc( sizeof(struct record) );
			memset( cur->next , 0 , sizeof(struct record) );

			p->records++;
			cur = cur->next;
		}
	} while( in != NULL );

	int status = 0;
	if( waitpid( p->pid, &status, 0) == -1 ) {
		bail_out(EXIT_FAILURE,"waitpid");
	}

	if( WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS ) {
		bail_out(EXIT_FAILURE,"child crashed!");
	}
}

static void print_usage() {
	(void) fprintf( stdout , "%s: <command1> <command2>", progname );
	exit( EXIT_FAILURE );
}

static void parse_arguments(struct options *opts,int argc,char *argv[]) {
	if( argc == 0 ) {
		bail_out(EXIT_FAILURE,"Unable to parse commandline!");
	}
	progname = argv[0];

	if( argc < 3 || argv[1][1] == '-' || argv[2][1] == '-' || argc > 3 ) {
		print_usage();
	}

	opts->program1 = argv[1];
	opts->program2 = argv[2];
}

static void free_resources() {
	if( env != NULL ) {
		free_params( &env->p1 );
		free_params( &env->p2 );
	
		if( env->data != NULL ) {
			free( env->data );	
		}

		free( env );
	}
}

static void free_records(struct record *head) {	

	struct record *cur = head;
	while( cur != NULL ) {
		struct record *next = cur->next;
		free( cur );

		cur = next;
	}
}

static void free_params(struct params *p) {
	
	if( p->fd[STDIN_FILENO] != -1 ) {
		close( p->fd[STDIN_FILENO] );
		p->fd[STDIN_FILENO] = -1;
	} 
	if( p->fd[STDOUT_FILENO] != -1 ) {
		close( p->fd[STDOUT_FILENO] );
		p->fd[STDOUT_FILENO] = -1;
	}

	if( p->data != NULL ) {
		free_records( p->data );
	}
	if( p->exec != NULL ) {
		free( p->exec );
	}
}

static void bail_out(int exitcode,const char *fmt, ...) {
	va_list arguments;

	(void) fprintf( stderr , "%s: ", progname);
	if( fmt != NULL ) {
		va_start(arguments,fmt);
		(void) vfprintf( stderr , fmt, arguments);
		va_end(arguments);
	}

	if( errno != 0 ) {
		(void) fprintf( stderr , ": %s", strerror(errno) );
	}

	(void) fprintf( stderr , "\n" );
	free_resources();
	exit( exitcode );
}

static int strcnt(const char * const str,int c) {
	int i=0;
	for(int p=0; str[p] != '\0'; p++) {
		if( str[p] == c ) {
			i++;
		}
	}
	
	return i;
}

static void signal_handler(int signal) {
	stopSignal = 1;

	if( env->p1.fd[STDIN_FILENO] != -1 ) { close(env->p1.fd[STDIN_FILENO]); env->p1.fd[STDIN_FILENO] = -1; }
	if( env->p1.fd[STDOUT_FILENO] != -1 ) { close(env->p1.fd[STDOUT_FILENO]); env->p1.fd[STDOUT_FILENO] = -1; }
	if( env->p2.fd[STDIN_FILENO] != -1 ) { close(env->p2.fd[STDIN_FILENO]); env->p2.fd[STDIN_FILENO] = -1; }
	if( env->p2.fd[STDOUT_FILENO] != -1 ) { close(env->p2.fd[STDOUT_FILENO]); env->p2.fd[STDOUT_FILENO] = -1; }
}
