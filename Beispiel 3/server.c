/**
 * @brief This is the Server Application for the Banking Task
 * @author Raphael Ludwig (e1526280)
 * @version 1
 * @date 25 Nov. 2016
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <signal.h>
#include <time.h>

/**
 * The shared.h Header does contain all 
 * definitions and constances which are
 * needed by the server and client
 */
#include "shared.h"


/* === Constants === */
/**
 * @breif The Name of the Server Mutex, this allows the
 *        server to detect and prevent more than one 
 *        running instance
 */
#define SEM_EXC (PREFIX "-BANKSERVER-EXCLUSIVE")


/* === Type Definitions === */

/**
 * @brief the account data of the server
 */
struct Account {
	/** @brief Account Number of the Bank account */
	int accountNr;
	/** @brief Balance of the current account */
	int balance;
	/** @brief Secret of the Account which is generated at CONNECT and deleted with DISCONNECT */
	int secret;
};

/**
 * @brief describes a linnked list of records which contain the account data for the server
 */
struct Record {
	/** @brief the account data */
	struct Account acc;
	/** @brief the pointer to the next element in the list */
	struct Record *next;
};

/** 
 * @brief a structure which does hold the internal and external data for the server
 */
struct Bank {
	/** @brief the main data of the server which is shared with the client */	
	struct BankServer server;
	/** @brief the internal list of accounts in the bank */
	struct Record *list;
	
	/** @brief a file from which the input data is read */
	FILE *inFile;
	/** @brief a file where the output data is written on closing the program */
	FILE *outFile;
	
	/** @brief the server mutex */
	sem_t *serverExSem;
};

/* === Globals === */

/**
 * @brief A global structure which does hold all the Data which is needed by the server
 */
static struct Bank *bank = NULL;

/**
 * @brief Variable which is set to 1 by the signal_handler if SIGINT or SIGTERM was recieved
 */
static sig_atomic_t stopSignal = 0;

/**
 * @brief The Base name of the Program which is displayed in the error message
 */
static const char *progname = "bankserver";

/** === Prototypes === */

/**
 * @brief initalizes the bank server, all shared resources and server exclusive ones
 * @param server the server structure which should be initalized
 */
static void init_bankserver(struct BankServer *server);

/**
 * @brief frees all resources, also shared ones, which where be allocated by the server
 */
static void free_resources(void);

/**
 * @brief the main loop which dispatches the actions recieved from the clients
 */
static void dispatch_action(void);

/**
 * @brief prints the usage to stdout
 */
static void print_usage(void);

/**
 * @brief handles the signals
 * @param signal the signal for the signal handler 
 */
static void signal_handler(int signal);

/**
 * @brief This Method parses the command line, results will be directly written in the global variable
 * @param argc number of arguments
 * @param argv the arguments form main
 */
static void parse_arguments(int argc,char *argv[]);

/**
 * @brief this function does install all the signal handlers which are needed
 */
static void install_signal_handlers(void);

/**
 * @brief with this function a CSV formatted file can be read 
 * @param f a file pointer to the file which should be read
 */
static void read_csv_table(FILE *f);

/**
 * @brief this function does extract a filed from a CSV formatted string
 * @param line the data of the line which should be extracted
 * @param num the field which should be extracted
 * @return a start pointer to the field in line
 */
static char *getfield(char* line, int num);

/**
 * @brief this method does search in the global structure for an Account
 * @param accountNr the account which should be searched for
 * @return a List element which contains the Account
 */
static struct Record *get_account(int accountNr);

/**
 * @brief with this method a CSV formatted file can be written
 * @param f a file ponter to the file which should be written
 */
static void write_csv_table(FILE *f);

/**
 * @brief this method processes the connect message from the client
 * @param r a pointer to the element which holds the Account
 */
static void dispatch_connect(struct Record *r);

/**
 * @brief this method processes the disconnect message from the client
 * @param r a pointer to the element which holds the Account
 */
static void dispatch_disconnect(struct Record *r);

/**
 * @brief this method processes the get_balance message from the client
 * @param r a pointer to the element which holds the Account
 */
static void dispatch_get_balance(struct Record *r);

/**
 * @brief this method processes the withdraw message from the client
 * @param r a pointer to the element which holds the Account
 */
static void dispatch_withdraw(struct Record *r);

/**
 * @brief this method processes the deposit message from the client
 * @param r a pointer to the element which holds the Account
 */
static void dispatch_deposit(struct Record *r);

