/* 
 * This Program is the client to the Mastermind Task 1B
 *
 * @author Raphael Ludwig (e1526280)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <errno.h>
#include <limits.h>
#include <netdb.h>
#include <time.h>
#include <limits.h>

/* === Constants === */

#define SLOTS (5)
#define COLORS (8)

#define READ_BYTES (1)
#define WRITE_BYTES (2)
#define SHIFT_WIDTH (3)
#define COLOR_MASK (7)
#define PARITY_BIT (15)
#define PARITY_ERR_BIT (1 << 6)
#define GAME_LOST_ERR_BIT (1 << 7)

#define EXIT_PARITY_ERROR (2)
#define EXIT_GAME_LOST (3)
#define EXIT_MULTIPLE_ERRORS (4)

/* === Macros === */
#ifdef ENDEBUG
#define DEBUG(...) do { fprintf(stderr,__VA_ARGS__); } while(0)
#else
#define DEBUG(...)
#endif

/* === Type Definitions === */

struct opts {
	char *portno;
	char *server;
};

enum color {
	beige = 0,
	darkblue,
	green,
	orange,
	red,
	black,
	violet,
	white
};

/* === Global Variables === */

/* Name of Program */
static const char *progname = "client";

/* File descriptor to the socket to the server */
static int sockfd = -1;

/* Is signal handling needed? */
volatile sig_atomic_t quit = 0;

/* Array which holds the population for the generic algorithm */
static uint16_t *population    = NULL;
static uint16_t *newpopulation = NULL;

static int populationSize = 0;
static int newpopulationSize = 0;

/* === Prototypes === */

/**
 * @brief Parses the command line arguments
 * @param argc Number of Arguments
 * @param argv The Arguments
 * @param options Struct which does hold the extracted options
 */
static void parse_arguments(int argc, char **argv, struct opts *options);

/**
 * @brief Connects the client to the server and returns the file descriptor of the socket
 * @param serverAddr the server address as a string
 * @param portNo the server port as a string
 * @return a file descriptor of the socket
 */
static void connect_to_server(char *serverAddr,char *portNo);

/**
 * @brief This function calculates the parity bit in the request
 * @param request the server request
 */
static void calculate_parity(uint16_t *request);

/**
 * @brief prints an error message and quits the program with exitcode 
 * @param exitcode an exit code which is returned by the program
 * @param fmd a format string for parameters
 */
static void bail_out(int exitcode, const char *fmt, ...);

/**
 * @brief frees global resources in the program
 */
static void free_resources(void);

/**
 * @brief signal handler for SIGINT
 */
static void signal_handler(int sig);

/**
 * @breif This function is the same as in the server, it does compute the result
 * @param req_guess current guess of colors
 * @param req_secret the current secret of colors
 * @param resp output parameter where the result is stored, same way as it would come from the server
 */
static  uint8_t compute_answer(uint16_t req_guess, uint16_t req_secret);

/**
 * @brief a pow function for ints
 * @param base base for power function
 * @param exp exp for the power function
 * @return the result of the power function
 */
static int ipow(int base, int exp);

/**
 * @brief this function does calculate the color character to the specific color code
 * @param color the color code
 * @return character which represents the color code
 */
static char colorToChar(uint16_t color);

/**
 * @brief this function does create the population for guessing
 * @param request the request which was send before this function was called
 * @param response the response of the request from the server
 */
static void generatePopulation(uint16_t request, uint8_t response);

/**
 * @brief Main Method of the Client 
 * @param argc Number of Arguments
 * @param argv The arguments
 * @return an exit code
 */
