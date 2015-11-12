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
#include <Common.h>
#include <Receiver.h>
#include <Sender.h>

using namespace std;

#pragma comment(lib, "Ws2_32.lib")
#define HOSTNAME_LENGTH 20
#define FILENAME_LENGTH 20
#define HANDSHAKE_PORT 5002
#define RCV_BUFFER_SIZE 512
#define TRACE 1

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
		Msg sendMsg,receiveMsg;			/* Message structure variables for Sending and Receiving data */
		WSADATA wsaData;				/* Variable to store socket information */
		string serverName;				/* Variable to store Server IP Address */
		string transferType;			/* Variable to store the Type of Operation */
		int numBytesSent;				/* Variable to store the bytes of data sent to the server */
		int numBytesRecv;				/* Variable to store the bytes of data received from the server */
		int bufferSize;					/* Variable to specify the buffer size */
		bool connectionStatus;			/* Variable to specify the status of the socket connection */
		string filesDirectory;
		Common::Receiver* receiver;
		Common::Sender* sender;
		
		int waitTimeOut();
		Msg* msgGet();
		Payload* rawGet();
		int msgSend(Msg *);				/* Sends the packed message to server */
		int rawSend(Payload* data);
	
		bool performHandshake();		/* Initiates and completes 3-way handshake w/ the server */
		Msg* getInitialHandshakeMessage();
		Msg* processFinalHandshakeMessage(Msg*);

		void setAckMessage(Msg*);

		void sendRstMessage();

		void performGet();				/* Retrieves the file from Server */
		void performList();
		void performUpload();
		void performRename();
		void terminate();

		void log(const string &logItem);

	public:
		FtpClient();
		void showMenu();				/* Displays the list of available options for User */
		bool startClient();				/* Starts the client process . Returns true if client has successfully completed the handshake, or false otherwise.*/
		unsigned long ResolveName(string name);	/* Resolve the specified host name */
		~FtpClient();		
};