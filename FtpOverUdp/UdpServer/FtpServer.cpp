/*************************************************************************************
*								 File Name	: Server.cpp		   			 	     *
*	Usage : Handles Client request for Uploading, downloading and listing of files   *
**************************************************************************************/
#define _CRT_SECURE_NO_WARNINGS
#include "FtpServer.h"
#include <string>

using namespace std;

/**
 * Constructor - FtpServer
 * Usage: Initialize the socket connection 
 *
 * @arg: void
 */
FtpServer::FtpServer() : ioLock(false, 256)
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

	int option;
	cout << "Do you want to reuse server IDs?" << endl;
	cout << "If not, there will be a limit of 256 operations in total, across all threads." << endl;
	cout << "Please enter 1 to reuse, or 0 otherwise: ";
	cin >> option;
	areServerIdsReused = option == 1;
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
	Msg* currentRequest = NULL;
	while (true) /* Run forever */ {
		currentRequest = getMessage();
		log(string("Received message: ").append(describeMessage(currentRequest)).append(";"));
		dispatchMessage(getServerId(currentRequest), currentRequest);
	}
}

Msg* FtpServer::getMessage()
{
	char* buffer = new char[RCV_BUFFER_SIZE];
	int bufferLength;
	clientLen = sizeof(ClientAddr);

	/* Check the received Message Header */
	if ((bufferLength = recvfrom(serverSock, buffer, RCV_BUFFER_SIZE, 0, (SOCKADDR *)&ClientAddr, &clientLen)) == SOCKET_ERROR)
	{
		cerr << "recvfrom(...) failed when getting message" << endl;
		cerr << WSAGetLastError() << endl;
		exit(1);
	}

	// must destruct after use!
	Msg* msg = new Msg();
	memcpy(msg, buffer, MSGHDRSIZE);
	memcpy(msg->buffer, buffer + MSGHDRSIZE, msg->length);

	delete[] buffer;
	return msg;
}

int FtpServer::getServerId(Msg* message) {
	int clientId = message->clientId;
	int requestServerId = message->serverId;
	if (clientIds.find(clientId) == clientIds.end()) {
		clientIds.insert(make_pair(clientId, createServerThread()));
		pastServerIds.insert(make_pair(clientId, clientIds[clientId]));
	} else {
		int currentServerId = clientIds[clientId];
		if (message->type == HANDSHAKE) {
			if (requestServerId == currentServerId) {
				// Clean up current thread
				pastServerIds[clientId] = requestServerId;
				isThreadActive[currentServerId] = false;
				dispatchMessage(currentServerId, message);
				if (areServerIdsReused) {
					AsyncLock* currentLock = threadLocks[currentServerId];
					currentLock->waitForSignalling();
					messages.erase(currentServerId);
					isThreadActive.erase(currentServerId);
					threadLocks.erase(currentServerId);
					currentLock->finalizeSignalling();
					delete currentLock;
				}

				// Spawn new thread
				clientIds[clientId] = createServerThread();
			} else if (requestServerId == pastServerIds[clientId])
				message->serverId = currentServerId;
		}
	}

	int currentServerId = clientIds[clientId];
	return (requestServerId == NULL_SERVER_ID || requestServerId == currentServerId) ? currentServerId : NULL_SERVER_ID;
}

int FtpServer::createServerThread() {
	int serverId = 0;
	do {
		srand(time(NULL));
		serverId = rand() % 256;
	} while (threadLocks.find(serverId) != threadLocks.end());

	Msg msg = Msg();
	std::memset(&msg, 0, sizeof(msg));
	messages.insert(std::make_pair(serverId, msg));
	isThreadActive.insert(std::make_pair(serverId, true));
	ServerThread* newThread = new ServerThread(serverId, serverSock, ClientAddr, &isThreadActive[serverId], &messages[serverId], &ioLock);
	threadLocks.insert(std::make_pair(serverId, newThread->getSync()));
	newThread->start();

	return serverId;
}

void FtpServer::dispatchMessage(int serverId, Msg* message) {
	if (serverId == NULL_SERVER_ID) {
		log(string("Discarded message: ").append(describeMessage(message)).append(";"));
		return;
	}

	message->serverId = serverId;
	log(string("About to dispatch message: ").append(describeMessage(message)).append(";"));
	
	AsyncLock* threadLock = threadLocks[serverId];
	threadLock->waitForSignalling();
	Msg* threadMsg = &messages[serverId];
	memcpy(threadMsg, message, sizeof(*message));
	threadLock->finalizeSignalling();
	log(string("Finished dispatching message: ").append(describeMessage(message)).append(";"));
}

std::string FtpServer::describeMessage(Msg* message) {
	return string("ServerId: ").append(std::to_string(message->serverId))
		.append("; ClientId: ").append(std::to_string(message->clientId))
		.append("; Type: ").append(std::to_string(message->type))
		.append("; Sequence: ").append(std::to_string(message->sequenceNumber));
}

void FtpServer::log(const std::string &logItem)
{
	time_t rawtime;
	char* formattedTime;

	time(&rawtime);
	char* unformattedTime = asctime(localtime(&rawtime));
	int length = strlen(unformattedTime);
	formattedTime = new char[length];
	memcpy(formattedTime, unformattedTime, length);
	formattedTime[length - 1] = 0;

	FILE *logFile = fopen("logfile.txt", "a");
	fprintf(logFile, "Server (root): %s -- %s\r\n", formattedTime, logItem.c_str());
	fclose(logFile);
}

int main(void)
{
	FtpServer ts;
	/* Start the server and start listening to requests */
	ts.start();

	return 0;
}