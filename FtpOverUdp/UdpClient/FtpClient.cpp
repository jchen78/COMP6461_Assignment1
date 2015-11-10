/*************************************************************************************
*								 File Name	: Client.cpp		   			 	     *          *
*	Usage : Sends request to Server for Uploading, downloading and listing of files  *
**************************************************************************************/
#define _CRT_SECURE_NO_WARNINGS
#include "FtpClient.h"
#include <time.h>
#include <stdlib.h>
using namespace std;

/**
 * Constructor - FtpClient
 * Usage: Initialize the connection status 
 *
 * @arg: void
 */
FtpClient::FtpClient()
{
	connectionStatus = true;
	clientIdentifier = -1;
	serverIdentifier = -1;
}

/**
 * Function - ResolveName
 * Usage: Returns the binary, network byte ordered address
 *
 * @arg: string
 */
unsigned long FtpClient::ResolveName(string name)
{
	struct hostent *host;            /* Structure containing host information */

	if ((host = gethostbyname(name.c_str())) == NULL)
	{
		cerr<<"gethostbyname() failed"<<endl;
		return(1);
	}

	/* Return the binary, network byte ordered address */
	return *((unsigned long *) host->h_addr_list[0]);
}

/**
 * Function - msgSend
 * Usage: Returns the length of bytes in msg_ptr->buffer,which have been sent out successfully
 *
 * @arg: Msg *
 */
int FtpClient::msgSend(Msg * msg_ptr)
{
	msg_ptr->clientId = clientIdentifier;
	msg_ptr->serverId = serverIdentifier;
	Payload* payload = new Payload();
	payload->length = MSGHDRSIZE + msg_ptr->length;
	memcpy(payload->data, (char*)msg_ptr, payload->length);
	int n = rawSend(payload);
	delete payload;
	return (n-MSGHDRSIZE);
}

int FtpClient::rawSend(Payload* data) {
	int n;
	int expectedMsgLen = data->length;
	int addrLength = sizeof(ServAddr);
	if ((n = sendto(clientSock, data->data, expectedMsgLen, 0, (SOCKADDR *)&ServAddr, addrLength)) != expectedMsgLen) {
		std::cerr << "Send MSGHDRSIZE+length Error " << endl;
		std::cerr << WSAGetLastError() << endl;
		return(-1);
	}

	return n;
}

/**
 * Function - msgGet
 * Usage: Blocks until the next incoming packet is completely received; returns the packet formatted as a message.
 *
 * @arg: void
 */
Msg* FtpClient::msgGet()
{
	Payload* data = rawGet();
	char* buffer = data->data;
	
	// must destruct after use!
	Msg* msg = new Msg();
	memcpy(msg, buffer, MSGHDRSIZE);
	memcpy(msg->buffer, buffer + MSGHDRSIZE, msg->length);

	delete data;
	return msg;
}

Payload* FtpClient::rawGet()
{
	char buffer[RCV_BUFFER_SIZE];
	int bufferLength;

	/* Check the received Message Header */
	if ((bufferLength = recv(clientSock, buffer, RCV_BUFFER_SIZE, 0)) == SOCKET_ERROR)
	{
		cerr << "recvfrom(...) failed when getting message" << endl;
		cerr << WSAGetLastError() << endl;
		exit(1);
	}

	Payload* data = new Payload();
	data->length = bufferLength;
	memcpy(data->data, buffer, bufferLength);
	return data;
}

/**
 * Function - getOperation
 * Usage: Establish connection and retrieve file from server
 *
 * @arg: void
 */
void FtpClient::performGet()
{ 
	string fileName;
	cout << "Type name of file to be retrieved: ";
	getline(cin, fileName);

	sendMsg.type = GET_FILE;
	strcpy(sendMsg.buffer,fileName.c_str());
	sendMsg.length = fileName.length();
	numBytesSent = msgSend(&sendMsg);
    if (numBytesSent == SOCKET_ERROR)
	{
		cout << "Send failed.. Check the Message length.. " << endl;     
		return;
	}

	cout << endl << endl << "Sent Request to " << serverName << ", Waiting... " << endl;
	/* Send the packed message */

	Msg* serverMsg = msgGet();
	string fileInPath = string("clientFiles\\").append(fileName);
	ofstream myFile(fileInPath, ios::out | ios::binary);
	while (serverMsg->type != RESP_ERR) {
		log(string("Received response with sequence number").append(to_string((_ULonglong)serverMsg->sequenceNumber)));
		myFile.write (serverMsg->buffer, serverMsg->length);
		setAckMessage(serverMsg);
		delete serverMsg;

		msgSend(&sendMsg);
		serverMsg = msgGet();
	}

	if(strcmp(serverMsg->buffer, "End of file.") == 0)
		cout << "File received "<< endl << endl;
	else
		cout << serverMsg->buffer << endl << endl;

	myFile.close();
	remove(fileName.c_str());
}

void FtpClient::performList()
{
	cout << endl << endl << "Listing directory contents" << endl;
	cout << "==================================================================" << endl;

	sendMsg.type = GET_LIST;
	sendMsg.length = 0;
	msgSend(&sendMsg);

	Msg* serverMsg = msgGet();
	while (serverMsg->type != RESP_ERR) {
		log(string("Received response with sequence number ").append(to_string((_ULonglong)serverMsg->sequenceNumber)));
		char* currentBuffer = serverMsg->buffer;
		for (int i = 0; i < serverMsg->length; i++)
			if (currentBuffer[i] == '\0')
				cout << endl;
			else
				cout << currentBuffer[i];

		setAckMessage(serverMsg);
		delete serverMsg;

		msgSend(&sendMsg);
		serverMsg = msgGet();
	}

	if(strcmp(serverMsg->buffer, "End of file.") == 0) {
		cout << endl << endl << "End of directory listing" << endl;
		cout << "==================================================================" << endl << endl;
	}
}

