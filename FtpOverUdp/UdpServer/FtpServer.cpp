/*************************************************************************************
*								 File Name	: Server.cpp		   			 	     *
*	Usage : Handles Client request for Uploading, downloading and listing of files   *
**************************************************************************************/
#define _CRT_SECURE_NO_WARNINGS
#include <winsock.h>
#include <iostream>
#include <windows.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cstdio>
#include <string>
#include <vector>
#include <fstream>
#include <process.h>
#include "FtpServer.h"
#include <stdlib.h>

using namespace std;

/**
 * Constructor - FtpServer
 * Usage: Initialize the socket connection 
 *
 * @arg: void
 */
FtpServer::FtpServer()
{
	WSADATA wsadata;
	/* Initialize Windows Socket information */
	if (WSAStartup(0x0202,&wsadata)!=0)
	{
		cerr << "Starting WSAStartup() error\n" << endl;
		exit(1);
	}		

	/* Display the name of the host */
	if(gethostname(serverName,HOSTNAME_LENGTH)!=0) 
	{
		cerr << "Get the host name error,exit" << endl;
		exit(1);
	}

	cout << "Server: " << serverName << " waiting to be contacted for get/put request..." << endl;
	log(string("Starting up on host ").append(serverName));

	/* Socket Creation */
	if ((serverSock = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
	{
		std::cerr << "Socket Creation Error,exit" << endl;
		exit(1);
	}

	/* Fill-in Server Port and Address information */
	ServerPort=REQUEST_PORT;
	memset(&ServerAddr, 0, sizeof(ServerAddr));      /* Zero out structure */
	ServerAddr.sin_family = AF_INET;                 /* Internet address family */
	ServerAddr.sin_addr.s_addr = INADDR_ANY;  /* Any incoming interface */
	ServerAddr.sin_port = htons(ServerPort);         /* Local port */

	/* Binding the server socket to the Port Number */
    if (::bind(serverSock, (struct sockaddr *) &ServerAddr, sizeof(ServerAddr)) < 0)
	{
		cerr << "Socket Binding Error from FtpServer,exit" << endl;
		exit(1);
	}

	log(string("Bound to port ").append(to_string((_ULonglong)ServerPort)));
}

/**
 * Destructor - ~FtpServer
 * Usage: DeAllocate the allocated memory
 *
 * @arg: void
 */
FtpServer::~FtpServer()
{
	closesocket(serverSock);
	WSACleanup();
}

/**
 * Function - start
 * Usage: Listen and handle the requests from clients
 *
 * @arg: void
 */
void FtpServer::start()
{
	while (true) /* Run forever */
	{
		/* For each message:
		 *		- log the message
		 *		- if the server-id does not correspond to any server thread:
		 *			- log error
		 *			- wait for next message (silently ignore the message)
		 *		- if no server-id is given:
		 *			- create a new server thread
		 *			- update the message with the new server id
		 *			- register the new server thread
		 *		- retrieve the server thread
		 *		- dispatch the message to the server thread
		 *		- wait for next message
		 */
	}
}

void FtpServer::log(const std::string &logItem)
{
	time_t rawtime;
	struct tm * timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	FILE *logFile = fopen("logfile.txt", "a");
	fprintf(logFile, "Server (root): %s -- %s\r\n", logItem.c_str(), asctime(timeinfo));
	fclose(logFile);
}

/*-------------------------------ServerThread Class--------------------------------*/
ServerThread::ServerThread(int serverSocket, struct sockaddr_in serverAddress, Msg* initialHandshake)
{
	srand(time(NULL));
	serverId = rand();
	socket = serverSocket;
	address = serverAddress;
	currentState = INITIALIZING;
	currentMsg = initialHandshake;
	sender = new Sender(socket, serverId, currentMsg->clientId, address);
	filesDirectory = "serverFiles\\";

	outerSync.lock();
}

int ServerThread::getId() { return serverId; }

mutex* ServerThread::getSync() { return &outerSync; }

void ServerThread::run()
{
	do // NOTE : currentMsg can only be null if the initial handshake is null, or ServerThread sets it to NULL.
	{
		switch (currentState) {
		case INITIALIZING:
			startHandshake();
			break;
		case HANDSHAKING:
			if (currentMsg->type == COMPLETE_HANDSHAKE)
				endHandshake();
			// TODO: Send RESP_ERR
			break;
		case WAITING_FOR_REQUEST:
			switch (currentMsg->type) {
			case GET_LIST:
				sendList();
				break;
			case GET_FILE:
				sendFile();
				break;
			case PUT:
				// TODO: Complete action
				break;
			case POST:
				// TODO: Complete action
				break;
			default:
				// TODO: Send RESP_ERR
				break;
			}
			break;
		case SENDING:
			if (currentMsg->type == ACK) {
				// TODO: Complete action
			} else {
				// TODO: Send RESP_ERR
			}
			break;
		case RECEIVING:
			if (currentMsg->type == RESP) {
				// TODO: Complete action
			} else if (currentMsg->type == RESP_ERR && strcmp(currentMsg->buffer, "End of file.") == 0) {
				// TODO: Complete action
			} else {
				// TODO: Send RESP_ERR
			}
			break;
		case RENAMING:
			if (currentMsg->type == RESP) {
				// TODO: Complete action
			} else if (currentMsg->type == TERMINATE && strcmp(currentMsg->buffer, originalFileName.c_str()) == 0) {
				// TODO: Complete action
			} else {
				// TODO: Send RESP_ERR
			}
			break;
		case EXITING:
			if (currentMsg->type == ACK) {
				// TODO: Complete action (incl. set currentMsg to null)
			} else {
				// TODO: Send RESP_ERR
			}
			break;
		}

		outerSync.lock();
	} while (currentMsg != NULL);
}

void ServerThread::startHandshake()
{
	currentState = HANDSHAKING;
	isResponseComplete = new bool(false);
	currentResponse = new SenderThread(socket, serverId, currentMsg->clientId, &address, isResponseComplete, HANDSHAKE, 0, "", 0);
}

void ServerThread::endHandshake()
{
	currentState = WAITING_FOR_REQUEST;
	(*isResponseComplete) = true; // Will stop sending HANDSHAKE after timeout (SenderThread will auto-destroy)
	currentResponse = NULL;
}

void ServerThread::sendList()
{
	currentState = SENDING;
	startSender(getDirectoryContents());
}

char* ServerThread::getDirectoryContents()
{
	queue<char*> payloadData;
	int payloadLength = 0;
	// Code adapted from https://msdn.microsoft.com/en-us/library/windows/desktop/aa365200(v=vs.85).aspx
	// Removed most of the error-checking --it is the responsibility of the person setting up the server to ensure that directories and files are set up correctly
	WIN32_FIND_DATA ffd;
	HANDLE hFind = INVALID_HANDLE_VALUE;

	hFind = FindFirstFile(string(filesDirectory).append("*").c_str(), &ffd);
	do
	{
		if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			payloadData.push(ffd.cFileName);
			payloadLength += strlen(ffd.cFileName) + 1;
		}
	} while (FindNextFile(hFind, &ffd) != 0);

	char* payloadContent = new char[payloadLength];
	int currentIndex = 0;
	while (!payloadData.empty()) {
		strcpy(payloadContent + currentIndex, payloadData.front());
		currentIndex += strlen(payloadData.front()) + 1;
		if (currentIndex > payloadLength)
			throw new exception("More files than expected. Please retry");
	}

	return payloadContent;
}

void ServerThread::sendFile()
{
	currentState = SENDING;
	try { startSender(getFileContents(currentMsg->buffer)); }
	catch (exception e) {
		currentState = WAITING_FOR_REQUEST;
		// TODO: Send RESP_ERR
	}
}

char* ServerThread::getFileContents(const char* fileName)
{
	ifstream fileToRead (string(filesDirectory).append(fileName), ios::ate || ios::binary);
	if (!fileToRead.is_open())
		throw new exception("File not found");

	streampos size = fileToRead.tellg();
	fileToRead.seekg(0);
	char* fileContents = new char[size];
	fileToRead.read(fileContents, size);
	fileToRead.close();

	return fileContents;
}

void ServerThread::startSender(const char* payloadData)
{
	sender->initializePayload(payloadData, sizeof(payloadData), currentMsg->sequenceNumber, currentMsg);
	sender->start();
}

int main(void)
{
	WSADATA wsadata;
	/* Initialize Windows Socket information */
	if (WSAStartup(0x0202, &wsadata) != 0)
	{
		cerr << "Starting WSAStartup() error\n" << endl;
		exit(1);
	}

	char serverName[20];
	/* Display the name of the host */
	if (gethostname(serverName, HOSTNAME_LENGTH) != 0)
	{
		cerr << "Get the host name error,exit" << endl;
		exit(1);
	}

	cout << "Server: " << serverName << " waiting to be contacted for get/put request..." << endl;

	SOCKET serverSock;
	/* Socket Creation */
	if ((serverSock = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
	{
		std::cerr << "Socket Creation Error,exit" << endl;
		exit(1);
	}

	/* Fill-in Server Port and Address information */
	int ServerPort = REQUEST_PORT;
	struct sockaddr_in ServerAddr;
	memset(&ServerAddr, 0, sizeof(ServerAddr));      /* Zero out structure */
	ServerAddr.sin_family = AF_INET;                 /* Internet address family */
	ServerAddr.sin_addr.s_addr = INADDR_ANY;  /* Any incoming interface */
	ServerAddr.sin_port = htons(ServerPort);         /* Local port */

	/* Binding the server socket to the Port Number */
	if (::bind(serverSock, (struct sockaddr *) &ServerAddr, sizeof(ServerAddr)) < 0)
	{
		cerr << "Socket Binding Error from FtpServer,exit" << endl;
		exit(1);
	}








	
	
	struct sockaddr_in ClientAddr;
	memset(&ClientAddr, 9, sizeof(ClientAddr));
	ClientAddr.sin_family = AF_INET;
	ClientAddr.sin_addr.S_un.S_addr = *((unsigned long *)gethostbyname("AsusG551")->h_addr_list[0]);
	ClientAddr.sin_port = htons(5000);
	Msg msg;
	memset(&msg, 0, sizeof(msg));
	Sender *sender = new Sender(serverSock, 123, 456, ClientAddr);
	Common::AsyncLock* sync = sender->getAsyncControl();
	sender->initializePayload("End of file.", 14, 3, &msg);
	sender->start();
	time_t start, current;
	time(&start);
	do { time(&current); } while (difftime(current, start) < 3);
	int i = 0;
	{
		std::unique_lock<mutex> locker(sync->dataLock);
		msg.sequenceNumber = 3;
		msg.type = ACK;
		sync->isAsyncReady = true;
		sync->operationLock.notify_one();
	}
	
	time(&start);
	do { time(&current); } while (difftime(current, start) < 3);
	{
		std::unique_lock<mutex> locker(sync->dataLock);
		msg.sequenceNumber = 4;
		msg.type = ACK;
		sync->isAsyncReady = true;
		sync->operationLock.notify_one();
	}










	closesocket(serverSock);
	WSACleanup();

	//FtpServer ts;
	///* Start the server and start listening to requests */
	//ts.start();

	//return 0;
}

Msg* msgGet(char* buffer)
{
	// must destruct after use!
	Msg* msg = new Msg();
	memcpy(msg, buffer, MSGHDRSIZE);
	memcpy(msg->buffer, buffer + MSGHDRSIZE, msg->length);
	
	delete[] buffer;
	return msg;
}

char* getRawBuffer(struct sockaddr_in ServAddr, int clientSock)
{
	char* buffer = new char[RCV_BUFFER_SIZE];
	int bufferLength;
	int addrLength = sizeof(ServAddr);

	/* Check the received Message Header */
	if ((bufferLength = recvfrom(clientSock, buffer, RCV_BUFFER_SIZE, 0, (SOCKADDR *)&ServAddr, &addrLength)) == SOCKET_ERROR)
	{
		cerr << "recvfrom(...) failed when getting message" << endl;
		cerr << WSAGetLastError() << endl;
		exit(1);
	}

	return buffer;
}