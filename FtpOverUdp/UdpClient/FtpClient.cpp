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
	srand(time(NULL));
	clientIdentifier = rand();
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
	int n;
	int expectedMsgLen = MSGHDRSIZE + msg_ptr->length;
	int addrLength = sizeof(ServAddr);
	if ((n = sendto(clientSock, (char *)msg_ptr, expectedMsgLen, 0, (SOCKADDR *)&ServAddr, addrLength)) != expectedMsgLen) {
		std::cerr << "Send MSGHDRSIZE+length Error " << endl;
		std::cerr << WSAGetLastError() << endl;
		return(-1);
	}

	return (n-MSGHDRSIZE);
}
/**
 * Function - msgGet
 * Usage: Blocks until the next incoming packet is completely received; returns the packet formatted as a message.
 *
 * @arg: void
 */
Msg* FtpClient::msgGet()
{
	char buffer[RCV_BUFFER_SIZE];
	int bufferLength;
	int addrLength = sizeof(ServAddr);

	/* Check the received Message Header */
	if ((bufferLength = recvfrom(clientSock, buffer, RCV_BUFFER_SIZE, 0, (SOCKADDR *)&ServAddr, &addrLength)) == SOCKET_ERROR)
	{
		cerr << "recvfrom(...) failed when getting message" << endl;
		cerr << WSAGetLastError() << endl;
		exit(1);
	}

	// must destruct after use!
	Msg* msg = new Msg();
	memcpy(msg, buffer, MSGHDRSIZE);
	memcpy(msg->buffer, buffer + MSGHDRSIZE, msg->length);
	return msg;
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

	sendMsg.type = REQ_GET;
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

	sendMsg.type = REQ_LIST;
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
	
	cout <<"ftp_tcp starting on host: "<<hostName<<endl;
	cout <<"Type name of ftp server: "<<endl;
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
		exit(1);
	}

	memset(&ServAddr, 0, sizeof(ServAddr));
	ServAddr.sin_family = AF_INET;
	ServAddr.sin_addr.S_un.S_addr = ResolveName(serverName);
	ServAddr.sin_port = htons(HANDSHAKE_PORT);

	Msg* request = getInitialHandshakeMessage();
	msgSend(request);

	struct sockaddr_in newServer;
	Msg* reply = msgGet();
	log(string("Received handshake from server. Message: ").append(reply->buffer));
	delete request;

	request = processFinalHandshakeMessage(reply);
	msgSend(request);
	delete request;
	delete reply;

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
	int startIndex = 0;
	for (; startIndex < serverHandshake->length && serverHandshake->buffer[startIndex] != ','; startIndex++);
	startIndex++;
	
	serverIdentifier = 0;
	Msg* request = new Msg();
	request->type = COMPLETE_HANDSHAKE;
	request->length = BUFFER_LENGTH;
	memset(request->buffer, 0, BUFFER_LENGTH);
	for (int i = 0; serverHandshake->buffer[startIndex + i] != '\0'; i++) {
		char currentDigit = serverHandshake->buffer[startIndex + i];
		serverIdentifier = serverIdentifier * 10 + (currentDigit - '0');
		request->buffer[i] = currentDigit;
	}
	
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
