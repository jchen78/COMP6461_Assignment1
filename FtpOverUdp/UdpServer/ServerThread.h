#pragma once

#include <Common.h>
#include <Thread.h>
#include <AsyncLock.h>
#include <Sender.h>
#include <Receiver.h>

using namespace Common;

typedef enum {
	INITIALIZING,
	STARTING_HANDSHAKE,
	HANDSHAKING,
	WAITING_FOR_REQUEST,
	SENDING,
	RECEIVING,
	RENAMING,
	EXITING,
	TERMINATED
} ServerState;

class ServerThread : public Thread
{
private:
	// Basic thread configuration
	int log;
	int serverId;
	int socket;
	struct sockaddr_in address;
	char filesDirectory[13];
	AsyncLock* ioLock;

	// Communication with server multiplexer
	class AsyncLock outerSync;
	ServerState currentState;
	struct Msg* currentMsg;

	class Sender* sender;
	class Receiver* receiver;
	std::string filename;
	SenderThread* currentResponse;
	bool* isResponseComplete;

	// Private methods to handle requests
	void startHandshake();
	void endHandshake();
	void sendList();
	Payload* getDirectoryContents();
	void sendFile();
	Payload* getFileContents(const char* fileName);
	void startSender(Payload* contents);
	void dispatchToSender();
	void getFile();
	void dispatchToReceiver();
	void saveFile(Payload* fileContents);

	void notifyWrongState();
	void resetToReadyState();
	void terminate();

	void sendMsg(Msg*);
public:
	/* NOTE: ServerThread will need to share an I/O mutex */
	ServerThread(int serverId, int serverSocket, struct sockaddr_in clientAddress, Msg* initialHandshake, AsyncLock* ioLock);
	int getId();
	AsyncLock* getSync();
	virtual void run();
	/* Switch by current state, msg type & misc. conditions (in order):
	*		- If state is Initializing & message is Handshake (impossible for other message states):
	*			- Set state to Handshaking
	*			- Send HANDSHAKE (done)
	*		- If state is Handshaking & msg is CompleteHandshake:
	*			- Stop sending HANDSHAKE
	*			- Set state to WaitingForRequest
	*		- If state is Handshaking & msg is any:
	*			- Send HANDSHAKE (do not bother with sequence number: could be randomly correct)
	*		- If state is WaitingForRequest & message is GetList:
	*			- Set state to Sending
	*			- Initialize Sender field
	*			- Start Sender
	*		- If state is WaitingForRequest & message is invalid GetFile:
	*			- Send RESP_ERR
	*		- If state is WaitingForRequest & message is valid GetFile:
	*			- Set state to Sending
	*			- Initialize Sender field
	*		- If state is WaitingForRequest & message is Post (note: if file already exists, add random suffix to filename):
	*			- Set state to Receiving
	*			- Initialize (as yet not-coded) Receiver field
	*			- Send ACK
	*		- If state is WaitingForRequest & message is invalid Put:
	*			- Send RESP_ERR
	*		- If state is WaitingForRequest & message is valid Put:
	*			- Set state to Renaming
	*			- Set originalFileName
	*			- Send ACK
	*		- If state is WaitingForRequest & message is Terminate w/out message body:
	*			- Set state to Exiting
	*			- Initialize exit timeout (3 delays, 2 duplicate TERMINATE responses)
	*			- Send TERMINATE
	*		- If state is WaitingForRequest & message is any:
	*			- Send RESP_ERR
	*		- If state is Sending & message is Ack:
	*			- Dispatch msg to Sender field
	*			- If Sender state is completed, set state to WaitingForRequest
	*		- If state is Sending & message is any:
	*			- Send RESP_ERR
	*		- If state is Receiving & message is Resp:
	*			- Dispatch to Receiver
	*		- If state is Receiving & message is RespErr EOF:
	*			- Set state to WaitingForRequest
	*			- Dispatch to Receiver
	*		- If state is Receiving & message is any:
	*			- Send RESP_ERR
	*		- If state is Renaming & message is Resp:
	*			- If rename is successful:
	*				- Set state to WaitingForRequest
	*				- Send ACK
	*			- Otherwise:
	*				- Send RESP_ERR
	*		- If state is Renaming & message is Terminate w/ originalFileName as message body:
	*			- Set state to WaitingForRequest
	*			- Clear originalFileName field
	*			- Send ACK
	*		- If state is Renaming & message is any:
	*			- Send RESP_ERR
	*		- If state is EXITING & message is ACK:
	*			- Exit (no response)
	*		- If state is EXITING & 3rd TERMINATE is sent (2nd duplicate TERMINATE):
	*			- Exit (no response & no extra timeout)
	*/
	virtual ~ServerThread() {
		if (sender != NULL)
			delete sender;

		if (currentResponse != NULL)
			delete currentResponse;
	}
};