/**
 * @brief this method processes the money_transfer message from the client
 * @param r a pointer to the element which holds the Account
 */
static void dispatch_transfer(struct Record *r);

/**
 * This function represents the entry point to the Bankserver. All resources
 * for the server will be created and then the server will be started.
 *
 * @brief main function of the server
 * @param argc argument counter of how many arguments
 * @param argv the actual agruments for this program
 * @return EXIT_SUCCESS or EXIT_FAILURE
 */
int main(int argc,char **argv) {
	bank = (struct Bank*) malloc( sizeof(struct Bank) );
	if( bank == NULL ) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	/* Zero out bank memory */	
	(void) memset( bank, 0, sizeof(struct Bank));
	srand(time(NULL));

	/** Set default error values */
	bank->server.client_lock 	= SEM_FAILED;
	bank->server.server_lock	= SEM_FAILED;
	bank->server.readBack_lock	= SEM_FAILED;
	bank->server.shared		= MAP_FAILED;
	bank->server.bankmgr		= -1;
		
	/** make server exclusive */
	bank->serverExSem = sem_open(SEM_EXC,O_CREAT | O_EXCL, 0600, 1);
	if( bank->serverExSem == SEM_FAILED ) {
		(void) fprintf( stderr , "Only one Instance can is allowed to run!\n" );
		(void) fprintf( stderr , "If your are sure that there is no server instance running or the previous instance crashed please delete the mutex: 'rm -f /dev/shm/%s*' \n", PREFIX ); 
		free(bank);
		exit(EXIT_FAILURE);
	}
	
	/** register free_resources function */
	if( atexit( free_resources ) != 0 ) {
		perror("atexit");
		exit(EXIT_FAILURE);
	}
		
	install_signal_handlers();
	parse_arguments(argc,argv);
	
	if( bank->inFile  == NULL ) bank->inFile  = stdin;
	if( bank->outFile == NULL ) bank->outFile = stdout;
	
	/** init all the shared memory and semaphores for the server */
	init_bankserver(&bank->server);
	
	/** read data for the bank either form STDIN or a file */
	read_csv_table( bank->inFile );

	/** enter dispatcher loop for actions */
	while( stopSignal == 0 ) {		
		dispatch_action();

	}

	//Write CSV File:
	write_csv_table( bank->outFile );
	
	// free_resources is called implicitly on return
	return EXIT_SUCCESS;	
}

static void dispatch_action(void) {
	/** Wait for a data from client */
	sem_wait(bank->server.server_lock);
	
	//Query user from account number
	struct Record *r = get_account(bank->server.shared->accountNr);
	if( r != NULL ) {
	
		//Do Auth if connection requests login
		if( bank->server.shared->action == BANK_ACTION_CONNECT ) dispatch_connect(r);
		else if( r->acc.secret == bank->server.shared->secret ) {
	
			/* check if money field is positive */
			if( bank->server.shared->data >= 0 ) {

				// if the account was verified than we proceed processing other actions
				switch( bank->server.shared->action ) {
					case BANK_ACTION_CONNECT: 		dispatch_connect(r);		break;
					case BANK_ACTION_DISCONNECT:		dispatch_disconnect(r);		break;
					case BANK_ACTION_GET_BALANCE:		dispatch_get_balance(r);	break;
					case BANK_ACTION_MONEY_WITHDRAW:	dispatch_withdraw(r);		break;
					case BANK_ACTION_MONEY_DEPOSIT: 	dispatch_deposit(r);		break;
					case BANK_ACTION_MONEY_TRANSFER:	dispatch_transfer(r);		break;
					case 0:  /** Shut down server befor a single request was send ... */	break;
					default: /** either client wrote something we don't understand or memory is fuzzy */
						(void) fprintf( stderr , "Unkown ACTION in SHM detected, server is starting to PANIC!\n");
						exit(EXIT_FAILURE);
				} 

			} else bank->server.shared->outcome = false; // Negative money field
		} else bank->server.shared->outcome = false; //Tell client that something went wrong ...	
	} else bank->server.shared->outcome = false; // Account was not found!

	/** Signal Client that we finished processing data */
	sem_post(bank->server.readBack_lock);
}

static void dispatch_connect(struct Record *r) {
	r->acc.secret = rand();
	bank->server.shared->secret = r->acc.secret;
	bank->server.shared->outcome = true;			
}

static void dispatch_disconnect(struct Record *r) {
	r->acc.secret = -1;
	bank->server.shared->outcome = true;
}

