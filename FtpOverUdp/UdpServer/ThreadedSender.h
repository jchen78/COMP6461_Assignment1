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
	class WorkerThread** currentWindow;
	int socket;

	void initializePayload(const char*, int);
	void normalizeCurrentWindow();
	void receiveAck();
	void finalizePayload();
public:
	ThreadedSender(int socket);
	void send(const char *messageContents, int messageLength);
	virtual ~ThreadedSender();
};

class WorkerThread : public Thread
{
private:
	int sourceSocket;
	char *packetContents;
	int packetLength;

	void sendMsg();
public:
	WorkerThread(int sendingSocket, char *packetContents, int packetLength) { }
	void run();
};
