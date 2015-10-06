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
#include "Thread.h"
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
	nextServerPort = REQUEST_PORT + 1;
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
	for (;;) /* Run forever */
	{
		FtpThread * pt = new FtpThread(nextServerPort++);
		
		// Wait for a request
		pt->listen(serverSock, ServerAddr);

		// Once request has arrived, start new thread so that server may receive another request
		pt-> start();
	}
}

/*-------------------------------FtpThread Class--------------------------------*/
/**
 * Function - listen
 * Usage: Blocks until incoming request message is captured
 *
 * @arg: void
 */
void FtpThread::listen(int sock, struct sockaddr_in initialSocket)
{
	/* Socket Creation */
	if ((thrdSock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	{
		std::cerr << "Socket Creation Error,exit" << endl;
		exit(1);
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(inPort);

	/* Binding the server socket to the Port Number */
    if (::bind(thrdSock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		cerr << "Socket Binding Error from FtpThread, exit" << endl;
		exit(1);
	}

	curRqt = msgGet(sock, initialSocket);
}

/**
 * Function - msgGet
 * Usage: Blocks until the next incoming packet is completely received; returns the packet formatted as a message.
 *
 * @arg: socket, address
 */
Msg* FtpThread::msgGet(SOCKET sock, struct sockaddr_in sockAddr)
{
	char buffer[512];
	int bufferLength;
	clientAddr = sockAddr;
	memcpy(clientAddr.sin_zero, sockAddr.sin_zero, 8);
	addrLength = sizeof(addr);

	/* Check the received Message Header */
	if ((bufferLength = recvfrom(sock, buffer, BUFFER_LENGTH, 0, (SOCKADDR *)&clientAddr, &addrLength)) == SOCKET_ERROR)
	{
		cerr << "recvfrom(...) failed when getting message" << endl;
		exit(1);
	}

	// must destruct after use!
	Msg* msg = new Msg();
	memcpy(msg, buffer, MSGHDRSIZE);
	memcpy(msg->buffer, buffer + MSGHDRSIZE, msg->length);
	return msg;
}

/**
 * Function - msgSend
 * Usage: Returns the length of bytes in msg_ptr->buffer,which have been sent out successfully
 *
 * @arg: int, Msg *
 */
int FtpThread::msgSend(int sock, Msg * msg_ptr)
{
	int n;
	int expectedMsgLen = MSGHDRSIZE + msg_ptr->length;
	if ((n = sendto(sock, (char *)msg_ptr, expectedMsgLen, 0, (SOCKADDR *)&clientAddr, addrLength)) != expectedMsgLen) {
		std::cerr << "Send MSGHDRSIZE+length Error " << endl;
		std::cerr << WSAGetLastError() << endl;
		return(-1);
	}

	return (n-MSGHDRSIZE);
}

/**
 * Function - run
 * Usage: Initiates client-terminated looping
 *
 * @arg: void
 */
void FtpThread::run()
{
	currentSequenceNumber = serverIdentifier % SEQUENCE_RANGE;
	handleCurrentMessage();

	while (currentState != Terminated) {
		curRqt = msgGet(thrdSock, addr);
		handleCurrentMessage();
	}

	/* Close the connection and unlock the Mutex after successful transfer */
	closesocket(thrdSock);
}

void FtpThread::handleCurrentMessage()
{
	/*Start receiving the request */
	if (curRqt == NULL)
	{
		cerr << "Receive Req error,exit " << endl;
		return;
	}

	Msg* reply = NULL;
	
	/* Check the type of operation and Construct the response and send to Client */
	if (currentState == Initialized && curRqt->type == HANDSHAKE) {
		reply = createServerHandshake();
		currentState = HandshakeStarted;
	} else if (currentState == HandshakeStarted && curRqt->type == COMPLETE_HANDSHAKE) {
		if (isHandshakeCompleted())
			currentState = ReceivingRequest;
	} else if (currentState == ReceivingRequest && curRqt->type == REQ_GET) {
		reply = tryLoadFile() ? getNextChunk() : getErrorMessage("No such file.");
	} else if (currentState == ReceivingRequest && curRqt->type == REQ_LIST) {
		loadDirectoryContents();
		reply = getNextChunk();
	} else if (currentState == Sending && curRqt->type == PUT) {
		if (curRqt->sequenceNumber == currentSequenceNumber && !payloadData.empty()) {
			// Iterate
			currentSequenceNumber = (currentSequenceNumber + 1) % SEQUENCE_RANGE;
			payloadData.pop(); // TODO: Manage memory?
			if (payloadData.empty())
				currentState = ReceivingRequest;
		}

		reply = getNextChunk();
	} else if (currentState == ReceivingRequest && curRqt->type == PUT) {
		reply = curRqt->sequenceNumber == currentSequenceNumber ? getErrorMessage("EOF") : NULL;
	} else if (currentState == ReceivingRequest && curRqt->type == TERMINATE)
		currentState = Terminated;
	else
		cerr << "Invalid request header; ignored and waiting for next request" << endl;
	
	if (reply != NULL) {
		msgSend(thrdSock, reply);
		delete reply;
	}
}

Msg* FtpThread::createServerHandshake()
{
	Msg* handshakeAck = new Msg(*curRqt);
	int lenClientData = 0;
	while (isdigit(handshakeAck->buffer[lenClientData]))
		lenClientData++;
	
	std::string serverPart = ",";
	serverPart += std::to_string((_ULonglong)serverIdentifier);
	strcpy(&handshakeAck->buffer[lenClientData], serverPart.c_str());

	return handshakeAck;
}

bool FtpThread::isHandshakeCompleted()
{
	return std::stoi(curRqt->buffer) == serverIdentifier;
}

bool FtpThread::tryLoadFile()
{
	// Empties out any data which may be remaining in the queue, if any
	if (!payloadData.empty())
		queue<char*>().swap(payloadData);

	ifstream fileToRead;
	fileToRead.open(string(filesDirectory).append(curRqt->buffer));
	char* currentBuffer = NULL;
	if (fileToRead.is_open()) {
		while (!fileToRead.eof()) {
			currentBuffer = new char[BUFFER_LENGTH];
			memset(currentBuffer, '\0', BUFFER_LENGTH);
			fileToRead.read(currentBuffer, BUFFER_LENGTH);

			payloadData.push(currentBuffer);
		}

		finalPayloadLength = fileToRead.gcount();
		return true;
	}

	return false;
}

void FtpThread::loadDirectoryContents()
{
	// Empties out any data which may be remaining in the queue, if any
	if (!payloadData.empty())
		queue<char*>().swap(payloadData);

	// Code adapted from https://msdn.microsoft.com/en-us/library/windows/desktop/aa365200(v=vs.85).aspx
	// Removed most of the error-checking --it is the responsibility of the person setting up the server to ensure that directories and files are set up correctly
    WIN32_FIND_DATA ffd;
    TCHAR szDir[MAX_PATH];
    size_t length_of_arg;
    HANDLE hFind = INVALID_HANDLE_VALUE;

	char *buffer = new char[BUFFER_LENGTH];
	int currIndex = 0;
	memset(buffer, '\0', BUFFER_LENGTH);
	hFind = FindFirstFile("files\\*", &ffd);
    do
    {
        if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
			for (int i = 0; i < sizeof(ffd.cFileName); i++, currIndex++) {
				if (currIndex == BUFFER_LENGTH) {
					payloadData.push(buffer);
					buffer = new char[BUFFER_LENGTH];
					memset(buffer, '\0', BUFFER_LENGTH);
					currIndex = 0;
				}

				buffer[currIndex] = ffd.cFileName[i];
				if (buffer[currIndex] == '\0')
					i = sizeof(ffd.cFileName);
			}
        }
    }
    while (FindNextFile(hFind, &ffd) != 0);

	payloadData.push(buffer);
	finalPayloadLength = currIndex == 0 ? BUFFER_LENGTH : currIndex;
}

Msg* FtpThread::getNextChunk()
{
	if (payloadData.empty())
		return NULL;

	currentState = Sending;
	Msg* responseMsg = new Msg();
	responseMsg->type = RESP;
	responseMsg->length = payloadData.size() == 1 ? finalPayloadLength : BUFFER_LENGTH;
	responseMsg->sequenceNumber = currentSequenceNumber;
	memcpy(responseMsg->buffer, payloadData.front(), BUFFER_LENGTH);

	return responseMsg;
}

Msg* FtpThread::getErrorMessage(const char* text)
{
	Msg* errorMsg = new Msg();
	errorMsg->type = RESP_ERR;
	errorMsg->length = BUFFER_LENGTH;
	strcpy(errorMsg->buffer, text);

	return errorMsg;
}

/**
 * Function - main
 * Usage: Initiates the Server
 *
 * @arg: void
 */
int main(void)
{
	FtpServer ts;
	/* Start the server and start listening to requests */
	ts.start();

	return 0;
}