void FtpClient::terminate()
{
	sendMsg.type = TERMINATE;
	sendMsg.length = 0;
	msgSend(&sendMsg);

	// Set artificially high wait time to make sure the message is completely flushed prior to exiting.
	Sleep(250);

	exit(0);
}

void FtpClient::log(const std::string &logItem)
{
	time_t rawtime;
	struct tm * timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	FILE *logFile = fopen("logfile.txt", "a");
	fprintf(logFile, "Client (%d): %s --%s\r\n", clientIdentifier, logItem.c_str(), asctime(timeinfo));
	fclose(logFile);
}

void FtpClient::setAckMessage(Msg* previousPart)
{
	sendMsg.length = 0;
	sendMsg.type = PUT;
	sendMsg.sequenceNumber = previousPart->sequenceNumber;
}

/**
 * Function - showMenu
 * Usage: Display the Menu with options for the User to select based on the operation
 *
 * @arg: void
 */
void FtpClient::showMenu()
{
	int optionVal;
	cout << "1 : GET " << endl;
	cout << "2 : LIST " << endl;
	cout << "3 : EXIT " << endl;
	cout << "Please select the operation that you want to perform : ";
	
	/* Check if invalid value is provided and reset if cin error flag is set */
	if(!(cin >> optionVal) || optionVal < 1 || optionVal > 3)
	{
		cout << endl << "Input Types does not match " << endl;
		cin.clear();
	}
	
	cin.ignore(250, '\n');
	
	/* Based on the option selected by User, set the transfer type and invoke the appropriate function */
	switch (optionVal)
	{
		case 1:
			performGet();
			break;

		case 2:
			performList();
			break;

		case 3:
			cout << "Terminating... " << endl; 
			terminate();
			break;

		default:
			cout << endl << "Please select from one of the above options " << endl;
			break;
	}
	cout << endl;
}

/**
 * Function - startClient
 * Usage: Initialize WinSocket and get the host name and server IP Address to connnect
 *
 * @arg: void
 */
bool FtpClient::startClient()
{
	/* Initialize WinSocket */
	if (WSAStartup(0x0202,&wsaData)!=0)
	{
		WSACleanup();
	    cerr<<"Error in starting WSAStartup()";
		return false;
	}

	/* Get Host Name */
	if(gethostname(hostName,HOSTNAME_LENGTH)!=0) 
	{
		cerr<<"can not get the host name,program ";
		return false;
	}
	
	cout <<"ftp_udp starting on host: "<<hostName<<endl;
	cout <<"Type name of ftp client muxer: "<<endl;
	getline(cin,serverName);
	
	return performHandshake();
}

bool FtpClient::performHandshake()
{
	/* Socket Creation */
	if ((clientSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	{
		std::cerr << "Socket Creation Error; exit" << endl;
		exit(1);
	}

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(0);

	/* Binding the server socket to the Port Number */
    if (::bind(clientSock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		cerr << "Socket Binding Error from FtpThread, exit" << endl;
		return false;
	}

	memset(&ServAddr, 0, sizeof(ServAddr));
	ServAddr.sin_family = AF_INET;
	ServAddr.sin_addr.S_un.S_addr = ResolveName(serverName);
	ServAddr.sin_port = htons(HANDSHAKE_PORT);

	// TODO
	/* 2-way handshake w/ the muxer (reliable connection to muxer) to get client ID
	 * 3-way handshake as usual (unreliable connection from muxer to router)
	 */

	int sizeInt = sizeof(clientIdentifier);
	Payload* clientIdRequest = new Payload();
	clientIdRequest->length = sizeInt;
	memcpy(clientIdRequest->data, &clientIdentifier, sizeInt);
	rawSend(clientIdRequest);
	delete clientIdRequest;

	Payload* clientIdResponse = rawGet();
	memcpy(&clientIdentifier, clientIdResponse, sizeof(clientIdentifier));
	delete clientIdResponse;

	Msg* request = getInitialHandshakeMessage();
	msgSend(request);
	delete request;

	Msg* reply = msgGet();
	log(string("Received handshake from server. Message: ").append(reply->buffer));
	request = processFinalHandshakeMessage(reply);
	delete reply;

	msgSend(request);
	delete request;

	return true;
}

Msg* FtpClient::getInitialHandshakeMessage()
{
	Msg* request = new Msg();
	request->type = HANDSHAKE;
	request->length = BUFFER_LENGTH;
	memset(request->buffer, 0, BUFFER_LENGTH);
	strcpy(request->buffer, std::to_string((_ULonglong)clientIdentifier).c_str());
	
	return request;
}

Msg* FtpClient::processFinalHandshakeMessage(Msg *serverHandshake)
{
	serverIdentifier = serverHandshake->serverId;
	sequenceNumber = serverHandshake->sequenceNumber;
	
	Msg* request = new Msg();
	request->type = COMPLETE_HANDSHAKE;
	request->length = BUFFER_LENGTH;
	request->sequenceNumber = serverHandshake->sequenceNumber;
	memset(request->buffer, 0, BUFFER_LENGTH);
	
	return request;
}

/**
 * Destructor - ~FtpClient
 * Usage: DeAllocate the allocated memory
 *
 * @arg: void
 */
FtpClient::~FtpClient()
{
	/* When done uninstall winsock.dll (WSACleanup()) and return; */
	WSACleanup();
}

/**
 * Function - main
 * Usage: Initiates the Client
 *
 * @arg: int, char*
 */
int main(int argc, char *argv[])
{
	FtpClient * tc=new FtpClient();
	if (!tc->startClient())
		return 1;

	while (true)
	{
		tc->showMenu();
	}
		
	return 0;
}
