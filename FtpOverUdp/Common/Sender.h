#ifdef COMMON_EXPORTS
#define COMMON_API __declspec(dllexport)
#else
#define COMMON_API __declspec(dllimport)
#endif

#pragma once
#include <mutex>
#include "Common.h"
#include "Thread.h"

namespace Common
{
	typedef enum {
		ACTIVE,
		COMPLETE
	} SenderState;

	class COMMON_API Sender
	{
	private:
		SenderState currentState;
		class SenderThread *currentWindow[SEQUENCE_RANGE];                    // Array of pointers, not 2D array.
		bool *windowState[SEQUENCE_RANGE];                                    // Array of pointers, not 2D array.

		int socket;
		int serverId;
		int clientId;
		struct sockaddr_in ServAddr;
		
		char *completePayload;
		int numberOfPackets;
		int payloadSize;
		int currentWindowOrigin;
		int sequenceSeed;
		Msg* currentAck;
		std::mutex* externalControl;

		void normalizeCurrentWindow();
		void receiveAck();
		void finalizePayload();

	public:
		Sender(int socket, int serverId, int clientId, struct sockaddr_in serverAddress);
		void initializePayload(const char* completePayload, int payloadLength);
		void send(int firstSequenceNumber, Msg* ackMsg, std::mutex *ackSync);
		SenderState getCurrentState();
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
		~SenderThread() {
			delete msg;
			delete isAcked;
		}
	};
}