#pragma once

#ifdef COMMON_EXPORTS
#define COMMON_API __declspec(dllexport)
#else
#define COMMON_API __declspec(dllimport)
#endif

#include <queue>
#include "Common.h"

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

		std::queue<Payload*> completedSet;
		int totalPayloadSize;
		Payload *currentWindow[SEQUENCE_RANGE];
		int sequenceSeed;
		int windowOrigin;
		bool isComplete;

		void sendAck(int sequenceNumber);
		bool isEofMsg(Msg*);
		bool isSequenceNumberInWindow(int);
	public:
		Receiver(int socket, int serverId, int clientId, struct sockaddr_in destAddr);
		void startNewPayload(int sequenceSeed);
		void handleMsg(Msg*);
		bool isPayloadComplete();
		Payload* getPayload();
		~Receiver();
	};
}