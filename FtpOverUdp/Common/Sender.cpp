#include "stdafx.h"
#include <string.h>
#include <winsock.h>
#include "Sender.h"

using namespace std;

namespace Common
{
	Sender::Sender(int socket, int serverId, int clientId, struct sockaddr_in destinationAddress)
	{
		this->socket = socket;
		this->DestAddr = destinationAddress;
		this->serverId = serverId;
		this->clientId = clientId;
		
		completePayload = NULL;
	}

	void Sender::initializePayload(const char* messageContents, int messageLength, int firstSequenceNumber, Msg* ackMsg)
	{
		if (messageContents == NULL)
			throw new exception("Payload data cannot be null");

		numberOfPackets = messageLength / BUFFER_LENGTH;
		payloadSize = messageLength;
		if (messageLength % BUFFER_LENGTH > 0)
			numberOfPackets++;

		completePayload = new char[numberOfPackets * BUFFER_LENGTH];
		memcpy(completePayload, messageContents, messageLength);
		for (int i = 0; i < SEQUENCE_RANGE; i++)
			currentWindow[i] = NULL;

		currentWindowOrigin = 0;
		isComplete = false;

		sequenceSeed = firstSequenceNumber;
		currentAck = ackMsg;
	}

	void Sender::send()
	{
		normalizeCurrentWindow();
	}

	void Sender::normalizeCurrentWindow()
	{
		// Assume the indices are correct and all expired workers are set to NULL;
		// merely ensure that all slots in the current window are populated.
		for (int i = 0; i < WINDOW_SIZE && currentWindowOrigin + i < numberOfPackets; i++) {
			int currentIndex = (currentWindowOrigin + sequenceSeed + i) % SEQUENCE_RANGE;
			int currentPayloadIndex = (currentWindowOrigin + i) % SEQUENCE_RANGE;
			if (currentWindow[currentIndex] == NULL) {
				bool* currentFlag = new bool(false);
				int currentChar = currentPayloadIndex * BUFFER_LENGTH;
				int currentSize = currentPayloadIndex == (numberOfPackets - 1) ? (payloadSize - currentChar - 1) : BUFFER_LENGTH;
				windowState[currentIndex] = currentFlag;
				currentWindow[currentIndex] = new SenderThread(socket, serverId, clientId, &DestAddr, currentFlag, RESP, currentIndex, &completePayload[currentChar], currentSize);
				currentWindow[currentIndex]->start();
			}
		}
	}

	void Sender::processAck()
	{
		int index = currentAck->sequenceNumber;
		*windowState[index] = true;
		currentWindow[index] = NULL;

		if (currentWindowOrigin == numberOfPackets) {
			isComplete = index == (currentWindowOrigin + sequenceSeed) % SEQUENCE_RANGE;
			return;
		}

		for (int i = 0; i < WINDOW_SIZE && currentWindowOrigin < numberOfPackets && currentWindow[(currentWindowOrigin + sequenceSeed) % SEQUENCE_RANGE] == NULL; i++)
			currentWindowOrigin++;

		if (currentWindowOrigin == numberOfPackets)
			finalizePayload();
		else
			normalizeCurrentWindow();
	}

	void Sender::finalizePayload()
	{
		int index = (numberOfPackets + sequenceSeed) % SEQUENCE_RANGE;
		windowState[index] = new bool(false);
		currentWindow[index] = new SenderThread(socket, serverId, clientId, &DestAddr, windowState[index], RESP_ERR, index, "EOF", 4);
		currentWindow[index]->start();
	}

	bool Sender::isPayloadSent() {
		return isComplete;
	}

	SenderThread::SenderThread(int sendingSocket, int serverId, int clientId, struct sockaddr_in* destinationAddress, bool* isAcked, Type messageType, int sequenceNumber, char *packetContents, int packetLength)
	{
		socket = sendingSocket;
		destination = destinationAddress;
		this->isAcked = isAcked;
		msg = new Msg();
		msg->type = messageType;
		msg->serverId = serverId;
		msg->clientId = clientId;
		msg->length = packetLength;
		msg->sequenceNumber = sequenceNumber;

		memcpy(msg->buffer, packetContents, packetLength);
	}

	void SenderThread::run()
	{
		int addrLength = sizeof(*destination);
		int expectedMsgLen = MSGHDRSIZE + msg->length;

		while (!(*isAcked)) {
			if (sendto(socket, (char *)msg, expectedMsgLen, 0, (SOCKADDR *)destination, addrLength) != expectedMsgLen) {
				std::cerr << "Send MSGHDRSIZE+length Error " << endl;
				std::cerr << WSAGetLastError() << endl;
				return;
			}

			Sleep(TIMER_DELAY);
		}

		delete this;
	}
}