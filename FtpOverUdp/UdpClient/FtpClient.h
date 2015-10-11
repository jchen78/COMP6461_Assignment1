/*************************************************************************************
*								 File Name	: Client.h		   			 	         *
*	Usage : Sends request to Server for Uploading, downloading and listing of files  *
**************************************************************************************/

#include <winsock.h>
#include <cstdio>
#include <iostream>
#include <cstring>
#include <string>
#include <fstream>
#include <climits>
#include <windows.h>

using namespace std;

#pragma comment(lib, "Ws2_32.lib")
#define HOSTNAME_LENGTH 20
#define FILENAME_LENGTH 20
#define HANDSHAKE_PORT 5001
#define BUFFER_LENGTH 256
#define RCV_BUFFER_SIZE 512
#define MSGHDRSIZE 12
#define SEQUENCE_RANGE 2
#define TRACE 1

/* Types of Messages */
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

/* Structure of Request */
typedef struct
{
	char hostname[HOSTNAME_LENGTH];
	char filename[FILENAME_LENGTH];
} Req;  

/* Buffer for uploading file contents */
typedef struct
{
	char dataBuffer[BUFFER_LENGTH];
} DataContent; //For Put Operation

/* Message format used for sending and receiving */
typedef struct
{
	Type type;
	int  length; /* length of effective bytes in the buffer */
	int  sequenceNumber;
	char buffer[BUFFER_LENGTH];
	char dataBuffer[BUFFER_LENGTH];
} Msg; 

/* FtpClient Class */
class FtpClient
{
	private:
		int clientSock;					/* Socket descriptor */
		int clientPort;					/*  */
		int clientIdentifier;
		int serverIdentifier;
		int sequenceNumber;
		struct sockaddr_in ServAddr;	/* Server socket address */
		unsigned short ServPort;		/* Server port */
		unsigned short ClientPort;		/* Client port */
		char hostName[HOSTNAME_LENGTH];	/* Host Name */
		Req reqMessage;					/* Variable to store Request Message */
		Msg sendMsg,receiveMsg;			/* Message structure variables for Sending and Receiving data */
		WSADATA wsaData;				/* Variable to store socket information */
		string serverName;				/* Variable to store Server IP Address */
		string transferType;			/* Variable to store the Type of Operation */
		int numBytesSent;				/* Variable to store the bytes of data sent to the server */
		int numBytesRecv;				/* Variable to store the bytes of data received from the server */
		int bufferSize;					/* Variable to specify the buffer size */
		bool connectionStatus;			/* Variable to specify the status of the socket connection */
		
		Msg* msgGet();
		int msgSend(Msg *);				/* Sends the packed message to server */
	
		bool performHandshake();		/* Initiates and completes 3-way handshake w/ the server */
		Msg* getInitialHandshakeMessage();
		Msg* processFinalHandshakeMessage(Msg*);

		void setAckMessage(Msg*);

		void performGet();				/* Retrieves the file from Server */
		void performList();
		void terminate();

		void log(const string &logItem);

	public:
		FtpClient();
		void showMenu();				/* Displays the list of available options for User */
		bool startClient();				/* Starts the client process . Returns true if client has successfully completed the handshake, or false otherwise.*/
		unsigned long ResolveName(string name);	/* Resolve the specified host name */
		~FtpClient();		
};