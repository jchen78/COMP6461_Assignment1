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
	REQ_GET=1
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
		int nextServerSock;					/* Socket for next worker thread */
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
class FtpThread :public Thread
{
	private:
		int serverPort;							/*  */
		int serverSock;							/* ServerSocket */
		struct sockaddr_in addr;				/* Address */
		int addrLength;							/* Length of addr field */
		char request[1024];						/* Request message */
		int requestLength;						/* Request length */

	public:
		FtpThread(int clientSocket, struct sockaddr_in clientAddr, int nextPort):serverSock(clientSocket), addr(clientAddr), serverPort(nextPort) { }
		void listen();							/* Gets the message */
		virtual void run();						/* Starts the thread for every client request */
		int msgRecv(int ,Msg * );				/* Receive the incoming requests */
		int msgSend(int ,Msg * );				/* Send the response */
		void sendFileData(char []);				/* Sends the contents of the file (get)*/
};

#endif
