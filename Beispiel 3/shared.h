/*
 * This file contains all the shared elements of the client and server. 
 * Since the server may only send small portions of data to the client, 
 * the only thing common between them is the protocol (structure) and 
 * the different flags. All functions are specific to the internal 
 * handling of the parts.
 *
 *
 * @brief A banking client / server system
 * @author Raphael Ludwig (e1526280)
 * @version 1
 * @date 13 Dez. 2016
 */
#include <stdbool.h>
#include <stdint.h>
#include <semaphore.h>



/* ==== SHM and SEM Definitions ==== */

/** @brief We have to prefix shm and sem with the student id */
#define PREFIX "1526280"

/** @brief The name of the SHM where data should be written / read */
#define BANK_CONNECT_SHM PREFIX "_bankmgr"

/** @brief The name of the sem which marks the critical section around a client server communication */
#define BANK_CONNECT_CLIENT_LOCK PREFIX "_client_lock"

/** @brief The name of the sem which signals the server when the client has written data */
#define BANK_CONNECT_SERVER_LOCK PREFIX "_server_lock"

/** @brief The name of the sem which signal the client that the server finished processing */
#define BANK_CONNECT_READ_LOCK PREFIX "_read_lock"



/* ==== Protocol Definition ==== */
/* Here all possible actions are listed and
 * described for the proctol. If any other 
 * value is written to the action field in 
 * the BankMgr structure the server might panic
 * because the action is not known!
 */

/** @brief An Action value which tells the Server that the Client wants to connect (inital connection) */
#define BANK_ACTION_CONNECT (1)

/** @brief An Action value which tells the Server that the Client wants to disconnect */
#define BANK_ACTION_DISCONNECT (2)

/** @brief An Action value which tells the Server that the Client does want to reciev the current balance of his account */
#define BANK_ACTION_GET_BALANCE (3)

/** @brief An Action value which tells the Server that the Client does want to withdraw money (stored in data field) */
#define BANK_ACTION_MONEY_WITHDRAW (4)

/** @brief An Action value which tells the Server that the Client does want to deposit money (stored in data field) */
#define BANK_ACTION_MONEY_DEPOSIT (5)

/** @brief An Action value which tells the Server that the Client does want to transfer money (stored in data field) to an other account */
#define BANK_ACTION_MONEY_TRANSFER (6)

/** @brief This Action must only be used by the Server, the client must check if this is present in the shared memory otherwise the client might deadlock itself */
#define BANK_CLOSED (99)


/** @brief A structure which represents the Data in the shared memory segement used to communicate with the server */
struct BankMgrData {
	/** @brief an action value what the client / server might want to communicate */	
	uint8_t action;
	/** @brief the account number which does want to act */
	int accountNr;
	/** @brief the secret which is required to authenticate the action */	
	int secret;
	/** @brief another account number which might be relevant to the action */	
	int otherAcc;
	/** @brief any additional data (most times money which is transfered) */	
	int data;
	
	/** @brief how the operation tured out, false means there was an error on the other side while processing */	
	bool outcome;
};


/** @brief A structure which does store all relevant locks which are needed by the client and server */
struct BankServer {
	/** @brief The client_lock is a mutex which protects the shared memory for multiple clients */	
	sem_t *client_lock;
	/** @brief This semaphor is used to signal the server that there is data for him to process */
	sem_t *server_lock;
	/** @brief This sempahor is used to signal the client that there is data from the server */
	sem_t *readBack_lock;
	
	/** @brief This FD is the FD to the shared memory segement */
	int bankmgr;
	/** @brief a pointer to the shared memory segment which was mapped with mmap*/
	struct BankMgrData *shared;
};
