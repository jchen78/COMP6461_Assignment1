#include "stdafx.h"
#include <string.h>
#include <winsock.h>
#include "Sender.h"

using namespace std;

namespace Common
{
	Sender::Sender(int socket, int serverId, int clientId, struct sockaddr_in serverAddress)
	{
		this->socket = socket;
		this->ServAddr = serverAddress;
		this->serverId = serverId;
		this->clientId = clientId;
		completePayload = NULL;
	}

	void Sender::send(const char* messageContents, int messageLength)
	{
		initializePayload(messageContents, messageLength);

		do
		{
			normalizeCurrentWindow();
			receiveAck();
		} while (currentWindowOrigin < numberOfPackets);

		finalizePayload();
	}

	void Sender::initializePayload(const char* messageContents, int messageLength)
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
		currentState = ACTIVE;
	}

	void Sender::normalizeCurrentWindow()
	{
		// Assume the indices are correct and all expired workers are set to NULL;
		// merely ensure that all slots in the current window are populated.
		for (int i = 0; i < WINDOW_SIZE && currentWindowOrigin + i < numberOfPackets; i++) {
			int currentIndex = (currentWindowOrigin + i) % SEQUENCE_RANGE;
			if (currentWindow[currentIndex] == NULL) {
				bool* currentFlag = new bool(false);
				int currentChar = currentIndex * BUFFER_LENGTH;
				int currentSize = currentIndex == (numberOfPackets - 1) ? (payloadSize - currentChar - 1) : BUFFER_LENGTH;
				windowState[currentIndex] = currentFlag;
				currentWindow[currentIndex] = new SenderThread(socket, serverId, clientId, &ServAddr, currentFlag, RESP, currentIndex, &completePayload[currentChar], currentSize);
				currentWindow[currentIndex]->start();
			}
		}
	}

	void Sender::receiveAck()
	{
		// Get the ACK message
		Msg* ack = msgGet();

		if (ack->type != ACK)
			return;

		int sequenceNumber = ack->sequenceNumber;
		*windowState[sequenceNumber] = true;
		currentWindow[sequenceNumber] = NULL;

		while (currentWindow[currentWindowOrigin] == NULL)
			currentWindowOrigin++;
	}

	Msg* Sender::msgGet()
	{
		char buffer[RCV_BUFFER_SIZE];
		int bufferLength;
		int addrLength = sizeof(ServAddr);

		/* Check the received Message Header */
		if ((bufferLength = recvfrom(socket, buffer, RCV_BUFFER_SIZE, 0, (SOCKADDR *)&ServAddr, &addrLength)) == SOCKET_ERROR)
		{
			cerr << "recvfrom(...) failed when getting message" << endl;
			cerr << WSAGetLastError() << endl;
			exit(1);
		}

		// must destruct after use!
		Msg* msg = new Msg();
		memcpy(msg, buffer, MSGHDRSIZE);
		memcpy(msg->buffer, buffer + MSGHDRSIZE, msg->length);
		return msg;
	}

	void Sender::finalizePayload()
	{
		bool isAcked = false;
		Msg* finalPayloadAck = NULL;
		int finalSequenceNumber = numberOfPackets % SEQUENCE_RANGE;
		SenderThread* senderThread = new SenderThread(socket, serverId, clientId, &ServAddr, &isAcked, RESP_ERR, finalSequenceNumber, "EOF", 4);
		senderThread->start();
		do {
			finalPayloadAck = msgGet();
		} while (finalPayloadAck->sequenceNumber != finalSequenceNumber);

		isAcked = true;
	}

	SenderState Sender::getCurrentState() {
		return currentState;
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
		}
	}
}