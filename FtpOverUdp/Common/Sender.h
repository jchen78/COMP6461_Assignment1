#ifdef COMMON_EXPORTS
#define COMMON_API __declspec(dllexport)
#else
#define COMMON_API __declspec(dllimport)
#endif

#pragma once
#include <mutex>
#include <condition_variable>
#include "Common.h"
#include "Thread.h"

namespace Common
{
	typedef enum {
		ACTIVE,
		COMPLETE
	} SenderState;

	struct COMMON_API AsyncLock
	{
	public:
		bool isAsyncReady;
		std::mutex dataLock;
		std::condition_variable operationLock;
	};

	class COMMON_API Sender : public Thread
	{
	private:
		SenderState currentState;
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
		AsyncLock externalControl;

		void normalizeCurrentWindow();
		void receiveAck();
		void finalizePayload();
	public:
		Sender(int socket, int serverId, int clientId, struct sockaddr_in serverAddress);
		AsyncLock* getAsyncControl();
		void initializePayload(const char* completePayload, int payloadLength, int firstSequenceNumber, Msg* ackMsg);
		void run();
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
		}
	};
}