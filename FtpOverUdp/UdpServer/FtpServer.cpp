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

int main(void)
{
	//AsyncLock lock(true);
	//ThreadA consumption(&lock);
	//ThreadB signalling(&lock);

	//consumption.start();
	//signalling.start();

	//time_t start, current;
	//time(&start); do { time(&current); } while (difftime(current, start) < 100);

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








	
	
	//{
		char payload[256 * 5 - 6];
		memset(payload, ' ', 256 * 5 - 6);
		payload[0] = 'A';
		payload[254] = '\r';
		payload[255] = '\n';

		payload[256] = 'B';
		payload[510] = '\r';
		payload[511] = '\n';

		payload[512] = 'C';
		payload[766] = '\r';
		payload[767] = '\n';

		payload[768] = 'D';
		payload[1022] = '\r';
		payload[1023] = '\n';

		payload[1024] = 'E';
	//	std::ofstream os("serverFiles\\testFile.txt", ios::out | ios::binary);
	//	os.write(payload, 256 * 5);
	//}
	struct sockaddr_in ClientAddr;
	memset(&ClientAddr, 9, sizeof(ClientAddr));
	ClientAddr.sin_family = AF_INET;
	ClientAddr.sin_addr.S_un.S_addr = *((unsigned long *)gethostbyname("AsusG551")->h_addr_list[0]);
	ClientAddr.sin_port = htons(5000);

	//time_t start, current;
	//time(&start); do { time(&current); } while (difftime(current, start) < 3);
	//int noOp = 0;

	Msg msg;
	memset(&msg, 0, sizeof(msg));
	msg.clientId = 123;
	msg.serverId = 456;
	msg.type = HANDSHAKE;
	msg.length = 0;

	AsyncLock ioLock;
	ServerThread* st = new ServerThread(456, serverSock, ClientAddr, &msg, &ioLock);
	AsyncLock* stl = st->getSync();
	st->start();

	stl->waitForSignalling();
	msg.type = COMPLETE_HANDSHAKE;
	stl->finalizeSignalling();

	stl->waitForSignalling();
	msg.type = POST;
	msg.sequenceNumber = 2;
	strcpy(msg.buffer, "receiverTest.txt");
	stl->finalizeSignalling();
	
	stl->waitForSignalling();
	msg.type = RESP;
	msg.sequenceNumber = 5;
	msg.length = 256;
	memcpy(msg.buffer, &payload[768], 256);
	stl->finalizeSignalling();
	
	stl->waitForSignalling();
	msg.type = RESP;
	msg.sequenceNumber = 2;
	msg.length = 256;
	memcpy(msg.buffer, &payload[0], 256);
	stl->finalizeSignalling();
	
	stl->waitForSignalling();
	msg.type = RESP;
	msg.sequenceNumber = 5;
	msg.length = 256;
	memcpy(msg.buffer, &payload[768], 256);
	stl->finalizeSignalling();

	stl->waitForSignalling();
	msg.type = RESP;
	msg.sequenceNumber = 3;
	msg.length = 256;
	memcpy(msg.buffer, &payload[256], 256);
	stl->finalizeSignalling();

	stl->waitForSignalling();
	msg.type = RESP;
	msg.sequenceNumber = 4;
	msg.length = 256;
	memcpy(msg.buffer, &payload[512], 256);
	stl->finalizeSignalling();

	stl->waitForSignalling();
	msg.type = RESP;
	msg.sequenceNumber = 6;
	msg.length = 250;
	memcpy(msg.buffer, &payload[1024], 250);
	stl->finalizeSignalling();

	stl->waitForSignalling();
	msg.type = RESP_ERR;
	msg.sequenceNumber = 0;
	msg.length = 4;
	memset(msg.buffer, 0, 256);
	strcpy(msg.buffer, "EOF");
	stl->finalizeSignalling();

	stl->waitForSignalling();
	msg.type = TERMINATE;
	msg.sequenceNumber = 0;
	msg.length = 4;
	memset(msg.buffer, 0, 256);
	stl->finalizeSignalling();

	stl->waitForSignalling();
	msg.type = ACK;
	msg.sequenceNumber = 0;
	msg.length = 4;
	memset(msg.buffer, 0, 256);
	stl->finalizeSignalling();









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