int main(int argc,char **argv) {
	DEBUG("Client started with DEBUG Build!\n");

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
	
	/* Parse Arguments: */
	struct opts options;
	parse_arguments(argc, argv, &options);
	
	/* connect to server */
	connect_to_server(options.server, options.portno);
	srand( time(NULL) );

	uint8_t response;
	uint16_t request;

	/* inital guess */
	request = beige 
		| (darkblue << (SHIFT_WIDTH * 1))
		| (green << (SHIFT_WIDTH * 2))
		| (orange << (SHIFT_WIDTH * 3))
		| (red << (SHIFT_WIDTH * 4))
		;

	/* inital seed */
	population = (uint16_t *) malloc(((sizeof(uint16_t)) * (ipow(COLORS, SLOTS))));
	newpopulation = (uint16_t *) malloc(((sizeof(uint16_t)) * (ipow(COLORS, SLOTS))));
	populationSize = ipow(COLORS, SLOTS);
	
	for (int i = 0; i < populationSize; i++) {
		population[i] = i;
	}

	/* enter guess loop */
	int rounds = 0;
	int ret    = EXIT_SUCCESS;
	while( quit == 0 ) {
		rounds++;

		request &= 0x7FFF;
		DEBUG("Send Data: 0x%4x ", request);

		/* send data */
		calculate_parity( &request );	
		
		DEBUG(" / 0x%4x  ",request);
		DEBUG("%d%c%c%c%c%c\t",
			  ( request >> PARITY_BIT ) & 0x1,
			  colorToChar(request & COLOR_MASK),
			  colorToChar((request >> (SHIFT_WIDTH * 1)) & COLOR_MASK),
			  colorToChar((request >> (SHIFT_WIDTH * 2)) & COLOR_MASK),
			  colorToChar((request >> (SHIFT_WIDTH * 3)) & COLOR_MASK),
			  colorToChar((request >> (SHIFT_WIDTH * 4)) & COLOR_MASK)
			  );
		
		if( send( sockfd , &request, WRITE_BYTES, 0 ) < 0 ) {
			bail_out(EXIT_FAILURE,"send");
		}
		
		/* get data */
		if( recv( sockfd , &response, READ_BYTES, 0 ) < 0 ) {
			bail_out(EXIT_FAILURE,"recv");
		}

		//If SIGINT was recieved fast break the loop
		if (quit == 1) {
			break;
		}

		if ( (response & GAME_LOST_ERR_BIT) > 0 ) {
			(void) fprintf( stdout , "Game lost\n" );
			quit = 1;
			ret  = EXIT_GAME_LOST;
			break;
		}
			
		if( (response & PARITY_ERR_BIT) > 0 ) {
			(void) fprintf( stderr , "Parity Error\n" );
			quit = 1;
			ret  = EXIT_PARITY_ERROR;
			break;
		}

		uint8_t red = response & COLOR_MASK;
		uint8_t white = (response >> SHIFT_WIDTH) & COLOR_MASK;

		if( red == SLOTS ) {
			(void) fprintf( stdout , "Gewonnen!\nRunden: %d\n", rounds );
			quit = 1;
			break;		
		}

		DEBUG("Got Data: 0x%2x %dw %dr\n",response, white, red);

		generatePopulation(request, response);

		if (populationSize == 0) {
			DEBUG("Generate new Population,ran out of choices ...\n");

			populationSize = ipow(COLORS, SLOTS);
			for (int i = 0; i < populationSize; i++) {
				population[i] = i;
			}

			generatePopulation(request, response);
		}

		if (populationSize > 0) {
			request = population[rand() % populationSize];
		} else {
			request = rand();
		}
	}

	if ( (response & GAME_LOST_ERR_BIT) > 0 && (response & PARITY_ERR_BIT) > 0) {
		ret = EXIT_MULTIPLE_ERRORS;
	}

	free_resources();
	return ret;
}

/* === Implementation === */

static void generatePopulation(uint16_t request, uint8_t response) {
	response &= 0x3F; //mask parity and error bit 
	request &= 0x7FFF; //mask parity bit

	/* delete all no possible states: */
	memset(newpopulation, 0, (sizeof(uint16_t)) * (ipow(COLORS, SLOTS)));
	uint8_t result;

	for (int i = 0; i < populationSize; i++) {
		result = compute_answer(population[i], request);

		if (result == response) {
			/*DEBUG("Add: %c%c%c%c%c\t",
				  colorToChar(population[i] & COLOR_MASK),
				  colorToChar((population[i] >> (SHIFT_WIDTH * 1)) & COLOR_MASK),
				  colorToChar((population[i] >> (SHIFT_WIDTH * 2)) & COLOR_MASK),
				  colorToChar((population[i] >> (SHIFT_WIDTH * 3)) & COLOR_MASK),
				  colorToChar((population[i] >> (SHIFT_WIDTH * 4)) & COLOR_MASK)
				  );

			DEBUG("Got Data: 0x%2x %dw %dr\n", result, (result >> SHIFT_WIDTH) & COLOR_MASK, result & COLOR_MASK);
			*/
			newpopulation[newpopulationSize] = population[i];
			newpopulationSize++;
		} /* else {
			DEBUG("NOT: %d%c%c%c%c%c\t",
				  colorToChar(population[i] & COLOR_MASK),
				  colorToChar((population[i] >> (SHIFT_WIDTH * 1)) & COLOR_MASK),
				  colorToChar((population[i] >> (SHIFT_WIDTH * 2)) & COLOR_MASK),
				  colorToChar((population[i] >> (SHIFT_WIDTH * 3)) & COLOR_MASK),
				  colorToChar((population[i] >> (SHIFT_WIDTH * 4)) & COLOR_MASK)
				  );

			DEBUG("Got Data: 0x%2x %dw %dr\n", result, (result >> SHIFT_WIDTH) & COLOR_MASK, result & COLOR_MASK);
		} */
	}

	uint16_t *t = population;
	population = newpopulation;
	newpopulation = t;

	populationSize = newpopulationSize;
	newpopulationSize = 0;
}

