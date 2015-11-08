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
	this->sender = new Sender(socket, serverId, currentMsg->clientId, address);
	this->receiver = new Receiver(socket, serverId, currentMsg->clientId, address);
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
			case POST:
				getFile();
				break;
			case PUT:
				// TODO: Complete action
				break;
			case TERMINATE:
				// TODO: Send ACK
				currentState = EXITING;
				break;
			default:
				// TODO: Send RESP_ERR
				break;
			}
			break;
		case SENDING:
			if (currentMsg->type == ACK) {
				dispatchToSender();
			}
			else {
				// TODO: Send RESP_ERR
			}
			break;
		case RECEIVING:
			if (currentMsg->type == RESP || currentMsg->type == RESP_ERR && strcmp(currentMsg->buffer, "EOF") == 0) {
				dispatchToReceiver();
			}
			else {
				// TODO: Send RESP_ERR
			}
			break;
		case RENAMING:
			if (currentMsg->type == RESP) {
				// TODO: Complete action
			}
			else if (currentMsg->type == TERMINATE && strcmp(currentMsg->buffer, filename.c_str()) == 0) {
				// TODO: Complete action
			}
			else {
				// TODO: Send RESP_ERR
			}
			break;
		case EXITING:
			if (currentMsg->type == ACK) {
				// TODO: Complete action (incl. set state to terminated)
				currentState = TERMINATED;
			}
			else {
				// TODO: Send RESP_ERR
			}
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
	currentResponse = new SenderThread(socket, serverId, currentMsg->clientId, &address, isResponseComplete, HANDSHAKE, serverId % SEQUENCE_RANGE, "", 0);
	currentResponse->start();
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

Payload* ServerThread::getDirectoryContents()
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
			char* currentFilename = new char[20];
			memcpy(currentFilename, ffd.cFileName, 20);
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
			throw new exception("More files than expected. Please retry");

		payloadData.pop();
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

Payload* ServerThread::getFileContents(const char* fileName)
{
	ifstream fileToRead(string(filesDirectory).append(fileName), ios::binary);
	if (!fileToRead)
		throw new exception("File not found");

	int size = 256 * 5;
	fileToRead.seekg(0, fileToRead.beg);

	Payload* contents = new Payload();
	contents->length = size;
	contents->data = new char[size];
	fileToRead.read(contents->data, size);
	fileToRead.close();

	return contents;
}

void ServerThread::startSender(Payload* payloadData)
{
	sender->initializePayload(payloadData->data, payloadData->length, currentMsg->sequenceNumber, currentMsg);
	sender->send();
}

void ServerThread::getFile() {
	currentState = RECEIVING;
	receiver->startNewPayload(currentMsg->sequenceNumber);
	filename = string(currentMsg->buffer);
}

void ServerThread::dispatchToReceiver() {
	receiver->handleMsg(currentMsg);
	if (receiver->isPayloadComplete()) {
		currentState = WAITING_FOR_REQUEST;
		saveFile(receiver->getPayload());
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
	ioLock->waitForConsumption();
	ioLock->finalizeConsumption();
}

void ServerThread::dispatchToSender()
{
	std::cout << "ServerThread: ACK #" << std::to_string(currentMsg->sequenceNumber) << endl;
	sender->processAck();
	if (sender->isPayloadSent())
		currentState = WAITING_FOR_REQUEST;
}
