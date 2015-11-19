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
	filesDirectory = string("clientFiles\\");
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
	payload->data = new char[payload->length];
	memcpy(payload->data, (char*)msg_ptr, payload->length);
	int n = rawSend(payload);
	delete payload->data;
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

int FtpClient::waitTimeOut() {
	timeval timeout;
	timeout.tv_usec = 2 * 1000 * TIMER_DELAY;
	fd_set socket_set;
	FD_ZERO(&socket_set);
	FD_SET(clientSock, &socket_set);
	return select(0, &socket_set, 0, 0, &timeout);
}

void FtpClient::msgGet(Msg* msg) {
	Payload* data = rawGet();
	char* buffer = data->data;
	
	memset(msg->buffer, '\0', BUFFER_LENGTH);
	memcpy(msg, buffer, MSGHDRSIZE);
	memcpy(msg->buffer, buffer + MSGHDRSIZE, msg->length);

	delete data;
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
	data->data = new char[bufferLength];
	memcpy(data->data, buffer, bufferLength);
	return data;
}

void FtpClient::sendRstMessage() {
	Msg msg;
	msg.type = TERMINATE;
	msg.length = 4;
	msg.sequenceNumber = sequenceNumber;
	strcpy(msg.buffer, "RST");
	msgSend(&msg);
}

/**
 * Function - getOperation
 * Usage: Establish connection and retrieve file from server
 *
 * @arg: void
 */
void FtpClient::performGet()
{ 
	string filename;
	cout << "Type name of file to be retrieved: ";
	cin >> filename;

	receiver->startNewPayload(sequenceNumber);
	bool* isAcked = new bool(false);
	char c_filename[20];
	strcpy(c_filename, filename.c_str());
	Common::SenderThread* requestSender = new Common::SenderThread(clientSock, serverIdentifier, clientIdentifier, &ServAddr, isAcked, GET_FILE, sequenceNumber, c_filename, (int)filename.length());
	requestSender->start();

	Msg serverMsg;
	while (!receiver->isPayloadComplete()) {
		msgGet(&serverMsg);
		log(string("Received response with sequence number").append(to_string((_ULonglong)serverMsg.sequenceNumber)));
		switch (serverMsg.type) {
		case RESP:
			if (serverMsg.sequenceNumber == sequenceNumber)
				*isAcked = true;
			// No break: fall-through intended
		case RESP_ERR:
			try { receiver->handleMsg(&serverMsg); }
			catch (exception e) {
				if (serverMsg.sequenceNumber == (sequenceNumber + 1) % SEQUENCE_RANGE && strcmp(e.what(), "No such file.") == 0) {
					*isAcked = true;
					receiver->terminateCurrentTransmission();
					cout << "No such file." << endl;
					return;
				}

				throw e;
			}
			break;
		case TYPE_ERR:
			if (serverMsg.sequenceNumber == sequenceNumber && strcmp(serverMsg.buffer, "WFR") == 0)
				break;

			receiver->terminateCurrentTransmission();
			*isAcked = true;
			sendRstMessage();
			cout << "Transmission failed (server was not ready). Please retry." << endl;
			sequenceNumber = (serverMsg.sequenceNumber + 1) % SEQUENCE_RANGE;
			return;
		}
	}

	sequenceNumber = (serverMsg.sequenceNumber + 1) % SEQUENCE_RANGE;

	// TODO: Test by re-sending EOF message.
	{
		Msg finalAck;
		finalAck.length = 0;
		finalAck.type = ACK;

		while (waitTimeOut() > 0) {
			msgGet(&serverMsg);
			if (serverMsg.type == RESP_ERR && strcmp(serverMsg.buffer, "EOF") == 0) {
				finalAck.sequenceNumber = serverMsg.sequenceNumber;
				msgSend(&finalAck);
			}
		}
	}

	// Write to file
	string fileInPath = string(filesDirectory).append(filename);
	Payload* payload = receiver->getPayload();
	{
		ofstream myFile(fileInPath, ios::out | ios::binary);
		myFile.write(payload->data, payload->length);
	}
}

