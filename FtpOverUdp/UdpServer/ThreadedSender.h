#pragma once
#include "Thread.h"

#define WINDOW_SIZE 3
#define SEQUENCE_RANGE 7
#define DESTINATION_PORT 7000
#define TIMER_DELAY 1000
#define BUFFER_SIZE 256

class ThreadedSender
{
private:
	char *completePayload;
	int numberOfPackets;
	int payloadSize;
	int currentWindowOrigin;
	int currentWindowEnd;
	class WorkerThread** currentWindow;
	int socket;

	void initializePayload(const char*, int);
	void normalizeCurrentWindow();
	void receiveAck();
	void notifyPayloadComplete();
public:
	ThreadedSender(int socket);
	void send(const char *messageContents, int messageLength);
	virtual ~ThreadedSender();
};

class WorkerThread : Thread
{
private:
	int sourceSocket;
	char *packetContents;
	void sendMsg();
public:
	WorkerThread() { } // Null-object by default
	void initialize(int sendingSocket, char *packetContents);
	void run();
};
