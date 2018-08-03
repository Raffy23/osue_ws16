/*
 * A Program which reads lines from stdin and checks if they are palindroms 
 *
 * @author Raphael Ludwig (e1526280)
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>

/* === Constants === */

#define FLAG_IGNORE_SPACE ('s')
#define FLAG_IGNORE_CASE  ('i')
#define END_OF_OPTS (-1)
#define MAX_INPUT_LEN (40)
#define SPACE (' ')

/* === Global Variables === */

/*
 * @brief Global Variable which indicates if the porgram should continue to read from stdin
 */
static volatile sig_atomic_t readFromInput = true;


/* === Prototypes === */

/*
 * @brief Prints the Usage of the Programm
 * @param command The name of the command (should be argv[0]) 
 */
static void printUsage(const char* const command);

/*
 * @brief 
 * 	parses the program agruments and sets the fales ignoreSpace and ignoreCase
 *      this function may print to stderr if an error occurs!
 *
 * @param argc the argument count from the commmand line
 * @param argv the argument strings from the command line
 * @param ignoreSpace a pointer to a boolean which is set to true if ignoreSpace flag was read
 * @param ignoreCase a pointer to a boolean which is set to true if ignoreCase flag was read
 *
 * @return EXIT_SUCCESS if parsing was successfull otherwise EXIT_FAILURE
 */
static int parseArguments(const int argc,char** argv,bool *ignoreSpace,bool *ignoreCase);

/*
 * @brief this function does check of the give string (input) is a palindrom or not
 *
 * @param input a null terminated string which should be checked if it's a palindrom
 * @param ignoreSpace true if spaces should be ignored
 * @param ignoreCase true if the case should be ignored
 *
 * @return true if string is palindrom otherwise not
 */
static bool isStringPalindrom(const char* input, const bool ignoreSpace, const bool ignoreCase);

/**
 * @brief handles the exit signals from the console 
 * @param signal the signal which is recieved by the program
 */ 
static void signalHandler( int signal );


/* === Implementations === */

/*
 * @brief Entry point of the Programm 
 * @param argc argument count from the command line
 * @param argv argument string from the command line
 */
int main( int argc , char** argv ) {
	
	/* these flags might get set by the parseArguments function */
	bool ignoreSpace = false;
	bool ignoreCase  = false;


	if( parseArguments( argc ,argv , &ignoreSpace , &ignoreCase ) == EXIT_FAILURE ) {
		exit( EXIT_FAILURE );
	}

	/* install signal handler (needed for endless input) */
	struct sigaction sigact;
	sigact.sa_handler = signalHandler;
	sigemptyset( &sigact.sa_mask );

	if( sigaction( SIGINT , &sigact , NULL ) == -1 ) {
		( void ) fprintf( stderr , "Error: Unable to install SIGINT Handler!\n" );
		exit( EXIT_FAILURE );
	}
	

	/* reserve memory for the string */
	char* input = (char*) calloc( MAX_INPUT_LEN + 1 + 1, sizeof(char) );
	if( input == NULL ) {
		( void ) fprintf( stderr , "Error: Out of Memory!\n" );
		exit( EXIT_FAILURE );
	}


	/* enter endless loop until CTRL-C or something else happends */
	while( readFromInput ) {
		( void ) memset( input , 0 , sizeof(char) ); // Clear memory 
		

		/* read chars from stdin */
		int inputChar = EOF;
		int currentInput = 0;
		do {
			inputChar = fgetc( stdin );
			if( inputChar == EOF ) break;

			input[ currentInput ] = (char)inputChar;
			currentInput++;

		} while( inputChar != '\n' && currentInput <= MAX_INPUT_LEN );

		
		/* check if fgetc did not even read a single char from stdin -> Ctrl-D ? */
		if( currentInput <= 0 && inputChar == EOF ) {
			break;
		}

		/* strip remaining \n from string */
		input[ --currentInput ] = '\0';
		

		/* check if we had to use the overflow char */
		if( currentInput >= MAX_INPUT_LEN ) {
			( void ) fprintf( stdout , "Only up to 40 Characters are supported!" );
			continue;
		}
		
		/* fgets did read a 0 char string ... will ignore it */
		if( currentInput == 0 ) {
			continue;
		}

		/* check if input is a palindrom */
		if( isStringPalindrom( (const char*)input , ignoreSpace , ignoreCase ) ) {
			( void ) fprintf( stdout , "%s is a palindrom\n" , input );
		} else {
			( void ) fprintf( stdout , "%s isn't a palindrom\n" , input );
		}
		
	}

	/* clean up */
	if( input != NULL ) {
		free( input );
		input = NULL;
	}

	return EXIT_SUCCESS;
}


static bool isStringPalindrom(const char* input, bool ignoreSpace, bool ignoreCase) {

	int length = strlen( input ) - 1;
	int offsetBegin = 0;
	int offsetEnd   = 0;
	
	/* Loop halve of the string and compare these positions */
	for( int i=0; i<=length/2 ; i++ ) {

		/* due ignoreSpaces flag there might be a offset due spaces */
		char begin = input[ i+offsetBegin      ];
		char end   = input[ length-i-offsetEnd ];

		/* do check with spaces -> set offset */
		if( ignoreSpace && begin == SPACE ) {
			 i--;	
			 offsetBegin++;
			 continue;
		}
		if( ignoreSpace && end == SPACE ) {
			i--;
			offsetEnd++;
			continue;
		}
		
		/* convert to lower if case should be ignored*/
		if( ignoreCase ) {
			begin = tolower( begin );
			end   = tolower( end );
		}


		/* compare and return false if chars do not match */
		if( begin != end ) {
			return false;
		}

	}

	return true;
}

static void printUsage(const char* const command) {
	( void ) fprintf( stderr , "Usage: %s [-%c] [-%c]\n"
				   "-%c\t\tIgnores spaces in the input\n"
				   "-%c\t\tIgnores character case in the input\n"
				   , command , FLAG_IGNORE_CASE , FLAG_IGNORE_SPACE
				   , FLAG_IGNORE_SPACE
				   , FLAG_IGNORE_CASE
			);
}

static int parseArguments(const int argc, char** argv, bool *ignoreSpace, bool *ignoreCase) {


	/* parse the actual option arguments */
	int opt = END_OF_OPTS;
	while( (opt=getopt(argc,argv,"si")) != END_OF_OPTS ) {
		switch( opt ) {
			case FLAG_IGNORE_CASE:
				if( *ignoreCase == true ) {
					( void ) fprintf( stderr , "Error: -%c was specified more than once\n" , FLAG_IGNORE_CASE );
					printUsage( argv[0] );
					return  EXIT_FAILURE;
				}

				*ignoreCase = true;
				break;
			
			case FLAG_IGNORE_SPACE:
			        if( *ignoreSpace == true ) {
				        ( void ) fprintf( stderr , "Error: -%c was specified more than once\n" , FLAG_IGNORE_SPACE );
				        printUsage( argv[0] );
				        return  EXIT_FAILURE;
			        }

			        *ignoreSpace = true;
		         	break;
			
			//case '?': /* this case does not have any use -> equals default case */ 					
			default: /* error unknown option */
				( void ) fprintf( stderr , "Error: Unknown argument: %s\n", argv[ optind-1 ] );
				printUsage( argv[0] );
				return EXIT_FAILURE;
		}
	}

	/* check for non option arguments */
	if( argc > optind ) {
		( void ) fprintf( stderr , "Error: there are more arguments than expected!" );
		printUsage( argv[0] );
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

static void signalHandler(int signal) {
	/* no extra signal handling required, will just exit input loop */
	readFromInput = false;
}
