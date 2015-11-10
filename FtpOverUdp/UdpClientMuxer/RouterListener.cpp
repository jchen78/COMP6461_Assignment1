#include "stdafx.h"
#include <Common.h>
#include "RouterListener.h"


RouterListener::RouterListener(int routerSocket, int clientSocket, map<int, sockaddr_in> *clientMap) : routerSocket(routerSocket), clientSocket(clientSocket), clients(clientMap) {
}

void RouterListener::run() {
	char* buffer = new char[RCV_BUFFER_SIZE];
	int bufferLength;

	Msg msg;
	sockaddr_in currentClient;

	while (true) {
		if ((bufferLength = recv(routerSocket, buffer, RCV_BUFFER_SIZE, 0)) == SOCKET_ERROR) 
		{
			cerr << "recvfrom(...) failed when getting message" << endl;
			cerr << WSAGetLastError() << endl;
			exit(1);
		}

		memset(&msg, 0, MSGHDRSIZE);
		memset(msg.buffer, 0, BUFFER_LENGTH);
		memcpy(&msg, buffer, bufferLength);

		currentClient = (*clients)[msg.clientId];
		int n;
		int expectedMsgLen = MSGHDRSIZE + msg.length;
		int addrLength = sizeof(currentClient);
		if ((n = sendto(clientSocket, buffer, bufferLength, 0, (SOCKADDR *)&currentClient, addrLength)) != bufferLength) {
			std::cerr << "RouterListener: Send MSGHDRSIZE+length Error " << endl;
			std::cerr << WSAGetLastError() << endl;
			exit(1);
		}
	}
}

RouterListener::~RouterListener() {
}
