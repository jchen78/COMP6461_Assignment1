#pragma once

#define WINDOW_SIZE 3
#define SEQUENCE_RANGE 7
#define BUFFER_LENGTH 256
#define RCV_BUFFER_SIZE 512
#define MSGHDRSIZE 20
#define TIMER_DELAY 300
#define HOSTID_RANGE 256
#define NULL_SERVER_ID -1
#define NULL_CLIENT_ID -1

#include <winsock.h>
#include <iostream>

/* Type of Messages */
typedef enum
{
	HANDSHAKE = 1,
	GET_LIST = 5,
	GET_FILE = 6,
	PUT = 15,
	POST = 16,
	RESP = 10,
	RESP_ERR = 12,
	ACK = 14,
	TERMINATE = 20
} Type;

/* Message format used for sending and receiving data */
struct __declspec(dllexport) Msg
{
public:
	Msg() {
		memset(buffer, 0, BUFFER_LENGTH);
	}
	int  clientId;
	int  serverId;
	Type type;
	int  length;
	int  sequenceNumber;
	char buffer[BUFFER_LENGTH];
};

struct __declspec(dllexport) Payload
{
public:
	int length;
	char* data;
};
// Method to copy Msg contents