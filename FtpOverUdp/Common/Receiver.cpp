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
		this->isComplete = false;
		this->sequenceSeed = sequenceSeed;
		this->windowOrigin = 0;
		this->totalPayloadSize = 0;
		this->completedSet = {};
		for (int i = 0; i < SEQUENCE_RANGE; i++)
			this->currentWindow[i] = NULL;
	}

	void Receiver::handleMsg(Msg* msg) {
		// ACK package
		int sequenceNumber = msg->sequenceNumber;
		int windowEnd = (windowOrigin + sequenceSeed) % SEQUENCE_RANGE + WINDOW_SIZE;

		sendAck(sequenceNumber);

		if (msg->sequenceNumber == (sequenceSeed + 1) % SEQUENCE_RANGE && msg->type == RESP_ERR && strcmp(msg->buffer, "NSF") == 0)
			throw std::exception("No such file.");

		// Handle EOF
		if (isEofMsg(msg)) {
			windowOrigin++;
			isComplete = true;
			return;
		}

		if (!isSequenceNumberInWindow(sequenceNumber))
			return;

		// Register package in buffer
		if (currentWindow[sequenceNumber] != NULL)
			return;

		int currentPayloadLength = msg->length;
		char* currentData = new char[currentPayloadLength];
		memcpy(currentData, msg->buffer, currentPayloadLength);

		Payload* currentPayload = new Payload();
		currentPayload->length = currentPayloadLength;
		currentPayload->data = currentData;
		currentWindow[sequenceNumber] = currentPayload;
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

		return (msg->sequenceNumber == (windowOrigin + sequenceSeed) % SEQUENCE_RANGE && strcmp(msg->buffer, "EOF") == 0);
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

	bool Receiver::isSequenceNumberInWindow(int sequenceNumber) {
		int currentHead = (windowOrigin + sequenceSeed) % SEQUENCE_RANGE;
		int currentTail = currentHead + WINDOW_SIZE - 1;
		if (sequenceNumber < currentHead)
			sequenceNumber += SEQUENCE_RANGE;

		return sequenceNumber >= currentHead && sequenceNumber <= currentTail;
	}

	bool Receiver::isPayloadStarted() {
		for (int i = 0; i < SEQUENCE_RANGE; i++)
			if (currentWindow[i] != NULL)
				return true;

		return windowOrigin != 0 || !completedSet.empty();
	}

	bool Receiver::isPayloadComplete() {
		return isComplete;
	}

	void Receiver::terminateCurrentTransmission() {
		// Note: Nothing to do. Cost of freeing up resources deferred to either destructor or next transmission.
	}

	int Receiver::finalSequenceNumber() {
		return (sequenceSeed + windowOrigin) % SEQUENCE_RANGE;
	}

	Payload* Receiver::getPayload() {
		if (!isComplete)
			throw std::exception("Payload not yet complete");

		Payload* payload = new Payload();
		payload->length = totalPayloadSize;
		payload->data = new char[totalPayloadSize];
		int currentIndex = 0;
		while (!completedSet.empty()) {
			Payload currentPayload = *completedSet.front();
			memcpy(&payload->data[currentIndex], currentPayload.data, currentPayload.length);
			currentIndex += currentPayload.length;
			completedSet.pop();
		}

		return payload;
	}

	Receiver::~Receiver() {
	}
}