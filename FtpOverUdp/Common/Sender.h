#pragma once
#include "Common.h"
#include "Thread.h"
#include <queue>

namespace Common
{
	class COMMON_API Sender
	{
	private:
		bool isComplete;
		class SenderThread *currentWindow[SEQUENCE_RANGE];                    // Array of pointers, not 2D array.
		bool *windowState[SEQUENCE_RANGE];                                    // Array of pointers, not 2D array.

		int socket;
		int serverId;
		int clientId;
		struct sockaddr_in DestAddr;
		
		char *completePayload;
		int numberOfPackets;
		int payloadSize;
		int currentWindowOrigin;
		int sequenceSeed;
		Msg* currentAck;

		void normalizeCurrentWindow();
		void finalizePayload();
	public:
		Sender(int socket, int serverId, int clientId, struct sockaddr_in destAddress);
		void initializePayload(const char* completePayload, int payloadLength, int firstSequenceNumber, Msg* ackMsg);
		void send();
		void processAck();
		bool isPayloadSent();
		void terminateCurrentTransmission();
		int finalSequenceNumber() { return (sequenceSeed + numberOfPackets + 1) % SEQUENCE_RANGE; }
		~Sender() {
			delete[] completePayload;
		}
	};

	class COMMON_API SenderThread : public Thread
	{
	private:
		int socket;
		struct sockaddr_in* destination;
		Msg* msg;
		bool* isAcked;

		void msgSend();
	public:
		SenderThread(int sendingSocket, int serverId, int clientId, struct sockaddr_in* destinationAddress, bool* isAcked, Type messageType, int sequenceNumber, char *packetContents, int packetLength);
		void run();
		~SenderThread() { }
	};
}