static void dispatch_get_balance(struct Record *r) {
	bank->server.shared->data = r->acc.balance;
	bank->server.shared->outcome = true;
}

static void dispatch_withdraw(struct Record *r) {
	if( r->acc.balance - bank->server.shared->data >= 0 ) {
		r->acc.balance -= bank->server.shared->data;
		bank->server.shared->outcome = true;				
	} else {
		bank->server.shared->outcome = false;
	}
}

static void dispatch_deposit(struct Record *r) {
	r->acc.balance += bank->server.shared->data;
	bank->server.shared->outcome = true;
}

static void dispatch_transfer(struct Record *r) {
	struct Record *other = get_account(bank->server.shared->otherAcc);

	if( other != NULL && r->acc.balance - bank->server.shared->data >= 0 ) {					
		r->acc.balance -= bank->server.shared->data;
		other->acc.balance += bank->server.shared->data;
		bank->server.shared->outcome = true;
	} else {
		bank->server.shared->outcome = false;
	}
}


static void init_bankserver(struct BankServer *server) {
	/** Aquire all the locks and shm segments */
	server->client_lock = sem_open(BANK_CONNECT_CLIENT_LOCK,O_CREAT | O_EXCL, 0660, 1);
	if( server->client_lock == SEM_FAILED ) {
		perror("client_lock");
		exit(EXIT_FAILURE);
	}
	
	server->server_lock = sem_open(BANK_CONNECT_SERVER_LOCK,O_CREAT | O_EXCL, 0660, 0);
	if( server->server_lock == SEM_FAILED ) {
		perror("server_lock");
		exit(EXIT_FAILURE);
	}

	server->readBack_lock = sem_open(BANK_CONNECT_READ_LOCK,O_CREAT | O_EXCL, 0660, 0);
	if( server->readBack_lock == SEM_FAILED ) {
		perror("read_lock");
		exit(EXIT_FAILURE);
	}
	
	server->bankmgr = shm_open(BANK_CONNECT_SHM,O_CREAT | O_EXCL | O_RDWR, 0660);
	if( server->bankmgr == -1 ) {
		perror("bankmgr shm");
		exit(EXIT_FAILURE);
	}

	/** extend size of memory to BankMgrData size */
	if( ftruncate(server->bankmgr, sizeof(struct BankMgrData) ) == -1) {		
		perror("ftruncate");
		exit(EXIT_FAILURE);	
	}
	
	/** map shared memory object */
	server->shared = mmap(NULL, sizeof(struct BankMgrData),PROT_READ | PROT_WRITE, MAP_SHARED,server->bankmgr, 0);
	if( server->shared == MAP_FAILED ) {
		perror("mmap");
		exit(EXIT_FAILURE);
	}
	
	/** zero memory so we don't read stupid things */
	(void) memset(server->shared,0,sizeof(struct BankMgrData));
}

static void free_resources(void) {
	//We do ignore most of the errors which are raised while closing ...
	(void) fprintf( stdout , "free_resources()\n" );
	
	/** write BANK_CLOSED to shared memory so clients do not hang */
	if( bank->server.shared != MAP_FAILED ) {
		bank->server.shared->action = BANK_CLOSED;
	}
	
	/** UN-Map the SHM */
	if( bank->server.shared != MAP_FAILED ) {
		(void) munmap(bank->server.shared, sizeof(struct BankMgrData));
		bank->server.shared = MAP_FAILED;
	}	
	
	/** Close the FD of SHM and unlink it -> cached data might remain in the client */
	if( bank->server.bankmgr != -1 ) {
		(void) close(bank->server.bankmgr);
		bank->server.bankmgr = -1;
	}
	(void) shm_unlink(BANK_CONNECT_SHM);
	
	/** Clean up all the sem locks */
	if( bank->server.client_lock   != SEM_FAILED ) { (void) sem_close(bank->server.client_lock  ); bank->server.client_lock = SEM_FAILED; }
	if( bank->server.server_lock   != SEM_FAILED ) { (void) sem_close(bank->server.server_lock  ); bank->server.server_lock = SEM_FAILED; }
	if( bank->server.readBack_lock != SEM_FAILED ) { (void) sem_close(bank->server.readBack_lock); bank->server.readBack_lock = SEM_FAILED; }

	(void) sem_unlink(BANK_CONNECT_CLIENT_LOCK);
	(void) sem_unlink(BANK_CONNECT_SERVER_LOCK);
	(void) sem_unlink(BANK_CONNECT_READ_LOCK);
	
	/** Delete all the internal data */
	while( bank->list != NULL ) {
		struct Record *r = bank->list;
		bank->list = r->next;
		
		free(r);
	}
	
	/** Close any open FD */
	if( bank->inFile  != NULL ) { fclose(bank->inFile);  bank->inFile  = NULL; }
	if( bank->outFile != NULL ) { fclose(bank->outFile); bank->outFile = NULL; } 
	
	/** Close and unlink the mutex, now any other server might start ... */
	(void) sem_close(bank->serverExSem);
	(void) sem_unlink(SEM_EXC);

	if( bank != NULL ) { free(bank); bank = NULL; }
}

