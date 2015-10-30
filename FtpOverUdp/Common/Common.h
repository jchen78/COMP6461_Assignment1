#pragma once

#define WINDOW_SIZE 3
#define SEQUENCE_RANGE 7
#define BUFFER_LENGTH 256
#define RCV_BUFFER_SIZE 512
#define MSGHDRSIZE 12
#define TIMER_DELAY 1000
#define HOSTID_RANGE 256

#include <winsock.h>
#include <iostream>

/* Type of Messages */
typedef enum
{
	HANDSHAKE = 1,
	COMPLETE_HANDSHAKE = 2,
	REQ_LIST = 5,
	REQ_GET = 6,
	RESP = 10,
	RESP_ERR = 12,
	ACK = 14,
	PUT = 15,
	TERMINATE = 20
} Type;

/* Message format used for sending and receiving data */
typedef struct
{
	Type type;
	int  length;
	int  sequenceNumber;
	char buffer[BUFFER_LENGTH];
} Msg;