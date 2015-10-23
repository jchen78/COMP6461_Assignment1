#include "ThreadedReceiver.h"


ThreadedReceiver::ThreadedReceiver(int socket)
{
	isComplete = false;
	totalPayloadSize = 0;
	windowOrigin = 0;
	receiveSocket = socket;
	for (int i = 0; i < SEQUENCE_RANGE; i++)
		currentWindow[i] = NULL;
}

char* ThreadedReceiver::getMultipartMessage()
{
	while (!isComplete) {
		Msg* msgPtr = msgGet();
		isComplete = isEofMsg(msgPtr);

		if (!isComplete)
			handlePayload(msgPtr);
	}

	char* completePayload = new char[totalPayloadSize];
	for (int i = 0; i < totalPayloadSize; i += BUFFER_LENGTH) {
		int length = (i + BUFFER_LENGTH) < totalPayloadSize ? BUFFER_LENGTH : totalPayloadSize - i;
		char* currentPacket = completedSet.front();
		completedSet.pop();

		memcpy(&completePayload[i], currentPacket, length);

		delete[] currentPacket;
	}

	return completePayload;
}

bool ThreadedReceiver::isEofMsg(Msg* msg)
{
	if (msg->type != RESP_ERR)
		return false;

	return (strcmp(msg->buffer, "eof") == 0);
}

void ThreadedReceiver::handlePayload(Msg* msg)
{
	// ACK package
	int sequenceNumber = msg->sequenceNumber;
	ackSend(sequenceNumber);

	// Register package in buffer
	if (currentWindow[sequenceNumber] != NULL)
		return;

	int currentPayloadLength = msg->length;
	currentWindow[sequenceNumber] = new char[BUFFER_LENGTH];
	memcpy(currentWindow[sequenceNumber], msg->buffer, currentPayloadLength);
	totalPayloadSize += currentPayloadLength; // It doesn't matter if it's out of order.

	// Normalize buffer
	while (currentWindow[windowOrigin] != NULL) {
		completedSet.push(currentWindow[windowOrigin]);
		currentWindow[windowOrigin++] = NULL;
	}
}

int ThreadedReceiver::getMessageLength() {
	return totalPayloadSize;
}

Msg* ThreadedReceiver::msgGet() {
	return NULL;
}

void ThreadedReceiver::ackSend(int sequenceNumber) {

}

ThreadedReceiver::~ThreadedReceiver()
{
	delete[] currentWindow;
}
