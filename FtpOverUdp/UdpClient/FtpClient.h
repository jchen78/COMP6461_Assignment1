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
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>




using namespace std;

#pragma comment(lib, "Ws2_32.lib")
#define HOSTNAME_LENGTH 20
#define FILENAME_LENGTH 20
#define REQUEST_PORT 5001
#define BUFFER_LENGTH 1024
#define MSGHDRSIZE 8
#define BUF_LEN 512
#define MAX_RESULT 256

/* Types of Messages */
typedef enum
{
	REQ_GET = 1,

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
	int  ACK;
	
} Msg; 

/* UdpClient Class */
class UdpClient
{
	private:
		int clientHandShakeSock, clientSock;        /* Hand shake Socket descriptor and data transport Socket descriptor */
		 					
		int syn;
		int ack;
		int sequenceNumber;

		char handShakeBuffer[BUFFER_LENGTH];      /*Storage the hand shake */
		
		char *handShakeMessage[BUFFER_LENGTH];  /*Hand shake message*/
		
		int addrLength = sizeof(ServAddr);

		struct sockaddr_in ServAddr;	/* Server socket address */
		unsigned short ServPort;		/* Server port */
		char hostName[HOSTNAME_LENGTH];	/* Host Name */
		Req reqMessage;					/* Variable to store Request Message */
		Msg sendtoMsg,receiveMsg;			/* Message structure variables for Sending and Receiving data */
		WSADATA wsaData;				/* Variable to store socket information */
		string serverIpAdd;				/* Variable to store Server IP Address */
		string transferType;			/* Variable to store the Type of Operation */
		string fileName;				/* Variable to store name of the file for retrieval or transfer */
		int numBytesSent;				/* Variable to store the bytes of data sent to the server */
		int numBytesRecvfrom;				/* Variable to store the bytes of data received from the server */
		int bufferSize;					/* Variable to specify the buffer size */
		bool connectionStatus;			/* Variable to specify the status of the socket connection */
	

		typedef struct
		{
			int ID;
			BYTE lparam[BUF_LEN * 2];
		}COMMAND;

		typedef struct
		{
			char FileName[MAX_PATH];//260byte  
			int FileLen;
			char Time[50];
			BOOL IsDir;
			BOOL Error;
			HICON hIcon;
		}FILEINFO;

		Msg * msg_ptr;


	public:
		UdpClient(); 
		void get();						/* Invokes the appropriate function based on selected option */
		void put();
		void list();
		void getDataOperation();			/* Retrieves the file from Server */
		void putDataOperation();            /*Upload data to server*/
		void listOperation();                /*Get the directory*/
		void showMenu();				/* Displays the list of available options for User */
		void startClient();				/* Starts the client process */
		void threeWayHandShake();       /*Start three way hand shake with server*/


		int msgsendto(int ,Msg * );		/* Sends the packed message to server */
		unsigned long ResolveName(string name);	/* Resolve the specified host name */
		~UdpClient();		

		
};
