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
	sender = NULL;

	sync.lock();
}

int ServerThread::getId() { return serverId; }

mutex* ServerThread::getSync() { return &sync; }

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

		sync.lock();
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
	setSender(getDirectoryContents());
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
	try { setSender(getFileContents(currentMsg->buffer)); }
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

void ServerThread::setSender(const char* payloadData)
{
	// TODO: Complete functionality here
}

int main(void)
{
	FtpServer ts;
	/* Start the server and start listening to requests */
	ts.start();

	return 0;
}