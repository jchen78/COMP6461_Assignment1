#include "ServerThread.h"
#include <winsock.h>
#include <fstream>
#include <queue>
#include <string>
#include <Shlwapi.h>

using namespace std;

/*-------------------------------ServerThread Class--------------------------------*/
ServerThread::ServerThread(int serverId, int serverSocket, struct sockaddr_in clientAddress, bool* isActive, Msg* initialHandshake, AsyncLock* ioLock)
{
	this->serverId = serverId;
	this->socket = serverSocket;
	this->address = clientAddress;
	this->currentState = INITIALIZING;
	this->isActive = isActive;
	this->currentMsg = initialHandshake;
	this->ioLock = ioLock;
	this->outerSync = new AsyncLock(true, serverId);
	strcpy(this->filesDirectory, "serverFiles\\");
}

int ServerThread::getId() { return serverId; }

AsyncLock* ServerThread::getSync() { return outerSync; }

void ServerThread::run()
{
	do // NOTE : currentMsg can only be null if the initial handshake is null, or ServerThread sets it to NULL.
	{
		switch (currentState) {
		case INITIALIZING:
			currentState = STARTING_HANDSHAKE;
			break;
		case STARTING_HANDSHAKE:
			initServerWithClientData();
			startHandshake();
			break;
		case WAITING_FOR_REQUEST:
			switch (currentMsg->type) {
			case HANDSHAKE:
				startHandshake();
				break;
			case GET_LIST:
				sendList();
				break;
			case GET_FILE:
				sendFile();
				break;
			case POST:
				getFile();
				break;
			case PUT:
				startRename();
				break;
			case RESP_ERR:
				sendAck();
				break;
			default:
				// TODO: Log
				break;
			}
			break;
		case SENDING:
			if (currentMsg->type == ACK)
				dispatchToSender();
			break;
		case RECEIVING:
			if (currentMsg->type == RESP || currentMsg->type == RESP_ERR && strcmp(currentMsg->buffer, "EOF") == 0)
				dispatchToReceiver();
			else if (currentMsg->type == POST && sequenceNumber == sequenceNumber)
				sendAck();
			break;
		case RENAMING:
			if (currentMsg->sequenceNumber == sequenceNumber) {
				switch (currentMsg->type) {
				case PUT:
					sendAck();
					break;
				case RESP:
					notifyFilenameCollision();
					break;
				}
			} else if (currentMsg->sequenceNumber == (sequenceNumber + 1) % SEQUENCE_RANGE) {
				if (currentMsg->type == RESP)
					performRename();
				else if (currentMsg->type == RESP_ERR) {
					filename = "";
					sequenceNumber = currentMsg->sequenceNumber;
					currentState = WAITING_FOR_REQUEST;
				}
			}
			
			break;
		}

		outerSync->finalizeConsumption();
		outerSync->waitForConsumption();
	} while (*isActive);

	sender->terminateCurrentTransmission();
	receiver->terminateCurrentTransmission();
	outerSync->finalizeConsumption();
	delete this;
}

void ServerThread::initServerWithClientData()
{
	sender = new Sender(socket, serverId, currentMsg->clientId, address);
	receiver = new Receiver(socket, serverId, currentMsg->clientId, address);
	currentState = WAITING_FOR_REQUEST;
}

void ServerThread::startHandshake()
{
	Msg handshakeReply;
	handshakeReply.type = HANDSHAKE;
	handshakeReply.sequenceNumber = serverId % SEQUENCE_RANGE;
	handshakeReply.length = currentMsg->length;
	memcpy(handshakeReply.buffer, currentMsg->buffer, handshakeReply.length);
	sendMsg(&handshakeReply);
}

void ServerThread::sendList()
{
	currentState = SENDING;
	sequenceNumber = currentMsg->sequenceNumber;
	startSender(getDirectoryContents());
}

Payload* ServerThread::getDirectoryContents()
{
	ioLock->waitForSignalling();

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
			char* currentFilename = new char[20];
			strcpy(currentFilename, ffd.cFileName);
			payloadData.push(currentFilename);
			payloadLength += strlen(currentFilename) + 1;
		}
	} while (FindNextFile(hFind, &ffd) != 0);

	Payload* payloadContent = new Payload();
	payloadContent->length = payloadLength;
	payloadContent->data = new char[payloadLength];
	int currentIndex = 0;
	while (!payloadData.empty()) {
		strcpy(payloadContent->data + currentIndex, payloadData.front());
		currentIndex += strlen(payloadData.front()) + 1;
		if (currentIndex > payloadLength)
			throw exception("More files than expected. Please retry");

		payloadData.pop();
	}

	ioLock->finalizeSignalling();
	ioLock->finalizeConsumption();

	return payloadContent;
}

void ServerThread::sendFile()
{
	currentState = SENDING;
	sequenceNumber = currentMsg->sequenceNumber;
	try { startSender(getFileContents()); }
	catch (exception e) {
		currentState = WAITING_FOR_REQUEST;
		sequenceNumber = (currentMsg->sequenceNumber + 1) % SEQUENCE_RANGE;
		Msg errMsg = Msg();
		errMsg.type = RESP_ERR;
		errMsg.length = 4;
		errMsg.sequenceNumber = sequenceNumber;
		strcpy(errMsg.buffer, "NSF");
		sendMsg(&errMsg);
	}
}

