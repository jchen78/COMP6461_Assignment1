#pragma once
#include <queue>
#include "Thread.h"
#include "Common.h"

class ThreadedReceiver
{
private:
	std::queue<char*> completedSet;
	int totalPayloadSize;
	char *currentWindow[SEQUENCE_RANGE];
	int windowOrigin;
	bool isComplete;

	int receiveSocket;

	Msg* msgGet();
	void ackSend(int sequenceNumber);
	bool isEofMsg(Msg*);
	void handlePayload(Msg*);
public:
	ThreadedReceiver(int socket);
	char* getMultipartMessage();
	int getMessageLength();
	~ThreadedReceiver();
};