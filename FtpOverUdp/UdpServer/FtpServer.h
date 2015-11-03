/*************************************************************************************
*								 File Name	: Server.h		   			 	         *
*	Usage : Handles Client request for Uploading, downloading and listing of files   *
**************************************************************************************/
#ifndef SER_TCP_H
#define SER_TCP_H

#define HOSTNAME_LENGTH 20
#define RESP_LENGTH 40
#define FILENAME_LENGTH 20
#define REQUEST_PORT 5001
#define RCV_BUFFER_SIZE 512
#define MAXPENDING 10
#define TRACE 1

#include <queue>
#include <mutex>
#include <Common.h>
#include <Sender.h>
#include <Thread.h>

using namespace Common;

typedef enum
{
	Initialized,
	HandshakeStarted,
	ReceivingRequest,
	Sending,
	Terminated
} ThreadState;

/* Request message structure */
typedef struct
{
	char hostname[HOSTNAME_LENGTH];
	char filename[FILENAME_LENGTH];
} Req;

/* Response message structure */
typedef struct
{
	char response[RESP_LENGTH];
} Resp;

/* FtpServer Class */
class FtpServer
{
	private:
		int serverSock;							/* Socket descriptor for server and client*/
		struct sockaddr_in ClientAddr;			/* Client address */
		struct sockaddr_in ServerAddr;			/* Server address */
		unsigned short ServerPort;				/* Server port */
		int clientLen;							/* Length of Server address data structure */
		char serverName[HOSTNAME_LENGTH];		/* Server Name */
		char* filesDirectory;

		void log(const std::string &logItem);	/* */
	public:
		FtpServer();
		~FtpServer();
		void start();							/* Starts the FtpServer */
};

typedef enum {
	INITIALIZING,
	HANDSHAKING,
	WAITING_FOR_REQUEST,
	SENDING,
	RECEIVING,
	RENAMING,
	EXITING
} ServerState;

class ServerThread : public Thread
{
private:
	// Basic thread configuration
	int serverId;
	int socket;
	struct sockaddr_in address;
	char* filesDirectory;

	// Communication with server multiplexer
	class std::mutex outerSync;
	ServerState currentState;
	class Msg* currentMsg;

	// Communication with sender module
	class std::mutex senderSync;
	class Sender* sender;

	// State for renaming files
	std::string originalFileName;

	// Single-packet communications
	SenderThread* currentResponse;
	bool* isResponseComplete;

	// Private methods to handle requests
	void startHandshake();
	void endHandshake();
	void sendList();
	char* getDirectoryContents();
	void sendFile();
	char* getFileContents(const char* fileName);
	void startSender(const char* contents);
public:
	ServerThread(int serverSocket, struct sockaddr_in serverAddress, Msg* initialHandshake);
	int getId();
	std::mutex* getSync();
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
	*			- Start Sender
	*		- If state is WaitingForRequest & message is Put:
	*			- Set state to Receiving
	*			- Initialize (as yet not-coded) Receiver field
	*			- Start Receiver
	*			- Send ACK
	*		- If state is WaitingForRequest & message is invalid Post:
	*			- Send RESP_ERR
	*		- If state is WaitingForRequest & message is valid Post:
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
		outerSync.unlock();
		senderSync.unlock();

		if (currentMsg != NULL)
			delete currentMsg;

		if (sender != NULL)
			delete sender;

		if (currentResponse != NULL)
			delete currentResponse;

		delete[] filesDirectory;
	}
};

#endif
