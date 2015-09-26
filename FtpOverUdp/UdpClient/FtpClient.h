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
#define REQUEST_PORT 5001
#define BUFFER_LENGTH 256
#define MSGHDRSIZE 8

/* Types of Messages */
typedef enum
{
	REQ_GET=1
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
	char buffer[BUFFER_LENGTH];
	char dataBuffer[BUFFER_LENGTH];
} Msg; 

/* FtpClient Class */
class FtpClient
{
	private:
		int clientSock;					/* Socket descriptor */
		struct sockaddr_in ServAddr;	/* Server socket address */
		unsigned short ServPort;		/* Server port */
		char hostName[HOSTNAME_LENGTH];	/* Host Name */
		Req reqMessage;					/* Variable to store Request Message */
		Msg sendMsg,receiveMsg;			/* Message structure variables for Sending and Receiving data */
		WSADATA wsaData;				/* Variable to store socket information */
		string serverIpAdd;				/* Variable to store Server IP Address */
		string transferType;			/* Variable to store the Type of Operation */
		string fileName;				/* Variable to store name of the file for retrieval or transfer */
		int numBytesSent;				/* Variable to store the bytes of data sent to the server */
		int numBytesRecv;				/* Variable to store the bytes of data received from the server */
		int bufferSize;					/* Variable to specify the buffer size */
		bool connectionStatus;			/* Variable to specify the status of the socket connection */
	
	public:
		FtpClient(); 
		void run();						/* Invokes the appropriate function based on selected option */
		void getOperation();			/* Retrieves the file from Server */
		void showMenu();				/* Displays the list of available options for User */
		void startClient();				/* Starts the client process */
		int msgSend(int ,Msg * );		/* Sends the packed message to server */
		unsigned long ResolveName(string name);	/* Resolve the specified host name */
		~FtpClient();		
};