Payload* ServerThread::getFileContents()
{
	ioLock->waitForSignalling();

	char* c_filename = new char[currentMsg->length + 1];
	memcpy(c_filename, currentMsg->buffer, currentMsg->length);
	c_filename[currentMsg->length] = '\0';
	string fullFileName = string(filesDirectory).append(c_filename);
	WIN32_FIND_DATA data;
	HANDLE h = FindFirstFile(fullFileName.c_str(), &data);
	if (h == INVALID_HANDLE_VALUE) {
		throw exception("File not found.");
		return NULL;
	}

	FindClose(h);
	int size = data.nFileSizeLow;

	ifstream fileToRead(fullFileName, ios::binary);
	Payload* contents = new Payload();
	contents->length = size;
	contents->data = new char[size];
	fileToRead.read(contents->data, size);
	fileToRead.close();

	ioLock->finalizeSignalling();
	ioLock->finalizeConsumption();

	return contents;
}

void ServerThread::startSender(Payload* payloadData)
{
	sender->initializePayload(payloadData->data, payloadData->length, currentMsg->sequenceNumber, currentMsg);
	sender->send();
}

void ServerThread::getFile() {
	currentState = RECEIVING;
	sequenceNumber = currentMsg->sequenceNumber;
	receiver->startNewPayload(sequenceNumber);
	sendAck();
	filename = string(currentMsg->buffer, currentMsg->buffer + currentMsg->length);
}

void ServerThread::dispatchToReceiver() {
	receiver->handleMsg(currentMsg);
	if (receiver->isPayloadComplete()) {
		sequenceNumber = receiver->finalSequenceNumber();
		currentState = WAITING_FOR_REQUEST;
		saveFile(receiver->getPayload());
	}
}

void ServerThread::startRename() {
	filename = string(filesDirectory).append(currentMsg->buffer);
	sequenceNumber = currentMsg->sequenceNumber;

	ioLock->waitForSignalling();

	if (PathFileExists(filename.c_str()))
		notifyFileError();
	else {
		sendAck();
		currentState = RENAMING;
	}

	ioLock->finalizeSignalling();
	ioLock->finalizeConsumption();
}

void ServerThread::performRename() {
	string newFilename = string(filesDirectory).append(currentMsg->buffer);
	sequenceNumber = currentMsg->sequenceNumber;

	ioLock->waitForSignalling();

	if (PathFileExists(newFilename.c_str()))
		notifyFileError();
	else
		rename(filename.c_str(), newFilename.c_str());

	ioLock->finalizeSignalling();
	ioLock->finalizeConsumption();
}

void ServerThread::notifyFileError() {
	Msg errorMessage = Msg();
	memset(&errorMessage, '\0', sizeof(errorMessage));
	errorMessage.type = RESP_ERR;
	errorMessage.length = 4;

	strcpy(errorMessage.buffer, "FAE");

	sendMsg(&errorMessage);
}

void ServerThread::dispatchToSender()
{
	std::cout << "ServerThread: ACK #" << std::to_string(currentMsg->sequenceNumber) << endl;
	sender->processAck();
	if (sender->isPayloadSent()) {
		sequenceNumber = sender->finalSequenceNumber();
		currentState = WAITING_FOR_REQUEST;
	}
}

void ServerThread::saveFile(Payload* fileContents) {
	ioLock->waitForSignalling();

	// Set filename
	std::string completeFilename = string(filesDirectory).append(filename);
	const char* extension = PathFindExtension(filename.c_str());
	while (PathFileExists(completeFilename.c_str())) {
		const char* currentFilenameTemp = completeFilename.c_str();
		char *currentFilename = new char[strlen(currentFilenameTemp)];
		strcpy(currentFilename, currentFilenameTemp);
		PathRemoveExtension(currentFilename);
		size_t position = filename.find(".");
		completeFilename = string(currentFilename).append("_").append(extension);
	}

	{
		std::ofstream writer(completeFilename, ios::out | ios::binary);
		writer.write(fileContents->data, fileContents->length);
	}

	ioLock->finalizeSignalling();
	ioLock->finalizeConsumption();
}

void ServerThread::sendAck() {
	Msg ack = Msg();
	ack.length = 0;
	ack.type = ACK;
	ack.sequenceNumber = currentMsg->sequenceNumber;
	sendMsg(&ack);
}

void ServerThread::sendMsg(Msg* msg) {
	msg->clientId = currentMsg->clientId;
	msg->serverId = serverId;

	int n;
	int expectedMsgLen = MSGHDRSIZE + msg->length;
	int addrLength = sizeof(address);
	if ((n = sendto(socket, (char *)msg, expectedMsgLen, 0, (SOCKADDR *)&address, addrLength)) != expectedMsgLen) {
		std::cerr << "Send MSGHDRSIZE+length Error " << endl;
		std::cerr << WSAGetLastError() << endl;
	}
}