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
#define MSGHDRSIZE 8

/* Type of Messages */
typedef enum
{
	REQ_GET = 1,
	RESP = 3
} Type;

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
	char buffer[BUFFER_LENGTH];
} Msg;

/* FtpServer Class */
class FtpServer
{
	private:
		int serverSock,clientSock;				/* Socket descriptor for server and client*/
		int nextServerSock;						/* Socket for next worker thread */
		struct sockaddr_in ClientAddr;			/* Client address */
		struct sockaddr_in ServerAddr;			/* Server address */
		unsigned short ServerPort;				/* Server port */
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
		int serverSock;									/* ServerSocket */
		struct sockaddr_in addr;						/* Address */
		struct sockaddr_in* clientAddr;					/* Client address */
		int addrLength;									/* Length of addr field */
		Msg* reqHdr;									/* Thread-initiating request */
		
		// Methods
		Msg* msgGet(SOCKET, SOCKADDR*, int*);			/* Gets a request message */
		int msgSend(int , Msg*);						/* Send the response */

	public:
		FtpThread() { reqHdr = NULL; }					/* Constructs the worker thread with sufficient data to receive a new request header */
		void listen(int, struct sockaddr_in);			/* Receives the handshake */
		virtual void run();								/* Starts the thread for every client request */
		void sendFileData(char []);						/* Sends the contents of the file (get)*/
};

#endif