void FtpClient::performList()
{
	receiver->startNewPayload(sequenceNumber);
	bool* isAcked = new bool(false);
	char* message = NULL;
	Common::SenderThread* requestSender = new Common::SenderThread(clientSock, serverIdentifier, clientIdentifier, &ServAddr, isAcked, GET_LIST, sequenceNumber, message, 0);
	requestSender->start();

	Msg serverMsg;
	while (!receiver->isPayloadComplete()) {
		msgGet(&serverMsg);
		log(string("Received response with sequence number").append(to_string((_ULonglong)serverMsg.sequenceNumber)));
		switch (serverMsg.type) {
		case RESP:
			if (serverMsg.sequenceNumber == sequenceNumber)
				*isAcked = true;
		case RESP_ERR:
			receiver->handleMsg(&serverMsg);
			break;
		case TYPE_ERR:
			receiver->terminateCurrentTransmission();
			*isAcked = true;
			sendRstMessage();
			cout << "Transmission failed (server was not ready). Please retry." << endl;
			sequenceNumber = (serverMsg.sequenceNumber + 1) % SEQUENCE_RANGE;
			return;
		}
	}

	sequenceNumber = serverMsg.sequenceNumber + 1 % SEQUENCE_RANGE;
	Payload* payload = receiver->getPayload();
	char* directoryContent = payload->data;
	int payloadLength = payload->length;

	{
		Msg finalAck;
		finalAck.length = 0;
		finalAck.type = ACK;

		while (waitTimeOut() > 0) {
			msgGet(&serverMsg);
			if (serverMsg.type == RESP_ERR && strcmp(serverMsg.buffer, "EOF") == 0) {
				finalAck.sequenceNumber = serverMsg.sequenceNumber;
				msgSend(&finalAck);
			}
		}
	}

	cout << endl << endl << "Listing directory contents" << endl;
	cout << "==================================================================" << endl;

	for (int i = 0; i < payloadLength; i++) {
		if (directoryContent[i] == 0)
			cout << endl;
		else
			cout << directoryContent[i];
	}

	cout << endl << endl << "End of directory listing" << endl;
	cout << "==================================================================" << endl << endl;
}

void FtpClient::performUpload()
{
	string filename;
	cout << "Please enter the file to upload: ";
	getline(cin, filename);

	string fullFileName = string(filesDirectory).append(filename);
	WIN32_FIND_DATA data;
	HANDLE h = FindFirstFile(fullFileName.c_str(), &data);
	if (h == INVALID_HANDLE_VALUE)
		throw exception("File not found.");

	FindClose(h);
	int size = data.nFileSizeLow;

	ifstream fileToRead(fullFileName, ios::binary);
	//fileToRead.seekg(0, fileToRead.beg);
	Payload* contents = new Payload();
	contents->length = size;
	contents->data = new char[size];
	fileToRead.read(contents->data, size);
	fileToRead.close();
	
	bool* isAcked = new bool(false);
	int filenameLength = filename.length();
	char* c_filename = new char[filenameLength];
	strcpy(c_filename, filename.c_str());
	Common::SenderThread* requestMessage = new Common::SenderThread(clientSock, serverIdentifier, clientIdentifier, &ServAddr, isAcked, POST, sequenceNumber, c_filename, filenameLength);
	requestMessage->start();

	Msg msg;
	do {
		msgGet(&msg);
	} while (msg.type != ACK || msg.sequenceNumber != sequenceNumber);
	*isAcked = true;

	sender->initializePayload(contents->data, contents->length, sequenceNumber, &msg);
	delete contents;

	sender->send();
	while (!sender->isPayloadSent()) {
		msgGet(&msg);
		if (msg.type == ACK)
			sender->processAck();
		else if (msg.type == TYPE_ERR) {
			sender->terminateCurrentTransmission();
			if (msg.sequenceNumber != sender->finalSequenceNumber() || strcmp(msg.buffer, "WFR") != 0) {
				sendRstMessage();
				cout << "Transmission failed (server was not ready). Please retry." << endl;
				break;
			}
		}
	}

	sequenceNumber = sender->finalSequenceNumber();
}

