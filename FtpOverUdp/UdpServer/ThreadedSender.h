#pragma once
#include "Thread.h"
#include "Common.h"

#define DESTINATION_PORT 7000
#define TIMER_DELAY 1000

class ThreadedSender
{
private:
	char *completePayload;
	int numberOfPackets;
	int payloadSize;
	int currentWindowOrigin;
	class SenderThread** currentWindow;
	bool *windowState[SEQUENCE_RANGE];
	
	int socket;
	struct sockaddr_in ServAddr;

	void initializePayload(const char*, int);
	void normalizeCurrentWindow();
	void receiveAck();
	void finalizePayload();

	Msg* msgGet();
public:
	ThreadedSender(int socket, struct sockaddr_in serverAddress);
	void send(const char *messageContents, int messageLength);
	virtual ~ThreadedSender();
};

class SenderThread : public Thread
{
private:
	int socket;
	struct sockaddr_in* destination;
	Msg* msg;
	bool* isAcked;

	void msgSend();
public:
	SenderThread(int sendingSocket, struct sockaddr_in* destinationAddress, bool* isAcked, Type messageType, int sequenceNumber, char *packetContents, int packetLength);
	void run();
	~SenderThread() { delete[] msg->buffer; delete msg; delete isAcked; }
};