static void parse_arguments(int argc,char *argv[]) {
	if( argc == 0 ) {
		(void) fprintf( stderr , "Unable to parse commandline!\n");
		exit(EXIT_FAILURE);
	}

	if( argc >= 2 ) {	
		if( argv[1][0] != '-' ) {	
			bank->inFile = fopen(argv[1],"r");
		
			if( bank->inFile == NULL ) {
				perror("fopen");
				exit(EXIT_FAILURE);
			}
		} else {
			(void) fprintf( stderr , "Expected file path but got argument!\n" );
			print_usage();
			exit(EXIT_FAILURE);
		}
	}

	if( argc == 3 ) {
		if( argv[2][0] != '-' ) {		
			bank->outFile = fopen(argv[2],"w");
		
			if( bank->outFile == NULL ) {
				perror("fopen");
				exit(EXIT_FAILURE);
			}
		} else {
			(void) fprintf( stderr , "Expected file path but got argument!\n" );
			print_usage();
			exit(EXIT_FAILURE);
		}
	}

	if( argc > 3 ) 
		print_usage();

	progname = argv[0];
}

static void signal_handler(int signal) {
	stopSignal = 1;
}

static void install_signal_handlers(void) {
	struct sigaction s;
	
	s.sa_handler = signal_handler;
	s.sa_flags   = 0;
	
	if( sigfillset(&s.sa_mask) < 0 ) {
		perror("sigfillset");
		exit(EXIT_FAILURE);
	}

	if( sigaction(SIGINT,&s,NULL) < 0 ) {
		perror("sigfillset");
		exit(EXIT_FAILURE);
	}

	if( sigaction(SIGTERM,&s,NULL) < 0 ) {
		perror("sigfillset");
		exit(EXIT_FAILURE);
	}

}

static void print_usage(void) {
	(void) fprintf( stdout , "%s: [account-in [account-out]]\n", progname );
	exit( EXIT_FAILURE );
}

static void read_csv_table(FILE *f) {
	const int BUFFER_SIZE = 32;
	char line[BUFFER_SIZE];
		
	struct Record *current = (struct Record *) malloc( sizeof(struct Record) );
	current->acc.accountNr =  0;
	current->acc.secret    = -9;
	current->next	       = NULL;
	
	bank->list = current;
	while( fgets(line, BUFFER_SIZE, f) != NULL ) {
		//(void) fprintf( stdout , "Read: %s\n",line);
        	char* tmp0 = strdup(line);
		char* tmp1 = strdup(line);
		
		(void) getfield(tmp0,0);
		char *balance = getfield(tmp1,1);
		
		struct Record *r = (struct Record *) malloc( sizeof(struct Record) );
		r->acc.accountNr = strtol(tmp0   ,NULL,10); //reads data till first invalid char
		r->acc.balance   = strtol(balance,NULL,10);
		r->acc.secret    = -1;
		r->next		 = NULL;
	
		current->next    = r;
		current          = r;
		
		// Free strdup generated data
        	free(tmp0);
		free(tmp1);
    	}

	//(void) fprintf( stdout , "Finished ...\n");
}

static char *getfield(char* line, int num) {
	char *elem;


	for( elem = strtok(line, ";"); elem && *elem; ) {
		elem = strtok(NULL, ";\n");

		if (!--num)
	    		return elem;
	}

	return NULL;
}

static struct Record *get_account(int accountNr) {
	struct Record *current = bank->list;
	while( current != NULL ) {
		if( current->acc.accountNr == accountNr ) 
			return current;
		
		current = current->next;
	}

	return NULL;
}

static void write_csv_table(FILE *f) {
	struct Record *current = bank->list;

	//Skip the 0 Account which is generated by the Bank server automatically
	if( current != NULL ) current = current->next;
	
	while( current != NULL ) {
		(void) fprintf( f, "%d;%d\n", current->acc.accountNr, current->acc.balance );
		current = current->next;
	}
}
