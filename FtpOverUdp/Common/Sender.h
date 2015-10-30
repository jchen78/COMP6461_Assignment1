#ifdef COMMON_EXPORTS
#define COMMON_API __declspec(dllexport)
#else
#define COMMON_API __declspec(dllimport)
#endif

#pragma once
#include "Common.h"
#include "Thread.h"

extern "C"
{
	namespace Common
	{
		class COMMON_API Sender
		{
		private:
			char *completePayload;
			int numberOfPackets;
			int payloadSize;
			int currentWindowOrigin;
			class SenderThread *currentWindow[SEQUENCE_RANGE];                    // Array of pointers, not 2D array.
			bool *windowState[SEQUENCE_RANGE];                                    // Array of pointers, not 2D array.

			int socket;
			struct sockaddr_in ServAddr;

			void initializePayload(const char*, int);
			void normalizeCurrentWindow();
			void receiveAck();
			void finalizePayload();

			Msg* msgGet();
		public:
			Sender(int socket, struct sockaddr_in serverAddress);
			void send(const char *messageContents, int messageLength);
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
			SenderThread(int sendingSocket, struct sockaddr_in* destinationAddress, bool* isAcked, Type messageType, int sequenceNumber, char *packetContents, int packetLength);
			void run();
			~SenderThread() {
				delete msg;
				delete isAcked;
			}
		};
	}
}