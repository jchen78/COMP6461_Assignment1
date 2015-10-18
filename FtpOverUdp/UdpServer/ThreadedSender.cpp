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
		normalizeCurrentWindow();
		receiveAck();
	} while (currentWindowOrigin < numberOfPackets);

	finalizePayload();
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
}

void ThreadedSender::normalizeCurrentWindow()
{
	// Assume the indices are correct and all expired workers are set to NULL;
	// merely ensure that all slots in the current window are populated.
	for (int i = 0; i < WINDOW_SIZE && currentWindowOrigin + i < numberOfPackets; i++) {
		int currentIndex = (currentWindowOrigin + i) % SEQUENCE_RANGE;
		if (currentWindow[currentIndex] == NULL) {
			int currentChar = currentIndex * BUFFER_SIZE;
			int currentSize = currentIndex == (numberOfPackets - 1) ? (payloadSize - currentChar - 1) : BUFFER_SIZE;
			currentWindow[currentIndex] = new WorkerThread(socket, &completePayload[currentChar], currentSize);
			currentWindow[currentIndex]->start();
		}
	}
}

void ThreadedSender::receiveAck()
{
	// must handle indices, and set the proper slots to NULL
}

void ThreadedSender::finalizePayload()
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
