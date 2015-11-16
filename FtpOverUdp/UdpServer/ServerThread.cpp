#include "ServerThread.h"
#include <winsock.h>
#include <fstream>
#include <queue>
#include <string>
#include <Shlwapi.h>

using namespace std;

/*-------------------------------ServerThread Class--------------------------------*/
ServerThread::ServerThread(int serverId, int serverSocket, struct sockaddr_in clientAddress, Msg* initialHandshake, AsyncLock* ioLock) :
outerSync(true, serverId)
{
	this->serverId = serverId;
	this->socket = serverSocket;
	this->address = clientAddress;
	this->currentState = INITIALIZING;
	this->currentMsg = initialHandshake;
	this->ioLock = ioLock;
	strcpy(this->filesDirectory, "serverFiles\\");
}

int ServerThread::getId() { return serverId; }

AsyncLock* ServerThread::getSync() { return &outerSync; }

void ServerThread::run()
{
	do // NOTE : currentMsg can only be null if the initial handshake is null, or ServerThread sets it to NULL.
	{
		switch (currentState) {
		case INITIALIZING:
			currentState = STARTING_HANDSHAKE;
			break;
		case STARTING_HANDSHAKE:
			startHandshake();
			break;
		case HANDSHAKING:
			if (currentMsg->type == COMPLETE_HANDSHAKE)
				endHandshake();
			else if (currentMsg->type == HANDSHAKE)
				notifyWrongState();
			else
				resetToReadyState(true);
			break;
		case WAITING_FOR_REQUEST:
			switch (currentMsg->type) {
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
				// TODO: Complete action
				break;
			case TERMINATE:
				if (currentMsg->length == 0)
					terminate();
				break;
			default:
				notifyWrongState();
				break;
			}
			break;
		case WAITING_FOR_ACK:
			if (currentMsg->type == ACK && currentMsg->sequenceNumber == sequenceNumber)
				currentState = WAITING_FOR_REQUEST;
			else
				notifyWrongState();

			break;
		case SENDING:
			if (currentMsg->type == ACK)
				dispatchToSender();
			else if (currentMsg->type == TERMINATE) {
				if (currentMsg->length == 0)
					terminate();
				else
					resetToReadyState();
			} else if (currentMsg->type != POST && currentMsg->sequenceNumber != sequenceNumber)
				notifyWrongState();

			break;
		case RECEIVING:
			if (currentMsg->type == RESP || currentMsg->type == RESP_ERR && strcmp(currentMsg->buffer, "EOF") == 0)
				dispatchToReceiver();
			else if (currentMsg->type == POST && sequenceNumber == sequenceNumber)
				sendAck();
			else if (currentMsg->type == TERMINATE) {
				if (currentMsg->length == 0)
					terminate();
				else
					resetToReadyState();
			} else
				notifyWrongState();

			break;
		case RENAMING:
			if (currentMsg->type == RESP) {
				// TODO: Complete action
			} else if (currentMsg->type == TERMINATE) {
				if (currentMsg->length == 0)
					terminate();
				else
					resetToReadyState();
			} else
				notifyWrongState();
			break;
		case EXITING:
			if (currentMsg->type == ACK)
				currentState = TERMINATED;
			else
				notifyWrongState();

			break;
		}

		outerSync.finalizeConsumption();
		if (currentState != TERMINATED)
			outerSync.waitForConsumption();
	} while (currentState != TERMINATED);

	delete this;
}

void ServerThread::startHandshake()
{
	currentState = HANDSHAKING;
	isResponseComplete = new bool(false);
	sender = new Sender(socket, serverId, currentMsg->clientId, address);
	receiver = new Receiver(socket, serverId, currentMsg->clientId, address);
	currentResponse = new SenderThread(socket, serverId, currentMsg->clientId, &address, isResponseComplete, HANDSHAKE, serverId % SEQUENCE_RANGE, "", 0);
	currentResponse->start();
}

void ServerThread::endHandshake()
{
	currentState = WAITING_FOR_REQUEST;
	*isResponseComplete = true; // Will stop sending HANDSHAKE after timeout (SenderThread will auto-destroy)
	currentResponse = NULL;
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
		isResponseComplete = new bool(false);
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

	char* c_filename = new char[currentMsg->length];
	memcpy(c_filename, currentMsg->buffer, currentMsg->length);
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

void ServerThread::notifyWrongState() {
	Msg errorNotification;
	errorNotification.type = TYPE_ERR;
	errorNotification.sequenceNumber = sequenceNumber;
	errorNotification.length = 4;
	switch (currentState) {
	case HANDSHAKING:
		memcpy(errorNotification.buffer, "HSK", 4);
		break;
	case WAITING_FOR_REQUEST:
		memcpy(errorNotification.buffer, "WFR", 4);
		break;
	case WAITING_FOR_ACK:
		memcpy(errorNotification.buffer, "WFA", 4);
		break;
	case SENDING:
		memcpy(errorNotification.buffer, "SND", 4);
		break;
	case RECEIVING:
		memcpy(errorNotification.buffer, "RCV", 4);
		break;
	case RENAMING:
		memcpy(errorNotification.buffer, "RNM", 4);
		break;
	case EXITING:
		memcpy(errorNotification.buffer, "EXT", 4);
		break;
	}

	sendMsg(&errorNotification);
}

void ServerThread::resetToReadyState() {
	resetToReadyState(false);
}

void ServerThread::resetToReadyState(bool ignoreSequenceNumber) {
	if (!ignoreSequenceNumber && currentMsg->sequenceNumber != (sequenceNumber + 1) % SEQUENCE_RANGE)
		return;

	sequenceNumber = currentMsg->sequenceNumber;
	*isResponseComplete = true;
	switch (currentState) {
	case SENDING:
		sender->terminateCurrentTransmission();
		break;
	case RECEIVING:
		receiver->terminateCurrentTransmission();
		break;
	}

	currentState = WAITING_FOR_REQUEST;
}

void ServerThread::terminate() {
	resetToReadyState();
	currentState = EXITING;
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