void FtpClient::terminate()
{
	bool isAcked = false;
	Msg* msg;
	Common::SenderThread* terminationStart = new Common::SenderThread(clientSock, serverIdentifier, clientIdentifier, &ServAddr, &isAcked, TERMINATE, sequenceNumber, NULL, 0);
	terminationStart->start();

	do {
		msgGet(msg);
	} while (msg->sequenceNumber != sequenceNumber || msg->type != ACK);
	isAcked = true;

	delete msg;
	msg = new Msg();
	msg->sequenceNumber = sequenceNumber;
	msg->length = 0;
	msg->type = ACK;
	msgSend(msg); // Unreliability OK: server will exit on timeout

	// Set artificially high wait time to make sure the message is completely flushed prior to exiting.
	Sleep(250);
	exit(0);
}

void FtpClient::resyncServer()
{
	// Clear out message queue
	Payload* dequeuedPayload;
	while (waitTimeOut() > 0) {
		dequeuedPayload = rawGet();
		delete dequeuedPayload;
	}

	// Reset server to ready state
	sequenceNumber = (sequenceNumber + 1) % SEQUENCE_RANGE;
	Msg response;
	response.sequenceNumber = -1;
	do {
		sendRstMessage();
		if (waitTimeOut() > 0)
			msgGet(&response);
	} while (response.sequenceNumber != sequenceNumber || response.type != TYPE_ERR || strcmp(response.buffer, "WFR") != 0);

	// Clear out message queue
	while (waitTimeOut() > 0) {
		dequeuedPayload = rawGet();
		delete dequeuedPayload;
	}
}

void FtpClient::log(const std::string &logItem)
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
	fprintf(logFile, "Client (%d): %s --%s\r\n", clientIdentifier, formattedTime, logItem.c_str());
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
	int lastSequenceNumber = sequenceNumber;
	sequenceNumber = (sequenceNumber + 1) % SEQUENCE_RANGE;
	int optionVal;
	cout << "1 : GET " << endl;
	cout << "2 : LIST " << endl;
	cout << "3 : UPLOAD " << endl;
	cout << "4 : RENAME " << endl;
	cout << "5 : EXIT " << endl;
	cout << "Please select the operation that you want to perform : ";
	
	/* Check if invalid value is provided and reset if cin error flag is set */
	if(!(cin >> optionVal) || optionVal < 1 || optionVal > 5)
	{
		cout << endl << "Input types does not match " << endl;
		cin.clear();
	}
	
	cin.ignore(250, '\n');
	
	// Clean up all messages in the buffer
	resyncServer();
	sequenceNumber = (sequenceNumber + 1) % SEQUENCE_RANGE;

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
			performUpload();
			break;

		case 5:
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
	cout <<"Type the host name for the ftp client multiplexer: ";
	getline(cin,serverName);
	
	if (!performHandshake())
		return false;

	receiver = new Common::Receiver(clientSock, serverIdentifier, clientIdentifier, ServAddr);
	sender = new Common::Sender(clientSock, serverIdentifier, clientIdentifier, ServAddr);

	return true;
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
	clientIdRequest->data = new char[4];
	memcpy(clientIdRequest->data, &clientIdentifier, sizeInt);
	rawSend(clientIdRequest);
	delete clientIdRequest;

	Payload* clientIdResponse = rawGet();
	memcpy(&clientIdentifier, clientIdResponse->data, sizeof(clientIdentifier));
	delete[] clientIdResponse->data;
	delete clientIdResponse;

	bool* isHandshakeAcked = new bool(false);
	Common::SenderThread* handshakeSender = new Common::SenderThread(clientSock, -1, clientIdentifier, &ServAddr, isHandshakeAcked, HANDSHAKE, 0, NULL, 0);
	handshakeSender->start();

	Msg reply;
	msgGet(&reply);
	*isHandshakeAcked = true;
	log(string("Received handshake from server. Message: ").append(reply.buffer));
	Msg* request = processFinalHandshakeMessage(&reply);

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
		tc->showMenu();
		
	return 0;
}
