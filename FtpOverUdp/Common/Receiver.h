#pragma once

#include <queue>
#include "Common.h"
#include "AsyncLock.h"

namespace Common
{
	class COMMON_API Receiver
	{
	private:
		int socket;
		int serverId;
		int clientId;
		struct sockaddr_in destAddr;
		int addrLength;

		std::queue<char*> completedSet;
		int totalPayloadSize;
		char *currentWindow[SEQUENCE_RANGE];
		int sequenceSeed;
		int windowOrigin;
		bool isComplete;

		void sendAck(int sequenceNumber);
		bool isEofMsg(Msg*);
	public:
		Receiver(int socket, int serverId, int clientId, struct sockaddr_in destAddr);
		void startNewPayload(int sequenceSeed);
		void handleMsg(Msg*);
		bool isPayloadComplete();
		Payload* getPayload();
		~Receiver();
	};
}