static void parse_arguments(int argc, char **argv, struct opts *options) {
	/* set real progname */
	if( argc > 0 ) {
		progname = argv[0];
	}

	if( argc != 3 ) {
		bail_out(EXIT_FAILURE,"Usage: %s <server-hostname> <server-port>",progname);
	}
	
	options->server = argv[1];
	options->portno = argv[2];
}

static void calculate_parity(uint16_t *request) {
	uint16_t parity = 0;

	for (int j = 0; j < PARITY_BIT; j++) {
		parity ^= ((*request) >> j) & 0x1;		
	}
	parity &= 0x1;

	(*request) |= (parity << PARITY_BIT);
}

static void connect_to_server(char *serverAddr,char *portNo) {
	struct addrinfo hints;
	struct addrinfo *ai, *aip;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family 	= AF_INET;
	hints.ai_socktype 	= SOCK_STREAM;

	if( getaddrinfo(serverAddr,portNo,&hints,&ai) < 0 ) {
		bail_out(EXIT_FAILURE, "getaddrinfo");
	}

	aip = ai;
	sockfd = socket(aip->ai_family, aip->ai_socktype, aip->ai_protocol);
	if( sockfd < 0 ) {
		freeaddrinfo(ai);
		bail_out(EXIT_FAILURE, "socket");
	}

	int ret = connect(sockfd,aip->ai_addr,aip->ai_addrlen);
	if( ret < 0 ) {
		freeaddrinfo(ai);
		bail_out(EXIT_FAILURE, "connect");
	}
	
	freeaddrinfo(ai);
	
	DEBUG("Socket: %d\n",sockfd);
	DEBUG("connect(): %d\n",ret);
}

static void bail_out(int exitcode, const char *fmt, ...) {
	va_list arguments;

	(void) fprintf( stderr, "%s: ", progname );
	if( fmt != NULL ) {
		va_start(arguments,fmt);
		(void) vfprintf( stderr, fmt, arguments );
		va_end(arguments);
	}

	if(errno != 0) {
		(void) fprintf( stderr, ": %s", strerror(errno) );
	}
	(void) fprintf( stderr, "\n" );
	
	free_resources();
	exit(exitcode);
}

static void free_resources(void) {
	DEBUG("free_resources\n");

	if( sockfd != -1 ) {
		close(sockfd);
	}

	if (population != NULL) {
		free(population);
	}

	if (newpopulation != NULL) {
		free(newpopulation);
	}
}

static void signal_handler(int sig) {
	quit = 1;
}

static uint8_t compute_answer(uint16_t req_guess, uint16_t req_secret) {
	int colors_left[COLORS];
	int secret[SLOTS];
	int guess[SLOTS];
	int j,red,white;
	uint8_t result;

	/* extract data from uint16_t */
	for (j = 0; j < SLOTS; ++j) {
		guess[j] = req_guess & COLOR_MASK;
		secret[j] = req_secret & COLOR_MASK;

		req_guess >>= SHIFT_WIDTH;
		req_secret >>= SHIFT_WIDTH;
	}

	/* marking red and white */
	(void)memset(&colors_left[0], 0, sizeof(colors_left));
	red = white = 0;
	for (j = 0; j < SLOTS; ++j) {
		/* mark red */
		if (guess[j] == secret[j]) {
			red++;
		} else {
			colors_left[secret[j]]++;
		}
	}
	for (j = 0; j < SLOTS; ++j) {
		/* not marked red */
		if (guess[j] != secret[j]) {
			if (colors_left[guess[j]] > 0) {
				white++;
				colors_left[guess[j]]--;
			}
		}
	}

	result = red;
	result |= (white << SHIFT_WIDTH);
	//No parity needed for internal calculation

	return ( result & 0x3F );
}

static int ipow(int base, int exp) {
	int result = 1;
	while (exp) {
		if (exp & 1)
			result *= base;
		exp >>= 1;
		base *= base;
	}

	return result;
}

static char colorToChar(uint16_t color) {
	switch (color) {
		case beige:     return 'b';
		case darkblue:  return 'd';
		case green:     return 'g';
		case orange:    return 'o';
		case red:       return 'r';
		case black:     return 's';
		case violet:    return 'v';
		case white:     return 'w';
		default:
			bail_out(EXIT_FAILURE,"Bad Color");
	}

	return '?';
}