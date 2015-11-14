#include "stdafx.h"
#include <string>
#include <time.h>
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
			cerr << "RouterListener: recvfrom(...) failed when getting message" << endl;
			cerr << WSAGetLastError() << endl;
			exit(1);
		}

		memcpy(&msg, buffer, bufferLength);
		string logItem = string("From router: (").append(to_string(buffer[8])).append(") - ").append(msg.buffer);
		log(logItem);

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

void RouterListener::log(std::string logItem)
{
	time_t rawtime;
	char* formattedTime;

	time(&rawtime);
	char* unformattedTime = asctime(localtime(&rawtime));
	int length = strlen(unformattedTime);
	formattedTime = new char[length];
	memcpy(formattedTime, unformattedTime, length);
	formattedTime[length - 1] = 0;

	FILE *logFile = fopen("muxerLog.txt", "a");
	fprintf(logFile, "Server (root): %s -- %s\r\n", formattedTime, logItem.c_str());
	fclose(logFile);
}
