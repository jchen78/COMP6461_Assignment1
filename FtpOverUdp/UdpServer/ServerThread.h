#pragma once

#include <Common.h>
#include <Thread.h>
#include <AsyncLock.h>
#include <Sender.h>
#include <Receiver.h>

using namespace Common;

typedef enum {
	INITIALIZING,
	STARTING_HANDSHAKE,
	WAITING_FOR_REQUEST,
	SENDING,
	RECEIVING,
	RENAMING
} ServerState;

class ServerThread : public Thread
{
private:
	// Basic thread configuration
	int log;
	int serverId;
	int socket;
	struct sockaddr_in address;
	char filesDirectory[13];
	AsyncLock* ioLock;

	// Communication with server multiplexer
	class AsyncLock* outerSync;
	ServerState currentState;
	struct Msg* currentMsg;
	bool* isActive;

	class Sender* sender;
	class Receiver* receiver;
	std::string filename;
	SenderThread* currentResponse;
	bool* isResponseComplete;
	int sequenceNumber;

	// Private methods to handle requests
	void initServerWithClientData();
	void startHandshake();
	void sendList();
	Payload* getDirectoryContents();
	void sendFile();
	Payload* getFileContents();
	void startSender(Payload* contents);
	void dispatchToSender();
	void getFile();
	void dispatchToReceiver();
	void saveFile(Payload* fileContents);

	void notifyWrongState();
	void resetToReadyState();
	void resetToReadyState(bool overrideSequenceNumber);

	void sendMsg(Msg*);
	void sendAck();
public:
	/* NOTE: ServerThread will need to share an I/O mutex */
	ServerThread(int serverId, int serverSocket, struct sockaddr_in clientAddress, bool *isActive, Msg *initialHandshake, AsyncLock *ioLock);
	int getId();
	AsyncLock* getSync();
	virtual void run();
	virtual ~ServerThread() {
		if (sender != NULL)
			delete sender;

		if (currentResponse != NULL)
			delete currentResponse;
	}
};
