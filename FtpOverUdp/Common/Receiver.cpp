#include "stdafx.h"
#include "Receiver.h"

namespace Common
{
	Receiver::Receiver(int socket, int serverId, int clientId, struct sockaddr_in destAddr) {
		this->socket = socket;
		this->serverId = serverId;
		this->clientId = clientId;
		this->destAddr = destAddr;
		this->addrLength = sizeof(destAddr);
	}

	void Receiver::startNewPayload(int sequenceSeed) {
		this->sequenceSeed = sequenceSeed;
		this->windowOrigin = 0;
		this->completedSet = {};
		for (int i = 0; i < SEQUENCE_RANGE; i++)
			this->currentWindow[i] = NULL;
	}

	void Receiver::handleMsg(Msg* msg) {
		// ACK package
		int sequenceNumber = msg->sequenceNumber;
		sendAck(sequenceNumber);

		// Handle EOF
		if (isEofMsg(msg)) {
			isComplete = true;
			return;
		}

		// Register package in buffer
		if (currentWindow[sequenceNumber] != NULL)
			return;

		int currentPayloadLength = msg->length;
		currentWindow[sequenceNumber] = new char[BUFFER_LENGTH];
		memcpy(currentWindow[sequenceNumber], msg->buffer, currentPayloadLength);
		totalPayloadSize += currentPayloadLength; // It doesn't matter if it's out of order.

		// Normalize buffer
		int currentSequence = (windowOrigin + sequenceSeed) % SEQUENCE_RANGE;
		while (currentWindow[currentSequence] != NULL) {
			completedSet.push(currentWindow[currentSequence]);
			currentWindow[currentSequence] = NULL;
			currentSequence = (++windowOrigin + sequenceSeed) % SEQUENCE_RANGE;
		}
	}

	bool Receiver::isEofMsg(Msg* msg) {
		if (msg->type != RESP_ERR)
			return false;

		return (strcmp(msg->buffer, "EOF") == 0);
	}

	void Receiver::sendAck(int sequenceNumber) {
		Msg msg;
		msg.clientId = clientId;
		msg.serverId = serverId;
		msg.sequenceNumber = sequenceNumber;
		msg.type = ACK;
		msg.length = 0;
		if (sendto(socket, (char *)&msg, MSGHDRSIZE, 0, (SOCKADDR *)&destAddr, addrLength) != MSGHDRSIZE) {
			std::cerr << "Send MSGHDRSIZE+length Error " << std::endl;
			std::cerr << WSAGetLastError() << std::endl;
			return;
		}
	}

	bool Receiver::isPayloadComplete() {
		return isComplete;
	}

	Payload* Receiver::getPayload() {
		if (!isComplete)
			throw new std::exception("Payload not yet complete");

		Payload* payload = new Payload();
		payload->length = totalPayloadSize;
		payload->data = new char[totalPayloadSize];
		for (int currentIndex = 0, currentLength; !completedSet.empty(); currentIndex += BUFFER_LENGTH) {
			currentLength = totalPayloadSize - currentIndex;
			if (currentLength > BUFFER_LENGTH)
				currentLength = BUFFER_LENGTH;

			memcpy(&payload->data[currentIndex], completedSet.front(), currentLength);
			completedSet.pop();
		}

		return payload;
	}

	Receiver::~Receiver() {
	}
}