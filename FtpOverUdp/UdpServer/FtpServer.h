/*************************************************************************************
*								 File Name	: Server.h		   			 	         *
*	Usage : Handles Client request for Uploading, downloading and listing of files   *
**************************************************************************************/
#ifndef SER_TCP_H
#define SER_TCP_H

#pragma comment(lib, "Ws2_32.lib")
#define HOSTNAME_LENGTH 20
#define RESP_LENGTH 40
#define FILENAME_LENGTH 20
#define REQUEST_PORT 5001
#define BUFFER_LENGTH 256
#define MAXPENDING 10
#define MSGHDRSIZE 12
#define SEQUENCE_RANGE 2

#include <queue>

/* Type of Messages */
typedef enum
{
	HANDSHAKE = 1,
	COMPLETE_HANDSHAKE = 2,
	REQ_LIST = 5,
	REQ_GET = 6,
	RESP = 10,
	RESP_ERR = 12,
	PUT = 15,
	TERMINATE = 20
} Type;

typedef enum
{
	Initialized,
	HandshakeStarted,
	ReceivingRequest,
	Sending,
	Terminated
} ThreadState;

/* Request message structure */
typedef struct
{
	char hostname[HOSTNAME_LENGTH];
	char filename[FILENAME_LENGTH];
} Req;

/* Response message structure */
typedef struct
{
	char response[RESP_LENGTH];
} Resp;

/* Message format used for sending and receiving datas */
typedef struct
{
	Type type;
	int  length;
	int  sequenceNumber;
	char buffer[BUFFER_LENGTH];
} Msg;

/* FtpServer Class */
class FtpServer
{
	private:
		int serverSock;							/* Socket descriptor for server and client*/
		struct sockaddr_in ClientAddr;			/* Client address */
		struct sockaddr_in ServerAddr;			/* Server address */
		unsigned short ServerPort;				/* Server port */
		unsigned short nextServerPort;			/* Socket for next worker thread */
		int clientLen;							/* Length of Server address data structure */
		char serverName[HOSTNAME_LENGTH];		/* Server Name */

	public:
		FtpServer();
		~FtpServer();
		void start();							/* Starts the FtpServer */
};

/* FtpThread Class */
class FtpThread : public Thread
{
	private:
		// Fields
		int serverIdentifier;							/* Random number to identify the server in the 3-way handshake */
		int inPort;										/* Initial, server-wide socket */
		ThreadState currentState;						/* Indicates the current state of the server, as defined by the requests received */
		SOCKET thrdSock;								/* Thread-specific socket */
		struct sockaddr_in addr;						/* Address */
		struct sockaddr_in clientAddr;					/* Client address */
		int addrLength;									/* Length of addr field */
		int currentSequenceNumber;						/* The sequence number (updated AFTER processing each request, as required) */
		std::string filesDirectory;						/* */
		std::queue<char*> payloadData;					/* The data to be sent. Until ACK is received, the previously sent chunk is kept at the top. */
		Msg* curRqt;									/* The latest received request */
		
		// Methods
		Msg* msgGet(SOCKET, struct sockaddr_in);		/* Gets a request message */
		void handleCurrentMessage();					/* Decide what response (if any) is appropriate. */
		int msgSend(int , Msg*);						/* Send the response */
		bool isHandshakeCompleted();					/* Determines whether the final handshake message is addressed to the correct server */
		bool tryLoadFile();								/* Retrieves the file in curRqt, if possible. Returns true if the file is found & loaded, or false otherwise. */
		void loadDirectoryContents();					/*  */

		// Message creation
		Msg* createServerHandshake();					/* Send 2nd handshake */
		Msg* getNextChunk();							/* Wraps the current payload inside a Msg object */
		Msg* getErrorMessage(const char*);				/* Wraps an error message inside a Msg object */
	public:
		FtpThread(int serverPort):inPort(serverPort) { curRqt = NULL; serverIdentifier = rand(); filesDirectory = "files"; currentState = Initialized; }
		void listen(int, struct sockaddr_in);			/* Receives the handshake */
		virtual void run();								/* Starts the thread for every client request */
};

#endif
