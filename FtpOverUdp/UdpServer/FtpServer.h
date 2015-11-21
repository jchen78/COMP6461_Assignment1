/*************************************************************************************
*								 File Name	: Server.h		   			 	         *
*	Usage : Handles Client request for Uploading, downloading and listing of files   *
**************************************************************************************/
#ifndef SER_TCP_H
#define SER_TCP_H

#define HOSTNAME_LENGTH 20
#define RESP_LENGTH 40
#define FILENAME_LENGTH 20
#define REQUEST_PORT 5000
#define RCV_BUFFER_SIZE 512
#define MAXPENDING 10
#define TRACE 1

#include "ServerThread.h"
#include <map>

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
		char filesDirectory[13];
		bool areServerIdsReused;
		class AsyncLock ioLock;
		class std::map<int, int> clientIds;
		class std::map<int, Msg> messages;
		class std::map<int, bool> isThreadActive;
		class std::map<int, AsyncLock*> threadLocks;

		Msg* getMessage();
		void log(const std::string &logItem);	/* */
		void dispatchMessage(int, Msg*);
		int getServerId(Msg*);
		bool serverIdExists(int);
		int createServerThread();
		std::string describeMessage(Msg*);
	public:
		FtpServer();
		~FtpServer();
		void start();							/* Starts the FtpServer */
};

#endif
