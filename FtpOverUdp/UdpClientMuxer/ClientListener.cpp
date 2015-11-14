#include "stdafx.h"
#include <string>
#include <time.h>
#include "ClientListener.h"

#define RCV_BUFFER_SIZE 512

void ClientListener::run() {
	srand(time(NULL));
	char* buffer = new char[RCV_BUFFER_SIZE];
	int bufferLength;

	sockaddr_in currentClient;
	int clientId;
	int clientAddrLen = sizeof(currentClient);
	int n;

	int routerAddrLen = sizeof(routerAddress);

	while (true) {
		if ((bufferLength = recvfrom(clientSocket, buffer, RCV_BUFFER_SIZE, 0, (SOCKADDR*)&currentClient, &clientAddrLen)) == SOCKET_ERROR)
		{
			cerr << endl << "ClientListener: recvfrom(...) failed when getting message" << endl;
			cerr << WSAGetLastError() << endl;
			exit(1);
		}

		memcpy(&clientId, buffer, sizeof(clientId));

		if (clientId == -1) {
			// Assign & register new client ID
			clientLock.waitForSignalling();
			do {
				clientId = rand() % 256;
			} while (clientMap.find(clientId) != clientMap.end());

			clientMap.insert(std::make_pair(clientId, currentClient));
			clientLock.finalizeSignalling();
			clientLock.finalizeConsumption();

			// Return ID back to client
			if ((n = sendto(clientSocket, (char*)&clientId, sizeof(clientId), 0, (SOCKADDR *)&currentClient, clientAddrLen)) != sizeof(clientId)) {
				std::cerr << "ClientListener: Send error " << endl;
				std::cerr << WSAGetLastError() << endl;
				exit(1);
			}
		} else {
			string logItem = string("From client: (").append(to_string(buffer[8])).append(") - ").append(buffer + MSGHDRSIZE);
			log(logItem);
			if ((n = sendto(routerSocket, buffer, bufferLength, 0, (SOCKADDR *)&routerAddress, routerAddrLen)) != bufferLength) {
				std::cerr << "ClientListener: Send error " << endl;
				std::cerr << WSAGetLastError() << endl;
				exit(1);
			}
		}
	}
}

void ClientListener::log(std::string logItem)
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