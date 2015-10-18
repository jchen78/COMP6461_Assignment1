#include <string.h>
#include "ThreadedSender.h"


ThreadedSender::ThreadedSender(int socket)
{
	this->socket = socket;
	completePayload = NULL;
}

void ThreadedSender::send(const char* messageContents, int messageLength)
{
	initializePayload(messageContents, messageLength);
	
	do
	{
		// loop: normalize the current window
		normalizeCurrentWindow();
		receiveAck();
	} while (currentWindowOrigin < numberOfPackets);

	notifyPayloadComplete();
}

void ThreadedSender::initializePayload(const char* messageContents, int messageLength)
{
	numberOfPackets = messageLength / BUFFER_SIZE;
	payloadSize = messageLength;
	if (messageLength % BUFFER_SIZE > 0)
		numberOfPackets++;

	completePayload = new char[numberOfPackets * BUFFER_SIZE];
	memcpy(completePayload, messageContents, messageLength);
	currentWindow = new WorkerThread*[SEQUENCE_RANGE];
	for (int i = 0; i < SEQUENCE_RANGE; i++)
		currentWindow[i] = NULL;

	currentWindowOrigin = 0;
	currentWindowEnd = WINDOW_SIZE - 1;
	if (currentWindowEnd >= numberOfPackets)
		currentWindowEnd = numberOfPackets - 1;
}

void ThreadedSender::normalizeCurrentWindow()
{
	// Assume the indices are correct and all expired workers are set to NULL;
	// merely ensure that all slots in the current window are populated.
}

void ThreadedSender::receiveAck()
{
	// must handle indices, and set the proper slots to NULL
}

void ThreadedSender::notifyPayloadComplete()
{
	// must also wait for an ACK before exiting.
}

ThreadedSender::~ThreadedSender()
{
	if (completePayload != NULL)
		delete[] completePayload;
}

void WorkerThread::run()
{

}
